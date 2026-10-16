/* city: abbreviated city names, accepted only before their state (REFERENCE 3.4a). */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

/* Rows of the city-state table for the state in u_2bd5 (by its first letter). @0x100402f0 */
void fe_CityState_LetterRange(TextProc *t, short *start, short *end)
{
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{";
    char c[2];
    short k;

    c[0] = t->u_2bd5[0];
    c[1] = 0;
    fe_strlwr(c);
    k = fe_strpos(letters, c);
    if (k > 52) k = 0;
    if (k < 1) {
        *start = 0;
        *end = 0;
    } else {
        *start = fe_city_letter_end[k - 1] + 1;
        *end = fe_city_letter_end[k];
    }
}

static void bare(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
}

/* Which state word[j] names, from its QUAL and the fixed tables of ambiguous abbreviations. */
static void state_of(const TextProc *t, int j, char *st)
{
    static const char abbr4[] = "que ont col ind la  oklany  n.y washsd  ";
    static const char full4[] =
        "Quebec      Ontario     Colorado    Indiana     Louisiana   Oklahoma    New York    New York    Washington  "
        "South Dakota";
    static const char abbr2[] = "ca co ct in mo fl mi mt ne ut ";
    static const char full2[] = "California Colorado   ConnecticutIndiana    Missouri   Florida    Michigan   Montana    "
                                "Nebraska   Utah       ";
    char w[64];
    short p;

    strcpy(st, t->qual[j]);
    if (!strcmp(st, "bcad")) strcpy(st, "British Columbia");
    if (!strcmp(st, "city")) {
        bare(t, j, w);
        fe_strlwr(w);
        p = fe_strpos(abbr4, w);
        if (p) {
            strcpy(st, "");
            strncat(st, full4 + ((p - 1) / 4) * 12, 12);   /* (p - 1) / 4 rounds toward zero, as the original */
            strcpy(st, fe_str_trim_spaces(st));
        }
    }
    strcpy(w, t->word[j]);
    fe_strlwr(w);
    if (strlen(w) == 2) {
        p = fe_strpos(abbr2, w);
        if (p) {
            strcpy(st, "");
            strncat(st, full2 + ((p - 1) / 3) * 11, 11);
            strcpy(st, fe_str_trim_spaces(st));
        }
    } else if (strlen(w) == 3 && !strcmp(w, "col")) {
        strcpy(st, "colorado");
    }
}

/* Is (state, city) in the table? The state is remembered in u_2bd5. */
static int in_table(TextProc *t, const char *st, const char *city)
{
    char key[64], exp[64];
    short start, end;

    strcpy(t->u_2bd5, st);
    fe_CityState_LetterRange(t, &start, &end);
    strcpy(key, st);
    fe_strlwr(key);
    strcpy(exp, city);
    fe_strlwr(exp);
    for (short k = start; k <= end; k++)
        if (!strcmp(fe_city_state[k].a, key) && !strcmp(fe_city_state[k].b, exp)) return 1;
    return 0;
}

/* Put an expansion into word[j] (in place when short, as the original), and empty rec_expansion. */
static void put_word(TextProc *t, int j, const char *s)
{
    if (strlen(s) < 30) {
        strcpy(t->word[j], s);
    } else {
        fe_free(t->word[j]);
        t->word[j] = fe_strdup(s);
    }
    strcpy(t->rec_expansion, "");
}

/* "Phila, PA", "St Pete FL": the key may span several words; the expansion replaces them. @0x1006bf30 */
void fe_Rule_City(TextProc *t, short i, short *res)
{
    char key[160], st[64], stl[64], nw[64], pun[64], buf[160];
    short n = 0, dots = 0, pos, p;
    int j;

    if (!(i < t->cur_word)) goto reject;
    strcpy(key, t->word_lc);
    /* how many words the key spans: spaces in the record key, less the dots in the word */
    for (pos = 0; (p = fe_strpos(t->rec_keyword + pos, " ")) != 0; pos += p) n++;
    for (pos = 0; (p = fe_strpos(t->word_lc + pos, ".")) != 0; pos += p) dots++;
    n = n - dots + 1;
    if (t->cur_word < i + n - 1) {
        n = 1;
    } else {
        for (short k = 1; k <= n - 1; k++) {
            strcat(key, " ");
            strncat(key, t->word[i + k], strlen(t->word[i + k]) - strlen(t->punc[i + k]));
            fe_strlwr(key);
        }
    }
    j = i + n;
    if (j <= t->cur_word) {
        state_of(t, j, st);
        strcpy(stl, st);
        fe_strlwr(stl);
        if ((n == 1 || !strcmp(key, t->rec_expansion)) && strcmp(t->qual[j], "") && strcmp(t->qual[j], stl) &&
            in_table(t, st, t->rec_expansion)) {
            if (n == 2) {
                strcpy(buf, t->rec_expansion);
                strcat(buf, t->punc[i + 1]);
                put_word(t, i + 1, buf);
            }
            if (n == 3) {
                strcpy(buf, t->rec_expansion);
                strcat(buf, t->punc[i + 2]);
                put_word(t, i + 2, buf);
                strcpy(t->word[i + 1], "");
            }
            goto accept;
        }
    }

    /* a two-word key, "word next": the next word loses its trailing punctuation, which follows the expansion */
    if (t->cur_word < i + 2) goto reject;
    strcpy(nw, t->word[i + 1]);
    fe_strlwr(nw);
    strcpy(pun, "");
    for (;;) {
        char lastc[8];
        strcpy(lastc, fe_tail(nw, 1));
        if (strcmp(lastc, ".") && strcmp(lastc, ",") && strcmp(lastc, "\"") && strcmp(lastc, "?") &&
            strcmp(lastc, "!") && strcmp(lastc, ":") && strcmp(lastc, ";"))
            break;
        strcpy(buf, "");
        strncat(buf, nw, strlen(nw) - 1);
        strcpy(nw, buf);
        strcat(pun, lastc);
    }
    strcpy(key, t->word_lc);
    strcat(key, " ");
    strcat(key, nw);
    if (strcmp(key, t->rec_keyword) != 0) goto reject;
    if (!strcmp(t->qual[i + 2], "")) goto reject;
    state_of(t, i + 2, st);
    if (!in_table(t, st, t->rec_expansion)) goto reject;
    if (!fe_fixes && strlen(pun) >= 4) {
        /* The original's punctuation buffer is 4 bytes, followed by the buffer that in_table last filled with the
           lower-case expansion: four or more marks lose their terminator and run on into it
           ("G Bay;!;," -> "Goose Bay,;!;goose bay"). fe_fixes keeps the marks only. */
        char e[64];
        strcpy(e, t->rec_expansion);
        fe_strlwr(e);
        pun[4] = 0;
        strcat(pun, e);
    }
    strcpy(buf, t->rec_expansion);
    strcat(buf, pun);
    put_word(t, i + 1, buf);
    goto accept;

reject:
    *res = 0;
    return;
accept:
    *res = 1;
}
