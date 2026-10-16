/* direction2: two-letter compass points (NE, SW, ...) (REFERENCE 3.4a). */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

/* word[j] without its punctuation, lower case */
static void bare_lc(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
    fe_strlwr(out);
}
static int is_state_name(const char *tag)
{
    return fe_strpos(fe_state_names[0], tag) || fe_strpos(fe_state_names[1], tag) || fe_strpos(fe_state_names[2], tag);
}

/* Accept a compass point after a street word ("Main St NE"), before an ordinal street ("NE 5th St"), before a
   state name or a capitalised state/province word, and in "NW Territories/Passage", "SE Asia", "SW Africa".
   Written label for label after the original. @0x1006ebc0 */
void fe_Rule_Direction2(TextProc *t, short i, short *res)
{
    short ok = *res;
    char w2[60], n1[60] = "", n2[60] = "", p1[60] = "", tag[128];

    if (strlen(t->word_lc) == 1) goto done;
    if (t->auc[i] == 0 && strcmp(t->word_lc, t->word_as_typed) != 0) goto done;
    fe_str_replace_all(t->word_lc, ".", "", w2);
    if (i + 1 <= t->cur_word) {
        bare_lc(t, i + 1, n1);
        if (i + 2 <= t->cur_word) bare_lc(t, i + 2, n2);
    }
    if (i - 1 > 0) bare_lc(t, i - 1, p1);

    /* "Main St NE" */
    if (i - 1 >= 1) {
        if (!strcmp(t->qual[i - 1], "street") || !strcmp(p1, "street") || !strcmp(p1, "st") || !strcmp(p1, "dr"))
            goto accept;
    }
    /* "5 NE Main": a number before and a capital after is enough */
    if (!(i - 1 < 1 || t->num[i - 1] == 0 || t->cur_word < i + 1 || (t->iuc[i + 1] == 0 && t->auc[i + 1] == 0)))
        goto accept;
    if (t->cur_word < i + 1) goto l_2af;

    /* "NE 5th St": the next word must have QUAL "ordnl" (5th, 42nd, fifth). The original also tests the word's
       last two letters against st/nd/rd/th, but copies them with strncpy into a stack buffer it never
       terminates, so the test never matches (DLL: "10 NW and", "SW Smith St", "NE 5TH St" rejected). Left out,
       in both modes: it adds nothing to the ordnl test, and a working suffix test would also take "Smith". */
    if (strcmp(t->qual[i + 1], "ordnl") != 0) goto l_2af;
    if (i - 1 < 1 || t->num[i - 1] == 0) {
        if (t->cur_word < i + 2) goto l_265;
        if (strcmp(t->qual[i + 2], "street") && strcmp(n2, "street") && strcmp(n2, "st") && strcmp(n2, "dr"))
            goto l_265;
    }
    goto accept;
l_265:
    if (strcmp(t->punc[i + 1], "") == 0 && t->cur_word - i != 1) goto l_2af;
    goto accept;

l_2af:
    if (!(i - 1 < 1 || t->num[i - 1] == 0)) {
        /* "5 NE St" */
        if (!(t->cur_word < i + 1)) {
            if (!strcmp(t->qual[i + 1], "street") || !strcmp(n1, "street")) goto accept;
        }
        if (t->cur_word < i + 2) goto l_3c7;
        if (!strcmp(t->qual[i + 2], "street") || !strcmp(n2, "street")) goto accept;
    }
l_3c7:
    /* "NE Kansas", "SW Ont" */
    if (strcmp(w2, "ne") == 0 || t->cur_word < i + 1) goto l_6d9;
    strcpy(tag, "#");
    strcat(tag, n1);
    strcat(tag, "#");
    if (is_state_name(tag)) goto accept;
    if ((t->iuc[i + 1] || t->auc[i + 1]) && t->stprov[i + 1] &&
        (int)strlen(t->word[i + 1]) - (int)strlen(t->punc[i + 1]) > 2)
        goto accept;
    if (t->cur_word < i + 2) goto l_6d9;
    /* two-word state names: "SE North Carolina" */
    strcpy(tag, "#");
    strcat(tag, n1);
    strcat(tag, " ");
    strncat(tag, t->word[i + 2], strlen(t->word[i + 2]) - strlen(t->punc[i + 2]));
    strcat(tag, "#");
    fe_strlwr(tag);
    if (is_state_name(tag)) goto accept;

l_6d9:
    if (strcmp(w2, "nw") == 0) {
        if (!strcmp(n1, "terr") || !strcmp(n1, "territories") || !strcmp(n1, "passage")) goto accept;
    }
    if (strcmp(w2, "se") == 0 && strcmp(n1, "asia") == 0) goto accept;
    if (strcmp(w2, "sw") != 0 || strcmp(n1, "africa") != 0) goto done;

accept:
    ok = 1;
done:
    *res = ok;
}
