/*
 * TruVoice's user dictionary (REFERENCE 10.6): the file of words and their
 * phoneme spellings that fills the engine's user lexicon (g_lexicon, which
 * UserLex_Try reads and UserLex_Add changes; engine/lexicon.c).
 *
 * The file is a word, a NUL, its spelling, a NUL, and so on (at most 5000
 * entries).  A word is letters only and a spelling uses the phoneme letters
 * (tts_Phoneme's), each shorter than 40 characters; other entries are left
 * out, with a warning the first time.  The words are kept in capitals and
 * sorted, for the lexicon's binary search.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "crt.h"
#include "truvoice.h"

#define USERDIC_MAX 5000
#define USERDIC_LEN 40      /* a word or spelling is shorter */
#define FIELD_MAX   100     /* what one read takes */

/* The letters a spelling may use: A-Z, a-z, 1-5 and @ | ~ & (the
 * pronunciation manager checks them too). */
/* @0x100bc6a8 */ extern const char g_phoneme_chars[];
/* Set when the lexicon changes (UserLex_Add, a deletion) */
/* @0x100bc67c */ extern int32_t g_lexicon_dirty;

/* The pronunciation manager's help-context pairs and the phoneme examples
 * of its Examples dialog, in the DLL's data. */
/* @0x100bc6e8 */ extern const uint32_t g_userdic_help_ids[];
/* @0x100bc758 */ extern const char g_userdic_examples[];

void (*tv_userdic_warn)(const char *path);

const char *tv_userdic_phoneme_chars(void)
{
    return g_phoneme_chars;
}

const uint32_t *tv_userdic_help(void)
{
    return g_userdic_help_ids;
}

const char *tv_userdic_example(uint32_t va)
{
    return g_userdic_examples + (va - 0x100bc758u);
}

/* fscanf "%100[^\0]" (the DLL's format string ends at the NUL, but MSVC's
 * scanset reader runs on to the "]" after it): up to 100 bytes other than
 * NUL.  -1 at the end of the file before any, 0 if none (out unchanged). */
static int read_field(FILE *f, char *out)
{
    int c, n = 0;

    c = getc(f);
    if (c == EOF)
        return -1;
    while (c != EOF && c != 0 && n < FIELD_MAX) {
        out[n++] = (char)c;
        c = getc(f);
    }
    if (c != EOF)
        ungetc(c, f);
    if (n == 0)
        return 0;
    out[n] = '\0';
    return 1;
}

/* fscanf "%c" until a NUL has been read: 0, or -1 at the end of the file. */
static int skip_to_nul(FILE *f)
{
    int c;

    do {
        c = getc(f);
        if (c == EOF)
            return -1;
    } while (c != 0);
    return 0;
}

/* One entry: 1, 0 if it is not a valid one, -1 at the end of the file.
 * An empty field keeps what the buffer held (in the DLL, the stack). */
/* @0x10025580 */
static int read_entry(FILE *f, char word[FIELD_MAX + 4], char pron[FIELD_MAX + 4])
{
    size_t i, n;

    if (read_field(f, word) < 0 || skip_to_nul(f) < 0)
        return -1;
    if (read_field(f, pron) < 0 || skip_to_nul(f) < 0)
        return -1;
    if (strlen(word) >= USERDIC_LEN || strlen(pron) >= USERDIC_LEN)
        return 0;
    n = strlen(word);
    for (i = 0; i < n; i++)
        if (!tv_isalpha(word[i]))
            return 0;
    n = strlen(pron);
    for (i = 0; i < n; i++)
        if (tv_strchr(g_phoneme_chars, pron[i]) == NULL)
            return 0;
    return 1;
}

static char *dup_str(const char *s)
{
    char *p = (char *)tv_new(strlen(s) + 1);

    if (p != NULL)
        strcpy(p, s);
    return p;
}

/* The entries of path replace the lexicon's (the old ones are not freed:
 * callers clear it first).  0 if the file cannot be opened, else 1. */
/* @0x100257c0 */
int tv_userdic_load(const char *path)
{
    char word[FIELD_MAX + 4] = "", pron[FIELD_MAX + 4] = "";
    int warned = 0, r;
    FILE *f = fopen(path, "r");

    if (f == NULL)
        return 0;
    g_lexicon_count = 0;
    do {
        while ((r = read_entry(f, word, pron)) == 0) {
            if (!warned) {
                warned = 1;
                if (tv_userdic_warn != NULL)
                    tv_userdic_warn(path);
            }
        }
        if (r < 0)
            break;
        g_lexicon[g_lexicon_count].word = tv_strupr(dup_str(word));
        g_lexicon[g_lexicon_count].pron = dup_str(pron);
        g_lexicon_count++;
    } while (g_lexicon_count < USERDIC_MAX);
    tv_qsort(g_lexicon, g_lexicon_count, sizeof(LexEntry), UserLex_Compare);
    fclose(f);
    return 1;
}

/* UserDic_Clear 10025a00 */
void tv_userdic_clear(void)
{
    uint32_t i;

    for (i = 0; i < g_lexicon_count; i++) {
        tv_delete((void *)g_lexicon[i].word);
        tv_delete((void *)g_lexicon[i].pron);
    }
    g_lexicon_count = 0;
}

/* The lexicon again from path.  @0x10025990 */
void tv_userdic_reload(const char *path)
{
    Lexicon_Lock();
    tv_userdic_clear();
    tv_userdic_load(path);
    Lexicon_Unlock();
}

/* The index of word (any case) in the lexicon, -1 if it is not there, -2
 * when out of memory.  @0x100252c0 */
int tv_userdic_find(const char *word)
{
    char *w = dup_str(word);
    const LexEntry *hit;

    if (w == NULL)
        return -2;
    tv_strupr(w);
    Lexicon_Lock();
    hit = (const LexEntry *)tv_bsearch(&w, g_lexicon, g_lexicon_count, sizeof(LexEntry), UserLex_Compare);
    Lexicon_Unlock();
    tv_delete(w);
    return hit != NULL ? (int)(hit - g_lexicon) : -1;
}

/* @0x100254f0 */
void tv_userdic_delete(const char *word)
{
    int i;

    Lexicon_Lock();
    i = tv_userdic_find(word);
    if (i >= 0) {
        tv_delete((void *)g_lexicon[i].word);
        tv_delete((void *)g_lexicon[i].pron);
        g_lexicon_count--;
        memmove(&g_lexicon[i], &g_lexicon[i + 1], (g_lexicon_count - (uint32_t)i) * sizeof(LexEntry));
        g_lexicon_dirty = 1;
    }
    Lexicon_Unlock();
}

/* The file part of the DLL's save (in UserDic_Save 100273a0): every entry,
 * each string with its NUL.  0, or -1 if the file cannot be made. */
int tv_userdic_save(const char *path)
{
    FILE *f = fopen(path, "wb");
    uint32_t i;

    if (f == NULL)
        return -1;
    Lexicon_Lock();
    for (i = 0; i < g_lexicon_count; i++) {
        fwrite(g_lexicon[i].word, 1, strlen(g_lexicon[i].word) + 1, f);
        fwrite(g_lexicon[i].pron, 1, strlen(g_lexicon[i].pron) + 1, f);
    }
    Lexicon_Unlock();
    fclose(f);
    return 0;
}
