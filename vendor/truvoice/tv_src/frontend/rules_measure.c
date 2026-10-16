/* The measure rules (REFERENCE 3.4a). Measure_Accept / Measure_Finalize build the expansion (singular or
   plural, "per", ...). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

/* word[i] without the punctuation stored after it (PUNC), lower case. */
static void word_bare_lc(const TextProc *t, int i, char *buf)
{
    buf[0] = 0;
    strncat(buf, t->word[i], strlen(t->word[i]) - strlen(t->punc[i]));
    fe_strlwr(buf);
}

/* "a sec", "the sec", "an s": the number word before is not really a number here. */
static int is_article(const char *w) { return fe_strpos("an", w) || strcmp("the", w) == 0; }

/* measure1: after a number, and only where the sentence goes on (the next word is lower case, a number or a
   direction). @0x10047a00 */
void fe_Rule_Measure1(TextProc *t, short i, short *res)
{
    short ok = *res;
    char buf[60];

    if (i - 1 > 0 && t->psnum[i - 1]) {
        word_bare_lc(t, i - 1, buf);
        if (strcmp(t->word_lc, "sec") == 0 && is_article(buf))
            t->psnum[i - 1] = 0;
        if (strcmp(t->word_lc, "s") == 0)
            t->psnum[i - 1] = 0;
    }
    if (i + 1 <= t->cur_word && i - 1 > 0 &&
        (t->fper[i] || strncmp(t->punc[i], ".", 1) == 0) &&
        (t->ilc[i + 1] || t->num[i + 1] || strcmp(t->qual[i + 1], "direction") == 0) &&
        (t->num[i - 1] || t->psnum[i - 1])) {
        fe_Measure_Accept(t, i, &ok);
    } else {
        fe_Measure_Accept(t, i, &ok);
        ok = 0;
    }
    fe_Measure_Finalize(t, i, &ok);
    *res = ok;
}

/* measure2: after a number. Mixed-case spellings count only for the units in the list; "p" before a word
   starting with m is part of "p m". @0x10047c60 */
void fe_Rule_Measure2(TextProc *t, short i, short *res)
{
    static const char mixed_ok[] = "#yd#hr#qt#gal#yr#tsp#wk#deg#";
    short ok = *res;
    char buf[60];

    if (i - 1 < 1) goto done;
    if (strcmp(t->word_lc, "p") == 0 && i + 1 <= t->cur_word) {
        buf[0] = 0;
        strncat(buf, t->word[i + 1], 1);
        fe_strlwr(buf);
        if (buf[0] == 'm') goto done;
    }
    if (t->psnum[i - 1]) {
        word_bare_lc(t, i - 1, buf);
        if (is_article(buf) &&
            (!strcmp(t->word_lc, "gal") || !strcmp(t->word_lc, "ft") || !strcmp(t->word_lc, "sec")))
            t->psnum[i - 1] = 0;
        if (strcmp(t->word_lc, "s") == 0)
            t->psnum[i - 1] = 0;
    }
    strcpy(buf, "#");
    strcat(buf, t->word_lc);
    strcat(buf, "#");
    if (t->ilc[i] || (!t->auc[i] && fe_strpos(mixed_ok, buf))) {
        if (t->num[i - 1] || t->psnum[i - 1])
            fe_Measure_Accept(t, i, &ok);
    }
    fe_Measure_Finalize(t, i, &ok);
done:
    *res = ok;
}

/* "n.n.n" with one- or two-digit parts, as in "v. 2.0.1". The original reads the parts into a 4-byte buffer. */
static int is_version(const TextProc *t, int j)
{
    const char *w = t->word[j];
    char part[64];
    short p1, p2;

    if (strlen(w) <= 4) return 0;
    p1 = fe_strpos(w, ".");
    if (p1 != 2 && p1 != 3) return 0;
    part[0] = 0;
    strncat(part, w, p1 - 1);
    if (atoi(part) <= 0) return 0;
    p2 = fe_strpos(w + p1, ".");
    if (p2 != 2 && p2 != 3) return 0;
    part[0] = 0;
    strncat(part, w + p1, p2 - 1);
    if (strcmp(part, "0") != 0 && atoi(part) <= 0) return 0;
    part[0] = 0;
    strncat(part, w + p1 + p2, (size_t)((int)(strlen(w) - strlen(t->punc[j])) - p2 - p1));
    return strcmp(part, "0") == 0 || atoi(part) > 0;
}

/* measure4: kHz/MHz/msec always; "v." before n.n.n is "version"; otherwise after a number. @0x10048090 */
void fe_Rule_Measure4(TextProc *t, short i, short *res)
{
    short ok = *res;
    size_t n;

    if (i - 1 > 0) {
        n = strlen(t->word_lc);
        if (strncmp(t->word_lc + n - 2, "hz", 2) != 0 && strcmp(t->word_lc, "msec") != 0) {
            if (strcmp(t->word_lc, "v") == 0 && strcmp(t->punc[i], ".") == 0 && i + 1 <= t->cur_word &&
                is_version(t, i + 1)) {
                ok = 1;
                strcpy(t->rec_expansion, "version");
                goto done;
            }
            if (t->num[i - 1] || t->psnum[i - 1])
                fe_Measure_Accept(t, i, &ok);
            fe_Measure_Finalize(t, i, &ok);
            goto done;
        }
        ok = 1;
    }
done:
    *res = ok;
}

/* Multiplier suffixes a number word may carry ("5K", "2million"). @0x10106990 (count @0x101069e4) */
static const char *const mult_suffix[] = {"+", "K", "k", "M", "m", "million", "billion", "'", "#"};

/* Accept a measure after a number and record in measure_sfx how to read it: "1" (singular) for one, "9"
   (plural) for other number words, the bare number + "0" for a digit string with a multiplier suffix. A '/'
   after the number makes the expansion "per ...". @0x10048460 */
void fe_Measure_Accept(TextProc *t, short i, short *res)
{
    char buf[60];
    const char *w;

    if (i - 1 < 1) return;
    w = t->word[i - 1];
    if (t->psnum[i - 1] == 0) {
        strcpy(t->measure_sfx, w + strlen(t->prepunc[i - 1]));
        for (int k = 0; k < (int)(sizeof mult_suffix / sizeof *mult_suffix); k++) {
            const char *sfx = mult_suffix[k];
            int cut = (int)strlen(w) - (int)strlen(sfx);
            if (strcmp(w + cut, sfx) == 0) {
                strcpy(buf, "");
                strncat(buf, w, (size_t)cut);
                strcpy(t->measure_sfx, fe_str_trim_spaces(buf));
                strcat(t->measure_sfx, "0");
                break;
            }
        }
    } else {
        if (strcmp(t->qual[i - 1], "which") == 0 &&
            (!strcmp(t->rec_keyword, "p") || !strcmp(t->rec_keyword, "mo") || !strcmp(t->rec_keyword, "in") ||
             !strcmp(t->rec_keyword, "gal")))
            return;
        strcpy(buf, w);
        fe_strlwr(buf);
        strcpy(t->measure_sfx, strcmp(buf, "one") == 0 ? "1" : "9");
    }
    if (strcmp(t->sepr[i - 1], "/") == 0 && !fe_strpos(t->accepted_text, "per") && !fe_strpos(w, "per")) {
        strcpy(buf, "per ");
        strcat(buf, t->rec_expansion);
        strcpy(t->rec_expansion, buf);
    }
    *res = 1;
}

static int has_range_mark(const char *w)
{
    return fe_strpos(w, "&") || fe_strpos(w, "+") || fe_strpos(w, "-") || fe_strpos(w, "/") || fe_strpos(w, "(") ||
           fe_strpos(w, "[");
}
static int is_joiner(const char *w) { return !strcmp(w, "and") || !strcmp(w, "or") || !strcmp(w, "by") || !strcmp(w, "x"); }
/* the sentence ends after word k (a period followed by a capital) or other punctuation follows it */
static int punct_stops(const TextProc *t, int k)
{
    const char *p = t->punc[k];
    return strlen(p) != 0 && (strcmp(p, ".") != 0 || (k + 1 <= t->cur_word && (t->iuc[k + 1] || t->auc[k + 1])));
}
/* the word after a measure is a plain word: no & + - / ( [, no leading punctuation, not "per" */
static int plain_word(const TextProc *t, int k)
{
    return !has_range_mark(t->word[k]) && strlen(t->prepunc[k]) == 0 && strcmp(t->word[k], "per") != 0;
}

/* Make the unit plural unless the number is one: "5 ft" -> "feet", "3 in" -> "inches", else + "s". Chains like
   "5 ft 3 in" are walked first; a following word that starts a new phrase (the stop-word lists) ends the walk.
   @0x1004af00 */
void fe_Rule_MeasureModifiers(TextProc *t, short i)
{
    static const char units[] = "footinchdozenmile per hour";
    char w[60], tail[60];
    short span = 2, off = 0;
    size_t n;

    if ((short)atoi(t->measure_sfx) == 1 && !fe_strpos(t->measure_sfx, ".") && !fe_strpos(t->measure_sfx, "and"))
        return;
    while (t->cur_word != (short)(i + off)) {
        int k = (short)(i + off);
        if (strlen(t->punc[k]) != 0 && strcmp(t->punc[k], "\"") != 0 && strcmp(t->punc[k], "'") != 0 &&
            punct_stops(t, k))
            break;
        if (strlen(t->sepr[k]) != 0 || fe_strpos(t->out_text, "to") || fe_strpos(t->out_text, "per")) break;
        if (k + 1 <= t->cur_word) {
            if (is_joiner(t->word[k + 1]) || !plain_word(t, k + 1)) break;
        }
        if (t->cur_word < k + 1) return;
        if (strcmp(t->qual[k + 1], "compmod") == 0) break;
        if (strcmp(t->qual[k + 1], "ppmod") == 0 || strcmp(t->qual[k + 1], "postmod") == 0) {
            if (t->cur_word - k == 1) break;
            if (punct_stops(t, k + 1)) break;
            if (strlen(t->sepr[k + 1]) != 0) break;
            if (k + 2 <= t->cur_word && (is_joiner(t->word[k + 2]) || !plain_word(t, k + 2))) break;
        }
        /* "5 deg 30' ..." */
        if (strncmp(t->out_text, "deg", 3) == 0) {
            strcpy(w, "");
            strncat(w, t->word[k + 1], strlen(t->word[k + 1]) - strlen(t->punc[k + 1]));
            if (strcmp(fe_tail(w, 1), "'") == 0) {
                strcpy(tail, "");
                strncat(tail, w, strlen(w) - 1);
                if (atoi(tail) != 0) break;
            }
        }
        strcpy(w, "#");
        strncat(w, t->word[k + 1], strlen(t->word[k + 1]) - strlen(t->punc[k + 1]));
        fe_strlwr(w);
        strcat(w, "#");
        for (int l = 0; l < (int)(sizeof fe_mod_stopwords / sizeof *fe_mod_stopwords); l++)
            if (fe_strpos(fe_mod_stopwords[l], w)) goto plural;
        /* "5 ft 3 in": go on with the next number + measure */
        if (t->cur_word < k + 2 || t->num[k + 1] == 0 || strncmp(t->qual[k + 2], "measure", 7) != 0) return;
        off += 2;
        span += 2;
        if (span < off) return;
    }
plural:
    n = strlen(t->out_text);
    if (strcmp(t->out_text + (int)n - 1, "s") == 0 || strcmp(t->out_text + (int)n - 1, "z") == 0) return;
    if (strncmp(t->rec_expansion, "per", 3) == 0) return;
    if (i - 1 > 0 && strcmp(t->sepr[i - 1], "-") == 0) return;
    if (i - 2 > 0 && strcmp(t->sepr[i - 2], "/") == 0 && !fe_strpos(t->measure_sfx, "and")) return;
    strcpy(tail, "");
    if (n > 4 && strcmp(t->out_text + n - 4, " per") == 0) {
        strcpy(tail, "");
        strncat(tail, t->out_text, n - 4);
        strcpy(t->out_text, tail);
        strcpy(tail, " per");
    }
    fe_strlwr(t->out_text);
    switch (fe_strpos(units, t->out_text)) {
    case 1: strcpy(t->out_text, "feet"); break;
    case 5: strcpy(t->out_text, "inches"); break;
    case 9: strcpy(t->out_text, "dozen"); break;
    case 14:
        if (strcmp(t->out_text, "mile per hour") == 0) { strcpy(t->out_text, "miles per hour"); break; }
        /* fall through */
    default: strcat(t->out_text, "s"); break;
    }
    strcat(t->out_text, tail);
}
