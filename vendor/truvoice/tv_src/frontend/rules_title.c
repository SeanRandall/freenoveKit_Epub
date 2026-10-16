/* Titles: title1-4 (Rule_Title1234) and the person-name lists (REFERENCE 3.4a). */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

static void bare(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
}
static void bare_lc(const TextProc *t, int j, char *out) { bare(t, j, out); fe_strlwr(out); }
static int starts_and(const char *w) { return !strncmp(w, "and", 3) || !strncmp(w, "or", 2) || !strncmp(w, "&", 1); }

/* The next word is a known surname or first name (lists by initial letter). @0x100586c0 */
void fe_IsPersonName(TextProc *t, short i, short *res)
{
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char tag[64], first[2];
    short k;

    if (t->cur_word < i + 1) return;
    strcpy(tag, "#");
    strncat(tag, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
    fe_strlwr(tag);
    strcat(tag, "#");
    first[0] = 0;
    strncat(first, tag + 1, 1);
    k = fe_strpos(alphabet, first);
    if (k < 1 || k > 26) return;
    for (const char *const *l = fe_person_names[k - 1]; *l; l++)
        if (fe_strpos(*l, tag)) { *res = 1; return; }
}

/* Titles before names. @0x1004c100 */
void fe_Rule_Title1234(TextProc *t, short i, short *res)
{
    static const char always[] = "#sgt#ssgt#msgt#tsgt#comdr#cmdr#lieut#radm#bros#sens#capt#messrs#esq#pvt#";
    static const char after_name[] = "#ens#capt#esq#";
    static const char with_period[] = "#mr#mrs#mt#";
    static const char before_name[] = "#mr#mrs#messrs#dr#gen#cpl#pvt#lt#prof#adm#mt#";
    static const char not_col[] =
        "#president#ambassador#minister#peso#andes#govt#government#natl#national#gov#governor#state#rockies#sen#"
        "senator#rep#representative#legistlature#";
    short ok = *res;
    int j = i + 1, nextcap;
    char tag[64], w[128];

    if (t->auc[i]) goto done;

    /* a two-word key ("mr and mrs"): the expansion replaces the next word */
    if (fe_strpos(t->rec_keyword, " ")) {
        if (t->cur_word < i + 1) goto done;
        strcpy(w, t->word_lc);
        strcat(w, " ");
        strncat(w, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        fe_strlwr(w);
        if (strcmp(t->rec_keyword, w) != 0) goto done;
        strcpy(w, t->rec_expansion);
        strcpy(t->rec_expansion, "");
        if (strlen(w) < 30) {
            strcpy(t->word[i + 1], w);   /* in place, as the original (it may overrun the old word) */
        } else {
            fe_free(t->word[i + 1]);
            t->word[i + 1] = fe_strdup(w);
        }
        goto accept;
    }

    strcpy(tag, "#");
    strcat(tag, t->word_lc);
    strcat(tag, "#");
    if (fe_strpos(always, tag)) goto accept;
    if (strcmp(t->punc[i], ".") == 0 && fe_strpos(with_period, tag)) goto accept;
    if (j <= t->cur_word) {
        fe_IsPersonName(t, i, &ok);
        if (ok) goto done;
    }
    /* "Mr. and Mrs. Smith" */
    if (!strcmp(t->word_lc, "mr") && i + 2 <= t->cur_word && !strncmp(t->qual[i + 2], "title", 5) &&
        starts_and(t->word[i + 1]))
        goto accept;
    if (!strcmp(t->word_lc, "mrs") && i - 2 >= 1 && !strncmp(t->qual[i - 2], "title", 5) && starts_and(t->word[i - 1]))
        goto accept;

    nextcap = j <= t->cur_word && t->iuc[i + 1];
    if (fe_strpos(after_name, tag)) {
        if (t->iuc[i] || nextcap) goto accept;
        /* "John Smith Esq" */
        if (!(strcmp(t->word_lc, "esq") != 0 || i - 1 < 1 || t->iuc[i - 1] == 0)) goto accept;
    }

    if (fe_strpos(before_name, tag) && t->iuc[i] && nextcap) {
        /* Mr, Dr, Gen, Lt, Prof, Adm ... before a capitalised word, with exceptions */
        if (!strcmp(t->word_lc, "dr") && i - 1 > 0 && t->iuc[i - 1]) goto done;
        strcpy(w, "");
        if (j <= t->cur_word) bare(t, i + 1, w);
        fe_strlwr(w);
        if (!strcmp(t->word_lc, "lt")) {
            if (!strcmp(w, "inf") || !strcmp(w, "infantry")) goto done;
            if (t->iuc[i + 1] && !strcmp(w, "b")) goto done;
        }
        if (!strcmp(t->word_lc, "prof") && (!strcmp(w, "eng") || !strcmp(w, "engineer"))) goto done;
        if (!strcmp(t->word_lc, "adm") && (!strcmp(w, "assistant") || !strcmp(w, "asst") || !strcmp(w, "ass't") ||
                                           !strcmp(w, "office")))
            goto done;
        w[0] = 0;
        if (i - 2 > 0) {
            bare(t, i - 2, w);
            strcat(w, " ");
            strncat(w, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
            fe_strlwr(w);
        }
        if (strcmp(t->word_lc, "adm") != 0) goto accept;
        if (!strcmp(w, "board of") || !strcmp(w, "bd of")) goto done;
        goto accept;
    }

    /* Ms, Msgr at the start of a sentence */
    if ((!strcmp(t->word_lc, "ms") || !strcmp(t->word_lc, "msgr")) && t->fper[i] && t->iuc[i] && nextcap) goto accept;
    {
        /* political context: a state before, a state or a party within four words after */
        int state_before = i - 1 > 0 && t->stprov[i - 1];
        int state_after = 0, party = 0, no_rep_before = 1;
        for (short s = 1; s < 5; s++)
            if (i + s <= t->cur_word && t->stprov[i + s]) { state_after = 1; break; }
        for (short s = 1; s < 5; s++) {
            if (t->cur_word < i + s) break;
            bare_lc(t, i + s, w);
            if (!strcmp(w, "republican") || !strcmp(w, "democrat") || !strcmp(w, "dem") || !strcmp(w, "democratic")) {
                party = 1;
                break;
            }
        }
        if ((!strcmp(t->word_lc, "sen") || !strcmp(t->word_lc, "gov") || !strcmp(t->word_lc, "govs")) && t->iuc[i] &&
            nextcap && (state_before || state_after || party)) {
            if (strcmp(t->word_lc, "gov") != 0) goto accept;
            if (strncmp(t->word[i], ".", 1) == 0) goto done;
            goto accept;
        }
        for (short s = 1; s < 7; s++) {
            if (i - s > 0) {
                bare_lc(t, i - s, w);
                if (!strcmp(w, "rep")) { no_rep_before = 0; break; }
            }
        }
        if ((!strcmp(t->word_lc, "rep") || !strcmp(t->word_lc, "reps")) && no_rep_before && t->fper[i] && t->iuc[i] &&
            nextcap && (state_before || state_after || party))
            goto accept;
    }
    /* Maj: "Senate Maj Leader" -> Majority */
    if (!strcmp(t->word_lc, "maj") && t->iuc[i] && nextcap) {
        if (i - 1 >= 1) {
            bare_lc(t, i - 1, w);
            if (!strcmp(w, "rep") || !strcmp(w, "republican") || !strcmp(w, "dem") || !strcmp(w, "democratic") ||
                !strcmp(w, "sen") || !strcmp(w, "senate") || !strcmp(w, "house") || !strcmp(w, "assembly")) {
                strcpy(t->rec_expansion, "Majority");
                goto accept;
            }
        }
        if (!(t->cur_word < j)) {
            bare_lc(t, i + 1, w);
            if (strcmp(w, "leader") != 0 && strcmp(w, "whip") != 0) goto accept;
            strcpy(t->rec_expansion, "Majority");
            goto accept;
        }
    }
    if (strcmp(t->word_lc, "maj") != 0) {
        /* Col before a name, but not "Col B" or "Col President" */
        if (t->cur_word < j) goto done;
        if (strcmp(t->word_lc, "col") != 0 || t->iuc[i] == 0 || !nextcap) goto done;
        bare(t, i + 1, w);
        if (strlen(w) == 1 && strcmp(w, "A") >= 0 && strcmp(w, "Z") <= 0) goto done;
        strcpy(tag, "#");
        strcat(tag, w);
        fe_strlwr(tag);
        strcat(tag, "#");
        if (fe_strpos(not_col, tag)) goto done;
    }
accept:
    ok = 1;
done:
    *res = ok;
}
