/* range: page / chapter / figure / part references and their ranges (REFERENCE 3.4a). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

/* What kind of item word[j] is: "num", "rom" (Roman numeral), "letr" (one letter, or a letter and a comma);
   out is left unchanged otherwise. @0x1004bed0 */
void fe_Range_ItemCheck(TextProc *t, short j, char *out)
{
    char c[4];
    size_t n;

    if (t->cur_word < j) return;
    if (t->num[j]) { strcpy(out, "num"); return; }
    if (strcmp(t->qual[j], "roman") == 0) { strcpy(out, "rom"); return; }
    strcpy(c, "");
    strncat(c, t->word[j], 1);
    fe_strlwr(c);
    n = strlen(t->word[j]);
    if (n != 1 && (n != 2 || strcmp(t->word[j] + 1, ",") != 0)) return;
    if (strcmp(c, "a") >= 0 && strcmp(c, "z") <= 0) strcpy(out, "letr");
}

/* Two items belong to one range when they are of the same kind; a one-character Roman numeral also pairs with a
   letter ("I" and "J"). wa / wb are the words whose length is tested (the original passes the wrong neighbour in
   two places; the callers reproduce that unless fe_fixes is set). */
static int same_kind(const TextProc *t, const char *a, int wa, const char *b, int wb)
{
    return strcmp(a, b) == 0 ||
           (strcmp(a, "rom") == 0 && strlen(t->word[wa]) == 1 && strcmp(b, "letr") == 0) ||
           (strcmp(b, "rom") == 0 && strlen(t->word[wb]) == 1 && strcmp(a, "letr") == 0);
}
static int is_conj(const char *w) { return !strcmp(w, "and") || !strcmp(w, "or") || !strcmp(w, "to") || !strcmp(w, "through"); }

/* @0x10046450 */
void fe_Rule_Range(TextProc *t, short i, short *res)
{
    short ok = *res;
    char a[8] = "", b[8] = "", c[8] = "", d[8] = "";
    const char *w = t->word_lc;

    /* forwards: "chap 5", "chap 5-7", "pts I and II", "chap 5, 6 and 7" */
    fe_Range_ItemCheck(t, i + 1, a);
    if (strcmp(a, "") != 0) {
        if (strcmp(w, "pt") != 0 && strcmp(w, "par") != 0 && strcmp(w, "art") != 0) ok = 1;
        if (t->cur_word < i + 2) goto back;
        fe_Range_ItemCheck(t, i + 2, b);
        if (!same_kind(t, a, i + 1, b, i + 2)) goto back;
        if (strcmp(t->sepr[i + 1], "-") == 0) goto accept;
        if (t->cur_word < i + 3) {
            if (is_conj(t->word[i + 2]) && i + 3 <= t->cur_word) fe_Range_ItemCheck(t, i + 3, b);
            if (strcmp(a, b) == 0) goto accept;
            goto back;
        }
        fe_Range_ItemCheck(t, i + 3, c);
        if (same_kind(t, a, i + 1, c, i + 3)) {
            if (i + 5 <= t->cur_word) fe_Range_ItemCheck(t, i + 5, d);
            if (i + 4 <= t->cur_word) {
                /* d is item i+5; the original tests the length of word i+4 (fixed with fe_fixes) */
                if (!same_kind(t, a, i + 1, d, fe_fixes ? i + 5 : i + 4)) goto back;
                if (strcmp(t->word[i + 4], "and") == 0 || strcmp(t->word[i + 4], "or") == 0) goto accept;
            }
            goto back;
        }
        if (strcmp(t->word[i + 3], "and") != 0 && strcmp(t->word[i + 3], "or") != 0) goto back;
        if (i + 4 <= t->cur_word) fe_Range_ItemCheck(t, i + 4, c);
        /* c is now item i+4; the original tests the length of word i+3 (fixed with fe_fixes) */
        if (same_kind(t, a, i + 1, c, fe_fixes ? i + 4 : i + 3)) goto accept;
        goto back;
    }

back:
    /* backwards: "5 p", "pp 5-7", "3rd div", "5th and 6th gr" */
    if (ok) goto done;
    if (!strcmp(w, "p") || !strcmp(w, "pg") || !strcmp(w, "pp") || !strcmp(w, "fig") || !strcmp(w, "vol") ||
        !strcmp(w, "iss") || strncmp(w, "chap", 4) == 0) {
        if (i - 1 >= 1 && t->num[i - 1]) {
            ok = 1;
            if (strcmp(t->sepr[i - 1], "-") == 0) goto done;
            if ((short)atoi(t->word[i - 1]) >= 2) goto accept;
            if (i - 2 > 0) {
                if (t->num[i - 2] && strcmp(t->sepr[i - 2], "-") == 0) goto accept;
                if (i - 3 > 0 && t->num[i - 3] && is_conj(t->word[i - 2])) goto accept;
            }
        }
    }
    if (i - 1 < 1 || strcmp(t->qual[i - 1], "ordnl") != 0) goto done;
    if (strcmp(w, "gr") != 0 && strcmp(w, "div") != 0 && strcmp(w, "vol") != 0 && strncmp(w, "chap", 4) != 0 &&
        strcmp(w, "iss") != 0 && strcmp(w, "pg") != 0 && strcmp(w, "amend") != 0)
        goto done;
    ok = 1;
    if (i - 2 > 0 && strcmp(t->qual[i - 2], "ordnl") == 0 && strcmp(t->sepr[i - 2], "-") == 0) goto accept;
    if (i - 3 < 1 || strcmp(t->qual[i - 3], "ordnl") != 0 || strcmp(t->word[i - 2], "and") != 0) goto done;

accept:
    /* a range: the plural */
    ok = 1;
    if (strcmp(fe_tail(t->rec_expansion, 1), "s") != 0) strcat(t->rec_expansion, "s");
done:
    *res = ok;
}
