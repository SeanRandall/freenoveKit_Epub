/* genl / ampm: the general abbreviations with a condition of their own (REFERENCE 3.4a). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

static void bare(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
}
static size_t bare_len(const TextProc *t, int j) { return strlen(t->word[j]) - strlen(t->punc[j]); }

/* One digit as a word: itself, or "oh" for 0. */
static void digit(char *out, char c)
{
    char d[3] = {c, 0, 0};
    strcat(out, c == '0' ? "oh" : d);
}

/* Digits read in groups, as for an extension or a phone number: "1 2 3 4,", "12 hundred," ("1 2 hundred,"),
   "5 thousand,", "oh oh oh oh, "; longer strings one by one. @0x1003b6f0 */
void fe_Num_SayDigitGroups(const char *s, char *out)
{
    short len;

    out[0] = 0;
    digit(out, s[0]);
    if (strlen(s) == 1) return;
    strcat(out, " ");
    if (strlen(s) > 4) {
        for (short k = 2; (short)strlen(s) >= k; k++) {
            digit(out, s[k - 1]);
            if ((short)strlen(s) <= k) return;
            strcat(out, " ");
        }
    }
    len = (short)strlen(s);
    if (len == 4) {
        if (!strcmp(s + len - 3, "000")) {
            if (!strcmp(fe_tail(out, 3), "oh ")) strcat(out, "oh oh oh, ");
            else strcat(out, "thousand,");
            return;
        }
        digit(out, s[1]);
        strcat(out, " ");
        if (!strcmp(s + len - 2, "00")) { strcat(out, "hundred,"); return; }
    }
    if (len == 3 && !strcmp(s + len - 2, "00")) { strcat(out, "hundred,"); return; }
    if (len > 2) {
        digit(out, s[len - 2]);
        strcat(out, " ");
    }
    digit(out, s[len - 1]);
    strcat(out, ",");
}

/* @0x100592f0 */
void fe_Rule_GenlAmpm(TextProc *t, short i, short *res)
{
    static const char special[] =
        "#c#w#lang#esp#cet#cntl#ctrl#sq#am#cf#eg#ie#ext#inc#jr#ltd#pf#ph#tel#sp#s#x#b#re#co#sys#i#a#d#ww#wwi#wwii#m#ca#"
        "sr#no#h#esc#vs#col#btw#";
    short ok = *res, pos;
    const char *w = t->word_lc;
    char tag[128], buf[128], digits[200];

    strcpy(tag, "#");
    strcat(tag, w);
    strcat(tag, "#");
    fe_str_replace_all(tag, ".", "", tag);
    pos = fe_strpos(special, tag);
    if (pos == 0 && strlen(w) == 1) goto done;
    switch (pos) {
    case 1:                                                  /* c/o */
        if (strcmp(t->sepr[i], "/") || t->cur_word <= i || t->cur_word < i + 1) goto done;
        if (strncmp(t->word[i + 1], "o", bare_len(t, i + 1))) goto done;
        strcpy(t->word[i + 1], "");
        break;
    case 3:                                                  /* w/, w/o */
        if (strcmp(t->sepr[i], "/")) goto done;
        if (i + 1 <= t->cur_word && !strncmp(t->word[i + 1], "o", bare_len(t, i + 1))) {
            strcpy(t->word[i + 1], "");
            strcpy(t->rec_expansion, "without");
        }
        break;
    case 5: case 10: case 34: case 37: case 40: case 122:   /* lang esp cf eg ie vs: lower case only */
        if (strcmp(t->word_as_typed, w)) goto done;
        break;
    case 14:                                                 /* cet par */
        if (t->cur_word < i + 1 || strncmp(t->word[i + 1], "par", bare_len(t, i + 1))) goto done;
        strcpy(t->word[i + 1], "");
        break;
    case 18:                                                 /* cntl. */
        if (strcmp(t->punc[i], ".")) goto done;
        break;
    case 23:                                                 /* Ctrl+X */
        if (t->cur_word < i + 1) goto done;
        if (!strcmp(t->sepr[i], "+") && t->iuc[i + 1] && strlen(t->word[i + 1]) == 1) break;
        if (t->cur_word < i + 2 || strcmp(t->word[i + 1], "+") || t->iuc[i + 2] == 0 || strlen(t->word[i + 2]) != 1)
            goto done;
        strcpy(t->word[i + 1], "");
        break;
    case 28:                                                 /* sq: a military squadron nearby */
        for (short s = i - 3; s <= i + 3; s++) {
            if (s > 0 && s <= t->cur_word) {
                char nw[128] = "";
                if (!strcmp(t->qual[s], "ordnl")) goto accept_as_is;
                strcpy(buf, t->word[s]);
                fe_strlwr(buf);
                if (s + 1 <= t->cur_word) {
                    strcpy(nw, t->word[s + 1]);
                    fe_strlwr(nw);
                }
                if (!strncmp(buf, "fighter", 7) || !strncmp(buf, "bomber", 6) || !strncmp(buf, "transport", 9) ||
                    !strncmp(buf, "tactical", 8) || (!strncmp(buf, "air", 3) && !strncmp(nw, "force", 5)))
                    goto accept_as_is;
            }
        }
        goto done;
    case 31:                                                 /* am: after a time */
        if (i + 1 < t->cur_word) {
            bare(t, i + 1, buf);
            if (!strcmp(buf, "I")) goto done;
            if (!strcmp(buf, "or") && i + 2 <= t->cur_word) {
                bare(t, i + 2, buf);
                if (!strcmp(buf, "pm") || !strcmp(buf, "p.m")) break;
            }
        }
        if (i - 1 < 1) goto done;
        if (strcmp(t->qual[i - 1], "timenum")) {
            bare(t, i - 1, buf);
            if (t->num[i - 1] == 0) {
                if (strcmp(buf, "o'clock")) goto done;
            } else {
                short h = (short)atoi(buf);
                if (h < 1 || h > 12) goto done;
            }
        }
        break;
    case 43:                                                 /* ext 1234 */
        if (t->cur_word < i + 1 || t->num[i + 1] == 0) goto done;
        bare(t, i + 1, buf);
        fe_Num_SayDigitGroups(buf, digits);
        strcat(t->rec_expansion, " ");
        strcat(t->rec_expansion, digits);
        strcpy(t->word[i + 1], "");
        break;
    case 51: case 110:                                       /* jr, sr */
        if (t->iuc[i] == 0 && strcmp(t->word_as_typed, w)) goto done;
        break;
    case 58:                                                 /* pF */
        if (strcmp(t->word_as_typed, "pF")) goto done;
        break;
    case 61: case 64:                                        /* ph, tel before a number */
        if (t->cur_word <= i) goto done;
        bare(t, i + 1, buf);
        fe_strlwr(buf);
        if (strcmp(buf, "#") && strcmp(buf, "#s") && strcmp(buf, "number") && strcmp(buf, "numbers") &&
            t->num[i + 1] == 0)
            goto done;
        break;
    case 68:                                                 /* sp? */
        buf[0] = 0;
        strncat(buf, t->punc[i], 1);
        if (buf[0] != '?') goto done;
        break;
    case 71: case 75: case 116:                              /* s/w, b/w, h/w */
        if (t->cur_word <= i || strcmp(t->sepr[i], "/")) goto done;
        bare(t, i + 1, buf);
        fe_strlwr(buf);
        if (strlen(buf) != 1 || buf[0] != 'w') goto done;
        strcpy(t->word[i + 1], "");
        break;
    case 73:                                                 /* x: "3 x 5", "5 ft x 3 ft" */
        if (i - 1 < 1 || t->cur_word < i + 1) goto done;
        if ((t->num[i - 1] || !strncmp(t->word[i - 1], "$", 1)) && (t->num[i + 1] || !strncmp(t->word[i + 1], "$", 1))) {
            if (i + 2 <= t->cur_word && !strcmp(t->word[i + 2], "=")) {
                strcpy(t->rec_expansion, "times");
                break;
            }
            if (t->num[i - 1] && t->num[i + 1]) break;
        }
        if (i - 2 < 1 || t->num[i - 2] == 0 || strncmp(t->qual[i - 1], "measure", 7) || t->num[i + 1] == 0 ||
            t->cur_word < i + 2 || strncmp(t->qual[i + 2], "measure", 7))
            goto done;
        break;
    case 77: case 80:                                        /* re, co: not in "re-", not after "dot" */
        if (!strcmp(t->sepr[i], "-")) goto done;
        if (t->auc[i] && i > 1 && !strcmp(t->word[i - 1], "dot")) goto done;
        break;
    case 83:                                                 /* sys */
        if (t->auc[i] && i > 1 && !strcmp(t->word[i - 1], "dot")) goto done;
        break;
    case 87: case 89: case 91: case 105:                     /* i/o, a/d, d/a, m/s */
        if (strcmp(t->sepr[i], "/") || t->cur_word < i + 1) goto done;
        bare(t, i + 1, buf);
        fe_strlwr(buf);
        if (!strcmp(w, "i") && !strcmp(buf, "o")) { strcpy(t->word[i + 1], ""); strcpy(t->rec_expansion, "eye-o"); break; }
        if (!strcmp(w, "a") && !strcmp(buf, "d")) { strcpy(t->word[i + 1], ""); strcpy(t->rec_expansion, "aye to d"); break; }
        if (!strcmp(w, "d") && !strcmp(buf, "a")) { strcpy(t->word[i + 1], ""); strcpy(t->rec_expansion, "d to aye"); break; }
        if (strcmp(w, "m") || strcmp(buf, "s")) goto done;
        strcpy(t->word[i + 1], "");
        strcpy(t->rec_expansion, "mail stop");
        break;
    case 93: case 96: case 100:                              /* WW, WWI, WWII in capitals */
        if (t->auc[i] == 0) goto done;
        break;
    case 107:                                                /* ca: before a number or a month */
        if (t->cur_word < i + 1) goto done;
        if (t->num[i + 1] == 0 && strcmp(t->qual[i + 1], "month")) goto done;
        break;
    case 113:                                                /* No. 5 */
        if (strcmp(t->punc[i], ".") || t->cur_word < i + 1 || t->num[i + 1] == 0) goto done;
        break;
    case 118:                                                /* Esc */
        if (t->iuc[i] == 0) goto done;
        break;
    case 125:                                                /* col 5, col b */
        if (t->cur_word < i + 1) goto done;
        if (t->num[i + 1] == 0 && bare_len(t, i + 1) != 1) goto done;
        break;
    case 129:                                                /* btw */
        if (t->auc[i] == 0 && i != 1) {
            if (strcmp(t->punc[i], ".") && (i < 2 || fe_strpos(t->punc[i - 1], ".") < 1)) goto done;
        }
        break;
    default:
        break;
    }
    /* a capitalised abbreviation gets a capitalised expansion */
    strcpy(buf, "");
    strncat(buf, t->word_as_typed, 1);
    fe_strupr(buf);
    if (!strncmp(t->word_as_typed, buf, 1)) {
        strcpy(tag, "");
        strncat(tag, t->rec_expansion, 1);
        fe_strupr(tag);
        strcat(tag, t->rec_expansion + 1);
        strcpy(t->rec_expansion, tag);
    }
accept_as_is:
    ok = 1;
done:
    *res = ok;
}
