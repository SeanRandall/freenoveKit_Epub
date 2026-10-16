/* Measure_Finalize: which number a measure belongs to (REFERENCE 3.4a). */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

/* "#<word[j] without its punctuation, lower case>#", for the "#a#b#" word lists */
static void tag_word(const TextProc *t, int j, char *out)
{
    strcpy(out, "#");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
    fe_strlwr(out);
    strcat(out, "#");
}
static int in_lists(const char *tag, int from, int to)
{
    for (int l = from; l <= to; l++)
        if (fe_strpos(fe_mod_stopwords[l], tag)) return 1;
    return 0;
}
static int is_article3(const char *w) { return !strcmp(w, "a") || !strcmp(w, "an") || !strcmp(w, "the"); }
/* word[j] without its punctuation, lower case */
static void bare_lc(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
    fe_strlwr(out);
}
/* the number word as measure_sfx wants it: the digits, or "1" / "9" for one / other number words */
static void number_sfx(const TextProc *t, int j, char *out)
{
    char w[64];
    if (t->num[j]) { strcpy(out, t->word[j]); return; }
    strcpy(w, t->word[j]);
    fe_strlwr(w);
    strcpy(out, strcmp(w, "one") == 0 ? "1" : "9");
}
static void add_per(TextProc *t, int j)
{
    char buf[128];
    if (!fe_strpos(t->word[j], "per") && !fe_strpos(t->accepted_text, "per")) {
        strcpy(buf, "per ");
        strcat(buf, t->rec_expansion);
        strcpy(t->rec_expansion, buf);
    }
}
/* digits, '.' and ',' only, over the first n characters */
static int digits_only(const char *s, int n)
{
    for (int k = 0; k <= n - 1; k++)
        if ((strncmp(s + k, "0", 1) < 0 || strncmp(s + k, "9", 1) > 0) && strncmp(s + k, ".", 1) != 0 &&
            strncmp(s + k, ",", 1) != 0)
            return 0;
    return 1;
}
/* the dollar amount of word[j] without the '$' and its punctuation, lower case */
static void dollar_amount(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j] + 1, (size_t)((int)strlen(t->word[j]) - (int)strlen(t->punc[j]) - 1));
    fe_strlwr(out);
}

/* Find the number a measure belongs to and how to read it (measure_sfx, used by Rule_MeasureModifiers):
   "5 sq ft", "5 doz", "5 or more ft", "5 ft/sec" (per), "$5/lb", "about 5 ft", "at least 5 ft", "5 ft long".
   Where none is found, "in" after a verb stays a preposition. Lists 4-5 and 9-23 of fe_mod_stopwords are the
   preposition and verb lists. @0x10048910 */
void fe_Measure_Finalize(TextProc *t, short i, short *res)
{
    static const char units_short[] = "#ft#in#lb#mb#mi#mo#p#pg#pp#sec#";
    static const char approx[] = "#approx#approximately#about#only#";
    static const char compare[] = "#at least#more than#less than#fewer than#";
    short ok = *res;
    char qtype[64] = "", tag[64], w[64], sfx[64];
    int i3 = i - 3, i2 = i - 2;

    if (ok) goto fin;
    if (strcmp(t->rule_type, "measure1") && strcmp(t->rule_type, "measure2") && strcmp(t->rule_type, "measure3") &&
        strcmp(t->rule_type, "measure4"))
        goto fin;

    /* number, then a measure or range word, then '/', then a premodifier */
    tag[0] = 0;
    if (i3 > 0) tag_word(t, i - 2, tag);
    if (i3 > 0 && (t->num[i - 3] || t->psnum[i - 3]) &&
        (strncmp(t->qual[i - 2], "measure", 7) == 0 || fe_strpos(t->qual[i - 2], "range") ||
         fe_strpos(units_short, tag)) &&
        strcmp(t->sepr[i - 2], "/") == 0 &&
        (!strcmp(t->qual[i - 1], "premod") || !strcmp(t->qual[i - 1], "ppmod"))) {
        ok = 1;
        strcpy(qtype, t->qual[i - 3]);
        number_sfx(t, i - 3, t->measure_sfx);
        goto fin;
    }

    if (i - 2 > 0) {
        /* "5 sq ft", "5 doz ft" */
        const char *q = t->qual[i - 1];
        if ((t->num[i - 2] || t->psnum[i - 2]) &&
            (!strcmp(q, "premod") || !strcmp(q, "ppmod") || !strcmp(q, "numod") || strncmp(t->word[i - 1], "doz", 3) == 0)) {
            ok = 1;
            strcpy(qtype, t->qual[i - 2]);
            number_sfx(t, i - 2, sfx);
            strcpy(t->measure_sfx, sfx);
            if (!strcmp(q, "numod")) strcat(t->measure_sfx, "0");
            goto fin;
        }
        /* "5 or more ft" */
        if (i3 > 0 && t->num[i - 3] && !strcmp(t->word[i - 2], "or") &&
            (!strcmp(t->word[i - 1], "more") || !strcmp(t->word[i - 1], "less") ||
             !strcmp(t->word[i - 1], "greater") || !strcmp(t->word[i - 1], "fewer"))) {
            ok = 1;
            strcpy(t->measure_sfx, t->word[i - 3]);
            goto fin;
        }
        /* "5 ft/sec" seen from the second unit */
        tag_word(t, i - 1, tag);
        if ((t->num[i - 2] || t->psnum[i - 2]) &&
            (strncmp(t->qual[i - 1], "measure", 7) == 0 || fe_strpos(t->qual[i - 1], "range") ||
             fe_strpos(units_short, tag)) &&
            strcmp(t->sepr[i - 1], "/") == 0) {
            ok = 1;
            add_per(t, i - 1);
            goto fin;
        }
        /* dollars: "$5 million/yr" */
        strcpy(w, "");
        strncat(w, t->word[i - 2], 1);
        if (strcmp(w, "$") == 0 && strcmp(t->sepr[i - 1], "/") == 0) {
            int digits;
            dollar_amount(t, i - 2, w);
            digits = digits_only(w, (short)strlen(w));
            strcpy(w, t->word[i - 1]);
            fe_strlwr(w);
            if (digits && (!strcmp(w, "million") || !strcmp(w, "billion"))) {
                ok = 1;
                add_per(t, i - 1);
                goto fin;
            }
        }
    }

    if (t->cur_word < i + 1 || i - 1 < 1) goto late;
    /* "5 ft long" */
    if ((t->num[i - 1] || t->psnum[i - 1]) &&
        (!strcmp(t->qual[i + 1], "postmod") || !strcmp(t->qual[i + 1], "ppmod"))) {
        ok = 1;
        strcpy(qtype, t->qual[i - 1]);
        number_sfx(t, i - 1, t->measure_sfx);
        goto fin;
    }
    /* "5 ft/sec" seen from the first unit: the number moves into measure_sfx and "per" follows */
    tag_word(t, i + 1, tag);
    if ((t->num[i - 1] || t->psnum[i - 1]) && strcmp(t->sepr[i], "/") == 0 &&
        (strncmp(t->qual[i + 1], "measure", 7) == 0 || fe_strpos(t->qual[i + 1], "range") ||
         fe_strpos(units_short, tag))) {
        ok = 1;
        strcpy(qtype, t->qual[i - 1]);
        number_sfx(t, i - 1, sfx);
        strcpy(t->measure_sfx, sfx);
        strcpy(t->word[i - 1], "");
        strcat(t->rec_expansion, " per");
        goto fin;
    }
    /* a number (or a / an / the) before, and a preposition after: "5 ft above" */
    bare_lc(t, i - 1, w);
    if (!t->num[i - 1] && !t->psnum[i - 1] && !is_article3(w)) goto late;
    tag_word(t, i + 1, tag);
    if (in_lists(tag, 4, 5)) goto accept;
    if (i2 < 1) goto late;
    /* "about 5 ft" */
    tag_word(t, i - 2, tag);
    if (fe_strpos(approx, tag)) {
        if (!(strcmp(t->word_lc, "in") == 0 && !fe_strpos(t->punc[i], "."))) goto accept;
    }
    if (i3 >= 1) {
        /* "at least 5 ft" */
        strcpy(w, "#");
        strncat(w, t->word[i - 3], strlen(t->word[i - 3]) - strlen(t->punc[i - 3]));
        strcat(w, " ");
        strncat(w, t->word[i - 2], strlen(t->word[i - 2]) - strlen(t->punc[i - 2]));
        fe_strlwr(w);
        strcat(w, "#");
        if (!fe_strpos(compare, w)) goto late;
        if (strcmp(t->word_lc, "in") == 0 && !fe_strpos(t->punc[i], ".")) goto late;
        goto accept;
    }

late:
    if (i - 1 < 1) goto fin;
    /* "$5/sq ft", "$5k/yr" */
    strcpy(w, "");
    strncat(w, t->word[i - 1], 1);
    if (strcmp(w, "$") == 0 && strcmp(t->sepr[i - 1], "/") == 0) {
        dollar_amount(t, i - 1, w);
        if (digits_only(w, (short)strlen(w) - 1)) {
            const char *last = fe_tail(w, 1);
            if ((strncmp(last, "0", 1) >= 0 && strncmp(last, "9", 1) <= 0) || strncmp(last, "k", 1) == 0 ||
                strncmp(last, "m", 1) == 0) {
                ok = 1;
                add_per(t, i - 1);
                goto fin;
            }
        }
    }
    /* the last word of the paragraph */
    if (t->cur_word == i) {
        bare_lc(t, i - 1, w);
        if (!t->num[i - 1] && !t->psnum[i - 1] && !is_article3(w)) goto fin;
        if (strcmp(t->word_lc, "in") == 0 && i2 > 0) {
            /* "... turned 5 in": after a verb, "in" is the preposition */
            tag_word(t, i - 2, tag);
            if (in_lists(tag, 9, 23)) { ok = 0; goto fin; }
            if (i3 > 0) {
                tag_word(t, i - 3, tag);
                if (in_lists(tag, 9, 23)) { ok = 0; goto fin; }
            }
        }
        goto accept;
    }
    goto fin;

accept:
    ok = 1;
fin:
    if (strcmp(qtype, "which") == 0 &&
        (!strcmp(t->rec_keyword, "p") || !strcmp(t->rec_keyword, "mo") || !strcmp(t->rec_keyword, "in") ||
         !strcmp(t->rec_keyword, "gal"))) {
        ok = 0;
        strcpy(t->measure_sfx, "");
    }
    if (i - 1 > 0 && (!strcmp(t->word_lc, "gal") || !strcmp(t->word_lc, "ft") || !strcmp(t->word_lc, "sec") ||
                      !strcmp(t->word_lc, "s"))) {
        bare_lc(t, i - 1, w);
        if (is_article3(w)) {
            ok = 0;
            strcpy(t->measure_sfx, "");
        }
    }
    *res = ok;
}
