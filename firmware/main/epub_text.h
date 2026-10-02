#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define EPUB_MAX_SECTIONS 512
#define EPUB_MAX_MARKERS 1024

typedef enum {
    EPUB_MARKER_HEADING,
} epub_marker_type_t;

typedef struct {
    epub_marker_type_t type;
    uint8_t level;
    char name[96];
    size_t text_offset;
} epub_marker_t;

typedef struct {
    char name[96];
    size_t text_offset;
    size_t text_length;
    uint32_t word_count;
} epub_section_t;

typedef struct {
    char title[192];
    char cache_text_path[512];
    char *text;
    size_t text_length;
    epub_section_t *sections;
    size_t section_count;
    epub_marker_t *markers;
    size_t marker_count;
    uint64_t word_count;
    size_t focus_section;
    bool cache_complete;
    bool index_rebuilt;
    bool truncated;
} epub_document_t;

/* Loads the readable spine of an unencrypted EPUB into PSRAM. */
esp_err_t epub_load_document(const char *path, epub_document_t *document);
/* Prioritises the resumed spine while creating/refreshing the SD cache. */
esp_err_t epub_load_document_at(const char *path, size_t focus_section,
                                epub_document_t *document);
bool epub_read_text(const epub_document_t *document, size_t offset,
                    char *destination, size_t length);
void epub_free_document(epub_document_t *document);
