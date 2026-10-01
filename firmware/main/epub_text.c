#include "epub_text.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "miniz.h"

#define ZIP_EOCD UINT32_C(0x06054b50)
#define ZIP_CENTRAL UINT32_C(0x02014b50)
#define ZIP_LOCAL UINT32_C(0x04034b50)
#define MAX_EPUB_ENTRY (2U * 1024U * 1024U)
#define MAX_BOOK_TEXT (512U * 1024U)

typedef struct {
    char name[384];
    uint16_t method;
    uint32_t compressed;
    uint32_t uncompressed;
    uint32_t local_offset;
} zip_entry_t;

typedef struct {
    FILE *file;
    uint32_t central_offset;
    uint16_t entries;
} zip_file_t;

/* Reserved at link time so EPUB reads never depend on the fragmented heap. */
static DRAM_ATTR uint32_t epub_dma_words[256];

static uint16_t le16(const unsigned char *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/*
 * SDMMC can DMA directly only into internal DMA-capable memory.  EPUB working
 * buffers deliberately live in PSRAM; passing a large one to fread() makes
 * the driver try to allocate an equally large internal bounce buffer.  Once
 * the speech engine is running that allocation is neither reliable nor
 * necessary, so copy through a small stack buffer instead.
 */
static bool read_to_psram(FILE *file, void *destination, size_t length)
{
    unsigned char *dma = (unsigned char *)epub_dma_words;
    unsigned char *out = destination;
    while (length) {
        size_t amount = length < sizeof(epub_dma_words)
            ? length : sizeof(epub_dma_words);
        if (fread(dma, 1, amount, file) != amount)
            return false;
        memcpy(out, dma, amount);
        out += amount;
        length -= amount;
    }
    return true;
}

static bool zip_open(zip_file_t *zip, const char *path)
{
    memset(zip, 0, sizeof(*zip));
    zip->file = fopen(path, "rb");
    if (!zip->file)
        return false;
    fseek(zip->file, 0, SEEK_END);
    long size = ftell(zip->file);
    size_t tail_size = size > 65557 ? 65557 : (size_t)size;
    unsigned char *tail = heap_caps_malloc(tail_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!tail) goto fail;
    fseek(zip->file, size - (long)tail_size, SEEK_SET);
    if (!read_to_psram(zip->file, tail, tail_size)) {
        free(tail);
        goto fail;
    }
    for (size_t at = tail_size >= 22 ? tail_size - 22 : 0;; --at) {
        if (le32(tail + at) == ZIP_EOCD) {
            zip->entries = le16(tail + at + 10);
            zip->central_offset = le32(tail + at + 16);
            free(tail);
            return true;
        }
        if (at == 0) break;
    }
    free(tail);
fail:
    fclose(zip->file);
    zip->file = NULL;
    return false;
}

static bool zip_find(zip_file_t *zip, const char *wanted, zip_entry_t *entry)
{
    unsigned char header[46];
    fseek(zip->file, (long)zip->central_offset, SEEK_SET);
    for (unsigned index = 0; index < zip->entries; ++index) {
        if (fread(header, 1, sizeof(header), zip->file) != sizeof(header)
            || le32(header) != ZIP_CENTRAL)
            return false;
        uint16_t name_length = le16(header + 28);
        uint16_t extra_length = le16(header + 30);
        uint16_t comment_length = le16(header + 32);
        size_t copy = name_length < sizeof(entry->name) - 1
            ? name_length : sizeof(entry->name) - 1;
        if (fread(entry->name, 1, copy, zip->file) != copy)
            return false;
        entry->name[copy] = 0;
        if (name_length > copy)
            fseek(zip->file, (long)(name_length - copy), SEEK_CUR);
        entry->method = le16(header + 10);
        entry->compressed = le32(header + 20);
        entry->uncompressed = le32(header + 24);
        entry->local_offset = le32(header + 42);
        fseek(zip->file, (long)extra_length + comment_length, SEEK_CUR);
        if (!strcmp(entry->name, wanted))
            return true;
    }
    return false;
}

static char *zip_extract(zip_file_t *zip, const char *name, size_t *length)
{
    zip_entry_t entry;
    if (!zip_find(zip, name, &entry) || entry.uncompressed > MAX_EPUB_ENTRY)
        return NULL;
    unsigned char local[30];
    fseek(zip->file, (long)entry.local_offset, SEEK_SET);
    if (fread(local, 1, sizeof(local), zip->file) != sizeof(local)
        || le32(local) != ZIP_LOCAL)
        return NULL;
    fseek(zip->file, (long)le16(local + 26) + le16(local + 28), SEEK_CUR);
    unsigned char *compressed = heap_caps_malloc(entry.compressed ?: 1,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *output = heap_caps_malloc((size_t)entry.uncompressed + 1,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!compressed || !output
        || !read_to_psram(zip->file, compressed, entry.compressed)) {
        free(compressed); free(output); return NULL;
    }
    bool okay = false;
    if (entry.method == 0) {
        memcpy(output, compressed, entry.uncompressed);
        okay = entry.compressed == entry.uncompressed;
    } else if (entry.method == 8) {
        size_t made = tinfl_decompress_mem_to_mem(output, entry.uncompressed,
            compressed, entry.compressed, 0);
        okay = made == entry.uncompressed;
    }
    free(compressed);
    if (!okay) { free(output); return NULL; }
    output[entry.uncompressed] = 0;
    *length = entry.uncompressed;
    return output;
}

static bool attribute(const char *tag, const char *name, char *out, size_t cap)
{
    char needle[48];
    snprintf(needle, sizeof(needle), "%s=", name);
    const char *at = strstr(tag, needle);
    if (!at) return false;
    at += strlen(needle);
    if (*at != '\'' && *at != '"') return false;
    char quote = *at++;
    const char *end = strchr(at, quote);
    if (!end) return false;
    size_t n = (size_t)(end - at);
    if (n >= cap) n = cap - 1;
    memcpy(out, at, n); out[n] = 0;
    return true;
}

static void base_dir(const char *path, char *out, size_t cap)
{
    strlcpy(out, path, cap);
    char *slash = strrchr(out, '/');
    if (slash) slash[1] = 0; else out[0] = 0;
}

static void append_space(char *out, size_t *used)
{
    if (*used && out[*used - 1] != ' ' && *used + 1 < MAX_BOOK_TEXT)
        out[(*used)++] = ' ';
}

static uint32_t utf8_codepoint(const unsigned char *s, size_t *bytes)
{
    if (s[0] < 0x80) { *bytes = 1; return s[0]; }
    if ((s[0] & 0xe0) == 0xc0 && (s[1] & 0xc0) == 0x80) {
        *bytes = 2;
        return ((uint32_t)(s[0] & 0x1f) << 6) | (s[1] & 0x3f);
    }
    if ((s[0] & 0xf0) == 0xe0 && (s[1] & 0xc0) == 0x80
        && (s[2] & 0xc0) == 0x80) {
        *bytes = 3;
        return ((uint32_t)(s[0] & 0x0f) << 12)
            | ((uint32_t)(s[1] & 0x3f) << 6) | (s[2] & 0x3f);
    }
    if ((s[0] & 0xf8) == 0xf0 && (s[1] & 0xc0) == 0x80
        && (s[2] & 0xc0) == 0x80 && (s[3] & 0xc0) == 0x80) {
        *bytes = 4;
        return ((uint32_t)(s[0] & 7) << 18)
            | ((uint32_t)(s[1] & 0x3f) << 12)
            | ((uint32_t)(s[2] & 0x3f) << 6) | (s[3] & 0x3f);
    }
    *bytes = 1;
    return '?';
}

static unsigned char windows_1252(uint32_t cp)
{
    if (cp <= 0x7f || (cp >= 0xa0 && cp <= 0xff))
        return (unsigned char)cp;
    struct mapping { uint32_t cp; unsigned char byte; };
    static const struct mapping map[] = {
        {0x20ac,0x80}, {0x201a,0x82}, {0x0192,0x83}, {0x201e,0x84},
        {0x2026,0x85}, {0x2020,0x86}, {0x2021,0x87}, {0x02c6,0x88},
        {0x2030,0x89}, {0x0160,0x8a}, {0x2039,0x8b}, {0x0152,0x8c},
        {0x017d,0x8e}, {0x2018,0x91}, {0x2019,0x92}, {0x201c,0x93},
        {0x201d,0x94}, {0x2022,0x95}, {0x2013,0x96}, {0x2014,0x97},
        {0x02dc,0x98}, {0x2122,0x99}, {0x0161,0x9a}, {0x203a,0x9b},
        {0x0153,0x9c}, {0x017e,0x9e}, {0x0178,0x9f},
    };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
        if (map[i].cp == cp) return map[i].byte;
    return '?';
}

static bool is_block_tag(const char *at)
{
    if (*at != '<') return false;
    ++at;
    if (*at == '/') ++at;
    while (isspace((unsigned char)*at)) ++at;
    static const char *const blocks[] = {
        "p", "div", "br", "li", "ul", "ol", "blockquote",
        "section", "article", "aside", "header", "footer", "hr",
        "table", "tr", "td", "th", "pre", "h1", "h2", "h3",
        "h4", "h5", "h6",
    };
    for (size_t i = 0; i < sizeof(blocks) / sizeof(blocks[0]); ++i) {
        size_t n = strlen(blocks[i]);
        if (!strncasecmp(at, blocks[i], n)
            && (isspace((unsigned char)at[n]) || at[n] == '>'
                || at[n] == '/'))
            return true;
    }
    return false;
}

static void extract_heading(const char *html, char *out, size_t capacity)
{
    const char *start = NULL;
    for (int level = 1; level <= 6; ++level) {
        char needle[5]; snprintf(needle, sizeof(needle), "<h%d", level);
        const char *candidate = strstr(html, needle);
        if (candidate && (!start || candidate < start)) start = candidate;
    }
    if (!start || !(start = strchr(start, '>'))) return;
    ++start;
    const char *end = strstr(start, "</h");
    if (!end) return;
    bool tag = false;
    size_t used = 0;
    for (const char *p = start; p < end && used + 1 < capacity;) {
        if (*p == '<') { tag = true; ++p; continue; }
        if (tag) { if (*p++ == '>') tag = false; continue; }
        if (*p == '&') {
            if (!strncmp(p, "&amp;", 5)) { out[used++] = '&'; p += 5; continue; }
            if (!strncmp(p, "&quot;", 6)) { out[used++] = '"'; p += 6; continue; }
            if (!strncmp(p, "&apos;", 6)) { out[used++] = '\''; p += 6; continue; }
            if (!strncmp(p, "&nbsp;", 6)) { out[used++] = ' '; p += 6; continue; }
        }
        size_t bytes = 1;
        uint32_t cp = utf8_codepoint((const unsigned char *)p, &bytes);
        unsigned char c = windows_1252(cp);
        p += bytes;
        if (isspace(c)) {
            if (used && out[used - 1] != ' ') out[used++] = ' ';
        } else if (c >= 32) out[used++] = (char)c;
    }
    while (used && out[used - 1] == ' ') --used;
    out[used] = 0;
}

static void add_marker(epub_document_t *document, epub_marker_type_t type,
                       uint8_t level, size_t offset, const char *name)
{
    if (document->marker_count >= EPUB_MAX_MARKERS)
        return;
    epub_marker_t *marker = &document->markers[document->marker_count++];
    marker->type = type;
    marker->level = level;
    marker->text_offset = offset;
    if (name)
        strlcpy(marker->name, name, sizeof(marker->name));
}

static void append_image_alt(const char *description, char *out, size_t *used)
{
    if (!description || !description[0]) return;
    append_space(out, used);
    for (const char *p = description; *p && *used + 2 < MAX_BOOK_TEXT;) {
        if (*p == '&') {
            if (!strncmp(p, "&amp;", 5)) { out[(*used)++] = '&'; p += 5; continue; }
            if (!strncmp(p, "&quot;", 6)) { out[(*used)++] = '"'; p += 6; continue; }
            if (!strncmp(p, "&apos;", 6)) { out[(*used)++] = '\''; p += 6; continue; }
            if (!strncmp(p, "&nbsp;", 6)) { append_space(out, used); p += 6; continue; }
        }
        size_t bytes = 1;
        uint32_t cp = utf8_codepoint((const unsigned char *)p, &bytes);
        unsigned char c = windows_1252(cp);
        p += bytes;
        if (isspace(c)) append_space(out, used);
        else if (c >= 32) out[(*used)++] = (char)c;
    }
    append_space(out, used);
}

static void html_to_text(const char *html, char *out, size_t *used,
                         epub_document_t *document)
{
    /* XHTML metadata belongs to the document head, not the reading stream.
       Begin at the body when it is present so <title> and other head text can
       never become the first words of a section. */
    const char *body = html;
    for (const char *p = html; *p; ++p) {
        if (!strncasecmp(p, "<body", 5)
            && (isspace((unsigned char)p[5]) || p[5] == '>')) {
            const char *end = strchr(p, '>');
            if (end) body = end + 1;
            break;
        }
    }
    bool tag = false, suppress = false;
    size_t i = 0;
    for (; body[i] && *used + 2 < MAX_BOOK_TEXT;) {
        if (!tag && body[i] == '<') {
            const char *element = body + i;
            if (element[1] && tolower((unsigned char)element[1]) == 'h'
                && element[2] >= '1' && element[2] <= '6'
                && (isspace((unsigned char)element[3])
                    || element[3] == '>')) {
                char heading[sizeof(document->sections[0].name)] = { 0 };
                extract_heading(element, heading, sizeof(heading));
                add_marker(document, EPUB_MARKER_HEADING,
                           (uint8_t)(element[2] - '0'), *used, heading);
            } else if (!strncasecmp(element, "<img", 4)
                       && (isspace((unsigned char)element[4])
                           || element[4] == '>')) {
                char description[96] = { 0 };
                (void)attribute(element, "alt", description,
                                sizeof(description));
                append_image_alt(description, out, used);
            }
            bool block = is_block_tag(body + i);
            tag = true;
            if (!strncasecmp(body + i, "<script", 7)
                || !strncasecmp(body + i, "<style", 6)) suppress = true;
            if (!strncasecmp(body + i, "</script", 8)
                || !strncasecmp(body + i, "</style", 7)) suppress = false;
            if (block) append_space(out, used);
            ++i; continue;
        }
        if (tag) {
            if (body[i++] == '>') tag = false;
            continue;
        }
        if (suppress) { ++i; continue; }
        if (body[i] == '&') {
            struct entity { const char *name; char value; } entities[] = {
                { "amp;", '&' }, { "lt;", '<' }, { "gt;", '>' },
                { "quot;", '"' }, { "apos;", '\'' }, { "nbsp;", ' ' },
            };
            bool found = false;
            for (size_t e = 0; e < sizeof(entities) / sizeof(entities[0]); ++e)
                if (!strncmp(body + i + 1, entities[e].name,
                             strlen(entities[e].name))) {
                    out[(*used)++] = entities[e].value;
                    i += strlen(entities[e].name) + 1; found = true; break;
                }
            if (found) continue;
        }
        size_t bytes = 1;
        uint32_t cp = utf8_codepoint((const unsigned char *)body + i, &bytes);
        i += bytes;
        unsigned char c = windows_1252(cp);
        if (isspace(c)) append_space(out, used);
        else if (c >= 32) out[(*used)++] = (char)c;
    }
    if (body[i])
        document->truncated = true;
    append_space(out, used);
}

esp_err_t epub_load_document(const char *path, epub_document_t *document)
{
    memset(document, 0, sizeof(*document));
    zip_file_t zip;
    if (!zip_open(&zip, path)) return ESP_ERR_INVALID_RESPONSE;
    size_t length;
    char *container = zip_extract(&zip, "META-INF/container.xml", &length);
    if (!container) { fclose(zip.file); return ESP_ERR_NOT_FOUND; }
    char opf_path[384];
    const char *rootfile = strstr(container, "<rootfile");
    bool found = rootfile
        && attribute(rootfile, "full-path", opf_path, sizeof(opf_path));
    free(container);
    if (!found) { fclose(zip.file); return ESP_ERR_INVALID_RESPONSE; }
    char *opf = zip_extract(&zip, opf_path, &length);
    if (!opf) { fclose(zip.file); return ESP_ERR_NOT_FOUND; }

    const char *title = strstr(opf, "<dc:title");
    if (title && (title = strchr(title, '>'))) {
        const char *end = strstr(++title, "</dc:title>");
        size_t n = end ? (size_t)(end - title) : 0;
        if (n >= sizeof(document->title)) n = sizeof(document->title) - 1;
        memcpy(document->title, title, n); document->title[n] = 0;
    }
    if (!document->title[0]) strlcpy(document->title, path, sizeof(document->title));

    typedef struct { char id[96]; char href[384]; } manifest_item_t;
    manifest_item_t *items = heap_caps_calloc(256, sizeof(*items),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!items) { free(opf); fclose(zip.file); return ESP_ERR_NO_MEM; }
    size_t item_count = 0;
    for (const char *p = opf; item_count < 256 && (p = strstr(p, "<item "));) {
        const char *end = strchr(p, '>');
        if (!end) break;
        if (attribute(p, "id", items[item_count].id,
                      sizeof(items[item_count].id))
            && attribute(p, "href", items[item_count].href,
                         sizeof(items[item_count].href))) ++item_count;
        p = end + 1;
    }

    char *book = heap_caps_malloc(MAX_BOOK_TEXT,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    document->sections = heap_caps_calloc(EPUB_MAX_SECTIONS,
        sizeof(*document->sections), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    document->markers = heap_caps_calloc(EPUB_MAX_MARKERS,
        sizeof(*document->markers), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!book || !document->sections || !document->markers) {
        free(book); free(document->sections); free(document->markers);
        document->sections = NULL; document->markers = NULL;
        free(items); free(opf); fclose(zip.file); return ESP_ERR_NO_MEM;
    }
    size_t used = 0;
    char directory[384]; base_dir(opf_path, directory, sizeof(directory));
    for (const char *p = opf; (p = strstr(p, "<itemref "));) {
        char idref[96] = { 0 };
        if (attribute(p, "idref", idref, sizeof(idref))) {
            for (size_t i = 0; i < item_count; ++i) if (!strcmp(idref, items[i].id)) {
                char entry[768];
                snprintf(entry, sizeof(entry), "%s%s", directory, items[i].href);
                size_t html_length;
                char *html = zip_extract(&zip, entry, &html_length);
                if (html) {
                    size_t section_start = used;
                    html_to_text(html, book, &used, document);
                    while (used > section_start && book[used - 1] == ' ')
                        --used;
                    if (used > section_start
                        && document->section_count < EPUB_MAX_SECTIONS) {
                        epub_section_t *section =
                            &document->sections[document->section_count++];
                        section->text_offset = section_start;
                        extract_heading(html, section->name,
                                        sizeof(section->name));
                        append_space(book, &used);
                    }
                    free(html);
                }
                break;
            }
        }
        ++p;
        if (document->truncated)
            break;
    }
    free(items); free(opf); fclose(zip.file);
    while (used && book[used - 1] == ' ') --used;
    book[used] = 0;
    if (!used) {
        free(book); free(document->sections); free(document->markers);
        document->sections = NULL; document->markers = NULL;
        return ESP_ERR_INVALID_RESPONSE;
    }
    document->text = book;
    document->text_length = used;
    if (!document->section_count) {
        document->section_count = 1;
        document->sections[0].text_length = used;
    } else {
        for (size_t i = 0; i < document->section_count; ++i) {
            size_t end = i + 1 < document->section_count
                ? document->sections[i + 1].text_offset : used;
            document->sections[i].text_length =
                end > document->sections[i].text_offset
                    ? end - document->sections[i].text_offset : 0;
        }
    }
    return ESP_OK;
}

void epub_free_document(epub_document_t *document)
{
    free(document->text);
    free(document->sections);
    free(document->markers);
    memset(document, 0, sizeof(*document));
}
