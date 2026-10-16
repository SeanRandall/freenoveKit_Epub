/* The short rule handlers (REFERENCE 3.4a). Each gets the word index and the accept flag (0 on entry). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

/* qual1: a number (or $n) before and a measure after ("5 sq ft"), or a number two back and a measure before.
   @0x100531a0 */
void fe_Rule_Qual1(TextProc *t, short i, short *res)
{
    short ok = *res;

    if (i - 1 >= 1 && i + 1 <= t->cur_word) {
        if (t->num[i - 1] || t->psnum[i - 1]) {
            if (strncmp(t->qual[i + 1], "measure", 7) == 0) goto accept;
        }
        if (strncmp(t->word[i - 1], "$", 1) == 0 && atoi(t->word[i - 1] + 1) != 0 &&
            strncmp(t->qual[i + 1], "measure", 7) == 0)
            goto accept;
    }
    if (i - 2 >= 1 && (t->num[i - 2] || t->psnum[i - 2]) && strncmp(t->qual[i - 1], "measure", 7) == 0)
        goto accept;
    goto done;
accept:
    ok = 1;
done:
    fe_Qual_PerSlash(t, i);
    *res = ok;
}

/* qual2: a number before and a measure after. @0x100532b0 */
void fe_Rule_Qual2(TextProc *t, short i, short *res)
{
    short ok = *res;

    if (i - 1 > 0 && i + 1 <= t->cur_word && (t->num[i - 1] || t->psnum[i - 1]) &&
        strncmp(t->qual[i + 1], "measure", 7) == 0)
        ok = 1;
    fe_Qual_PerSlash(t, i);
    *res = ok;
}

/* qual3: a number two back and a measure before ("5 ft cubed"). @0x10053330 */
void fe_Rule_Qual3(TextProc *t, short i, short *res)
{
    short ok = *res;

    if (i - 2 > 0 && (t->num[i - 2] || t->psnum[i - 2]) && strncmp(t->qual[i - 1], "measure", 7) == 0)
        ok = 1;
    fe_Qual_PerSlash(t, i);
    *res = ok;
}

/* After a measure (or a number or $n) followed by '/', the expansion is read "per ...". @0x10053680 */
void fe_Qual_PerSlash(TextProc *t, short i)
{
    char buf[60];

    if (i - 1 < 1) return;
    if (strncmp(t->qual[i - 1], "measure", 7) == 0 && strcmp(t->sepr[i - 1], "/") == 0) {
        strcpy(buf, "per ");
        strcat(buf, t->rec_expansion);
        strcpy(t->rec_expansion, buf);
    }
    if (t->num[i - 1] == 0 && (strncmp(t->word[i - 1], "$", 1) != 0 || atoi(t->word[i - 1] + 1) == 0))
        return;
    if (strcmp(t->sepr[i - 1], "/") == 0) {
        strcpy(buf, "per ");
        strcat(buf, t->rec_expansion);
        strcpy(t->rec_expansion, buf);
    }
}

/* fname: the word starts with a capital. @0x10058690 */
void fe_Rule_Fname(TextProc *t, short i, short *res)
{
    if (t->iuc[i]) *res = 1;
}

/* direction3: the word is all capitals or all lower case. @0x1006f8b0 */
void fe_Rule_Direction3(TextProc *t, short i, short *res)
{
    if (t->auc[i] || strcmp(t->word_lc, t->word_as_typed) == 0) *res = 1;
}

/* bcad: a number before and bc / b.c / ad / a.d. @0x10058570 */
void fe_Rule_Bcad(TextProc *t, short i, short *res)
{
    const char *w = t->word_lc;

    if (strlen(w) == 1) return;
    if (i - 1 > 0 && t->num[i - 1] &&
        (!strcmp(w, "bc") || !strcmp(w, "b.c") || !strcmp(w, "ad") || !strcmp(w, "a.d")))
        *res = 1;
}

/* measure3: a number before and the word typed exactly as the record key (MB vs mB). @0x10047ff0 */
void fe_Rule_Measure3(TextProc *t, short i, short *res)
{
    short ok = *res;

    if (i - 1 > 0) {
        if ((t->num[i - 1] || t->psnum[i - 1]) && strcmp(t->word_as_typed, t->rec_keyword) == 0)
            fe_Measure_Accept(t, i, &ok);
        fe_Measure_Finalize(t, i, &ok);
    }
    *res = ok;
}

/* measure5: a number follows ("p 5" -> page 5). @0x100483d0 */
void fe_Rule_Measure5(TextProc *t, short i, short *res)
{
    short ok = *res;

    if (i + 1 <= t->cur_word) {
        if (t->num[i + 1]) {
            ok = 1;
            strcpy(t->measure_sfx, "1");
        }
        fe_Measure_Finalize(t, i, &ok);
    }
    *res = ok;
}

static int is_numeric(const TextProc *t, int j) { return t->num[j] || t->psnum[j]; }

/* qual4 (C, F, K): after "n <measure>" or "n deg"; C/F/K themselves only right after "deg"; and the word must
   start with the same letter, in the same case, as its expansion (C = Celsius, not c). @0x100533a0 */
void fe_Rule_Qual4(TextProc *t, short i, short *res)
{
    short ok = *res;
    int hit = 0;

    if (i - 1 > 0 && i + 1 <= t->cur_word && is_numeric(t, i - 1) && strncmp(t->qual[i + 1], "measure", 7) == 0)
        hit = 1;
    if (i - 2 > 0 && is_numeric(t, i - 2) &&
        (strncmp(t->qual[i - 1], "measure", 7) == 0 || strncmp(t->word[i - 1], "deg", 3) == 0))
        hit = 1;
    if (i - 3 > 0 && is_numeric(t, i - 3) && strncmp(t->word[i - 1], "deg", 3) == 0 &&
        (!strcmp(t->qual[i - 2], "premod") || !strcmp(t->qual[i - 2], "ppmod") || !strcmp(t->qual[i - 2], "numod")))
        hit = 1;
    if (hit) {
        if (!strcmp(t->word_lc, "k") || !strcmp(t->word_lc, "c") || !strcmp(t->word_lc, "f"))
            hit = i - 1 > 0 && (strncmp(t->word[i - 1], "deg", 3) == 0 || strncmp(t->word[i - 1], "Deg", 3) == 0);
        if (hit && strncmp(t->word_as_typed, t->rec_expansion, 1) == 0)
            ok = 1;
    }
    fe_Qual_PerSlash(t, i);
    *res = ok;
}

/* Time zones: after a time ("5:30 PST", "5 pm PST"); lower-case "est" never; "a hst" after a time too.
   @0x10057930 */
void fe_Rule_Time(TextProc *t, short i, short *res)
{
    char w[60];

    if (strlen(t->word_lc) == 1) return;
    strcpy(w, t->word[i]);
    fe_strlwr(w);
    if (strcmp(w, "est") == 0 && t->auc[i] == 0) return;
    if (i - 1 < 1) return;
    if (strcmp(t->qual[i - 1], "timenum") == 0) goto accept;
    if (i - 2 > 0 && strcmp(t->qual[i - 2], "timenum") == 0 && strcmp(t->qual[i - 1], "ampm") == 0) goto accept;
    if (strcmp(w, "hst") != 0) return;
    if (strcmp(t->word[i - 1], "A") != 0 && strcmp(t->word[i - 1], "a") != 0) return;
    if (i - 2 < 1) return;
    if (strcmp(t->qual[i - 2], "timenum") == 0) goto accept;
    if (i - 3 < 1 || strcmp(t->qual[i - 3], "timenum") != 0 || strcmp(t->qual[i - 2], "ampm") != 0) return;
accept:
    *res = 1;
}
