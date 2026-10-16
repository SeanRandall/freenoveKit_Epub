/* ClassifyWord (patent 136): the per-word classifier. Called once for each new word (cur_word) by the tokenizer;
   splits off the trailing punctuation (PUNC) and the opening quote/bracket (PREPUNC), sets the case and number flags
   and PSNUM/STPROV, and finds the word's QUAL (REFERENCE 3.2, 3.6). Written from the listing of 100330f0. */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

/* The byte in front of the word buffer (frame +0x3f), which the original never writes: it reads it as the last
   character of an empty word and as the letter before a one-letter "s". The port takes 0; the test harness passes
   the real byte (fe_cw_stack). */
const unsigned char *fe_cw_stack;
static const unsigned char *cw_st;   /* this call's: fe_cw_stack, or the harness's replay for a nested call */
static char before_w(void) { return !fe_fixes && cw_st ? (char)cw_st[0x3f] : 0; }

static char lastc(const char *w)
{
    size_t n = strlen(w);
    return n ? w[n - 1] : before_w();
}
static void chop(char *w, size_t k)       /* drop the last k characters (none left if shorter, as strncat) */
{
    size_t n = strlen(w);
    w[n >= k ? n - k : 0] = 0;
}
static int ends(const char *w, const char *s)
{
    size_t n = strlen(w), m = strlen(s);
    return n >= m && !strcmp(w + n - m, s);
}
static int ends_ordinal(const char *w)
{
    return ends(w, "st") || ends(w, "nd") || ends(w, "rd") || ends(w, "th");
}
static int is_genl(const char *q) { return !strcmp(q, "") || !strcmp(q, "genl"); }

/* Rows of the abbreviation table for the first character of word_lc (not lower-cased: other characters give
   rows 0 .. T[0]). @0x10040280 */
void fe_Abbrev_LetterRange(TextProc *t, short *start, short *end)
{
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz{";
    char c[2] = {t->word_lc[0], 0};
    short k = fe_strpos(letters, c);

    if (k > 52) k = 0;
    *start = k >= 1 ? fe_abbrev_letter_end[k - 1] + 1 : 0;
    *end = fe_abbrev_letter_end[k];
}

/* One abbreviation record whose key (or expansion) is the word: a state record gives the state name as QUAL and
   marks STPROV (except for or/ok/oh/hi/me/in/on); any other record gives its rule name. Returns 1 to stop. */
static int take_record(TextProc *t, int k, char *q)
{
    static const char *const words[] = {"or", "ok", "oh", "hi", "me", "in", "on"};
    int c = t->cur_word;

    if (!strncmp(fe_abbrev[k].rule, "state", 5)) {
        int w = 0;
        if (is_genl(q)) strcpy(q, fe_abbrev[k].exp);
        for (int j = 0; j < 7; j++)
            if (!strcmp(t->word_lc, words[j])) w = 1;
        if (!w) t->stprov[c] = 1;
    } else if (is_genl(q)) {
        strcpy(q, fe_abbrev[k].rule);
    }
    return !is_genl(q) && t->stprov[c];
}

/* the previous word, without its punctuation, in lower case, + " " + word_lc */
static void two_words(const TextProc *t, char *out)
{
    int c = t->cur_word;
    out[0] = 0;
    strncat(out, t->word[c - 1], strlen(t->word[c - 1]) - strlen(t->punc[c - 1]));
    fe_strlwr(out);
    strcat(out, " ");
    strcat(out, t->word_lc);
}

/* @0x100330f0 */
void fe_ClassifyWord(TextProc *t)
{
    static const char ordinals[] =
        "#fir#seco#thi#four#fif#six#seven#eigh#nine#ten#eleven#twelf#thirteen#fourteen#fifteen#sixteen#seventeen#"
        "eighteen#nineteen#twentie#thirtie#fortie#fiftie#sixtie#seventie#eightie#ninetie#hundred#thousand#";
    int c = t->cur_word;
    char w[256], p[64] = "", q[64] = "", pp[64], tmp[256], num[256], two[256], orig[256], sing[256];
    char ch, *s;
    short si, start, end, v;

    cw_st = fe_cw_stack ? fe_cw_stack : fe_frame_of(0x100330f0);
    strcpy(w, t->word[c]);
    t->fper[c] = t->ilc[c] = t->iuc[c] = t->auc[c] = 0;
    t->psnum[c] = t->stprov[c] = 0;
    /* NUM is set only for words that reach the number test: a punctuation-only word keeps whatever its slot holds
       (the TextProc is not cleared: an earlier paragraph's flag or heap garbage), and the number stage runs on it. */
    if (fe_fixes) t->num[c] = 0;
    strcpy(pp, fe_str_trim_spaces(t->prepunc[c]));

    /* trailing punctuation -> PUNC (at most 4 characters) */
    if (lastc(w) == '\n') { strcpy(p, "\n"); chop(w, 1); }
    if (lastc(w) == '"') { strcpy(p, "\""); chop(w, 1); }
    if (lastc(w) == '.') t->fper[c] = 1;
    do {
        ch = lastc(w);
        if (!ch || !strchr(".,;!?:)]", ch)) break;
        tmp[0] = ch;
        tmp[1] = 0;
        strcat(tmp, p);
        strcpy(p, tmp);
        chop(w, 1);
    } while (strlen(p) != 4);
    /* an opening quote or bracket -> PREPUNC */
    if (w[0] == '"' || w[0] == '(' || w[0] == '[') {
        char o[2] = {w[0], 0};
        strcat(pp, o);
        memmove(w, w + 1, strlen(w));
    }
    if (w[0] == 0) goto done;

    if ((w[0] >= '0' && w[0] <= '9') || w[0] == '.') {
        /* a number: "20+", "256K", "5#" (= "5 pounds"), "5'", "3:45", "21st" */
        strcpy(num, w);
        for (int k = 0; k < fe_num_mods_count; k++) {
            if (!ends(w, fe_num_mods[k])) continue;
            num[0] = 0;
            strncat(num, w, strlen(w) - strlen(fe_num_mods[k]));
            if (!strcmp(fe_num_mods[k], "#")) {
                strcpy(t->word[c], num);
                strcat(t->word[c], " pounds");
            }
            break;
        }
        if (lastc(w) == '"' || lastc(w) == '\'') {
            num[0] = 0;
            strncat(num, w, strlen(w) - 1);
        }
        t->num[c] = 1;
        for (si = 2; (short)strlen(num) >= si; si++) {
            ch = w[si - 1];
            if (!((ch >= '0' && ch <= '9') || ch == '.' || ch == ',')) {
                t->num[c] = 0;
                break;
            }
        }
        if (t->num[c] == 0) {
            short colon = fe_strpos(num, ":");
            if (colon) {
                /* h:mm or h:mm:ss; the word buffer keeps what the test left in it */
                w[0] = 0;
                strncat(w, num, colon - 1);
                if (strlen(w) <= 2) {
                    v = (short)atoi(w);
                    if (v <= 24 && v > 0) {
                        w[0] = 0;
                        strcat(w, num + colon);
                        if (strlen(w) == 2) {
                            if ((short)atoi(w) < 60) strcpy(q, "timenum");
                        } else if (strlen(w) == 5) {
                            tmp[0] = 0;
                            strncat(tmp, w, 2);
                            if ((short)atoi(tmp) < 60) {
                                strcpy(tmp, w + 3);
                                if ((short)atoi(tmp) < 60) strcpy(q, "timenum");
                            }
                        }
                    }
                }
            } else if ((short)strlen(num) - si == 1 && strlen(num) > 2 && ends_ordinal(num)) {
                strcpy(q, "ordnl");
            }
        }
    } else {
        t->num[c] = 0;
    }

    if (t->num[c] && !strcmp(t->sepr[c], "+")) strcat(t->word[c], " +");
    if (t->num[c]) {
        v = (short)atoi(w);
        switch (strlen(w)) {
        case 1: strcpy(q, "day"); break;
        case 2: strcpy(q, v < 32 ? "day" : "oldyr"); break;
        case 3: strcpy(q, "oldyr"); break;
        case 4: if (v < 2500) strcpy(q, "year"); break;
        }
        goto done;
    }

    /* case flags */
    strcpy(orig, w);
    fe_strlwr(w);
    t->auc[c] = 1;
    for (si = 1; (short)strlen(w) >= si; si++) {
        unsigned char a = orig[si - 1], b = w[si - 1];
        if (a == b && b >= 'a' && b <= 'z') {
            t->auc[c] = 0;
            break;
        }
    }
    if (orig[0] != w[0] && !strcmp(w + 1, orig + 1)) t->iuc[c] = 1;
    if (orig[0] == w[0]) t->ilc[c] = 1;

    /* the QUAL table, also for the singular ("boxes" -> "box", "feet"...) */
    strcpy(tmp, w);
    sing[0] = 0;
    if (lastc(w) == 's') {
        chop(tmp, 1);
        strcpy(sing, tmp);
        if ((strlen(w) >= 2 ? w[strlen(w) - 2] : before_w()) == 'e') {
            tmp[0] = 0;
            strncat(tmp, w, strlen(w) - 2);
        }
    }
    for (int k = 0; k < fe_qual_table_count; k++) {
        if (!strcmp(tmp, fe_qual_table[k].a) || !strcmp(sing, fe_qual_table[k].a)) {
            strcpy(q, fe_qual_table[k].b);
            break;
        }
    }
    strcpy(t->word_lc, tmp);
    /* PSNUM: a count word ("which", "number", "dozen"), or "of" after one */
    if (!strcmp(q, "which") || !strcmp(q, "number") || !strncmp(t->word_lc, "doz", 3)) t->psnum[c] = 1;
    if (!strcmp(w, "of") && c - 1 >= 1 &&
        (!strcmp(t->qual[c - 1], "which") || !strcmp(t->qual[c - 1], "number") || !strncmp(t->word[c - 1], "doz", 3))) {
        t->psnum[c] = 1;
        if (strlen(q) == 0) strcpy(q, "which");
    }
    /* spelled ordinals: "fifth", "twentieth", "hundredth" */
    if (!strcmp(q, "") && strlen(w) >= 5 && ends_ordinal(w)) {
        strcpy(two, "#");
        strncat(two, w, strlen(w) - 2);
        strcat(two, "#");
        if (fe_strpos(ordinals, two)) strcpy(q, "ordnl");
    }

    /* the abbreviation table: by key, then by expansion ("Calif" / "california" -> state) */
    strcpy(t->word_lc, w);
    fe_str_replace_all(t->word_lc, ".", " ", t->word_lc);
    fe_Abbrev_LetterRange(t, &start, &end);
    t->rule_type[0] = 0;
    for (short k = start; k <= end; k++) {
        strcpy(w, fe_abbrev[k].key);
        fe_strlwr(w);
        if (!strcmp(t->word_lc, w)) {
            if (take_record(t, k, q)) break;
        } else if (strncmp(t->word_lc, w, strlen(t->word_lc)) < 0) {
            break;                                  /* the table is sorted */
        }
    }
    if (is_genl(q)) {
        for (short k = start; k <= end; k++) {
            strcpy(w, fe_abbrev[k].exp);
            fe_strlwr(w);
            if (!strcmp(t->word_lc, w) && take_record(t, k, q)) break;
        }
    }
    /* a two-word expansion with the previous word ("new york", "rhode island"): both words get the QUAL */
    if (c > 1 && !strcmp(t->qual[c - 1], "") && is_genl(q)) {
        two_words(t, two);
        strcpy(t->u_2bd5, t->word_lc);
        strcpy(t->word_lc, two);
        fe_Abbrev_LetterRange(t, &start, &end);
        for (short k = start; k <= end; k++) {
            strcpy(w, fe_abbrev[k].exp);
            fe_strlwr(w);
            if (strcmp(t->word_lc, w)) continue;
            if (!strncmp(fe_abbrev[k].rule, "state", 5)) {
                strcpy(q, fe_abbrev[k].exp);
                t->stprov[c] = 1;
                strcpy(t->qual[c - 1], fe_abbrev[k].exp);
                t->stprov[c - 1] = 1;
            } else {
                strcpy(q, fe_abbrev[k].rule);
                strcpy(t->qual[c - 1], fe_abbrev[k].rule);
            }
            if (!is_genl(q) && t->stprov[c]) break;
        }
        strcpy(t->word_lc, t->u_2bd5);
    }
    /* a full state name in the city-state table */
    if (t->stprov[c] == 0) {
        strcpy(t->u_2bd5, t->word_lc);
        fe_CityState_LetterRange(t, &start, &end);
        for (short k = start; k <= end; k++) {
            if (!strcmp(fe_city_state[k].a, t->word_lc)) {
                t->stprov[c] = 1;
                break;
            }
        }
    }
    /* a city, also of two words ("san jose"): the previous word gets QUAL city too */
    if (is_genl(q)) {
        for (int k = 0; k < fe_city_state_count; k++) {
            if (!strcmp(fe_city_state[k].b, t->word_lc)) {
                strcpy(q, "city");
                break;
            }
        }
    }
    if (c > 1 && (!strcmp(t->qual[c - 1], "") || !strncmp(t->qual[c - 1], "direction", 9)) && is_genl(q)) {
        two_words(t, two);
        for (int k = 0; k < fe_city_state_count; k++) {
            if (!strcmp(fe_city_state[k].b, two)) {
                strcpy(q, "city");
                strcpy(t->qual[c - 1], "city");
                break;
            }
        }
    }
    /* am/pm, bc/ad (also "a.m.", "b. c."), then roman numerals */
    if (!strcmp(q, "")) {
        strcpy(w, t->word_lc);
        fe_strlwr(w);
        fe_str_replace_all(w, ".", "", w);
        fe_str_replace_all(w, " ", "", w);
        if (strlen(w) == 2) {
            if (!strcmp(w, "am") || !strcmp(w, "pm")) strcpy(q, "ampm");
            if (!strcmp(w, "bc") || !strcmp(w, "ad")) strcpy(q, "bcad");
        }
    }
    if (strlen(q) == 0) {
        strcpy(q, "roman");
        for (si = 1; (short)strlen(w) >= si; si++) {
            if (!strchr("ivxlcdm", w[si - 1])) {
                q[0] = 0;
                break;
            }
        }
    }

done:
    /* The PUNC buffer is 4 bytes, directly followed by the QUAL buffer: a 4-character PUNC ("?!?!") runs on into
       QUAL, and cutting it back to 4 characters writes its terminator over QUAL's first byte. */
    if (!fe_fixes && strlen(p) >= 4) q[0] = 0;   /* (fe_fixes: the QUAL is kept) */
    if (strlen(p) > 4) p[4] = 0;
    if (strlen(p) < 4) {
        s = fe_strdup("    ");
        strcpy(s, p);
    } else {
        s = fe_strdup(p);
    }
    t->punc[c] = s;
    t->qual[c] = fe_strdup("                                                            ");
    strcpy(t->qual[c], q);
    strcpy(t->prepunc[c], pp);
}
