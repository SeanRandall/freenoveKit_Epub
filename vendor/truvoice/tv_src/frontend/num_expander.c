/* NumberExpander: the number expander's dispatcher (patent 142; REFERENCE 3.8, 8.4). */
#include <string.h>
#include <stdlib.h>
#include "frontend.h"

#pragma GCC diagnostic ignored "-Wrestrict"   /* the work buffers share one frame array */

/* The four 60-byte work buffers keep their frame offsets: some tests index them with computed offsets. */
#define FRAME 0x110
#define F30 (F + 0x30)
#define F6C (F + 0x6c)
#define FA8 (F + 0xa8)   /* word[i+1], as Num_DatesFractionsOrdinals may rewrite it; copied back at the end */
#define FE4 (F + 0xe4)

static size_t bare_len(const TextProc *t, int j) { return strlen(t->word[j]) - strlen(t->punc[j]); }

/* A number word i: feet/inches (5' 6"), minutes/seconds after "deg", "n/50%", years and ranges, weekday dates, phone
   numbers, digit strings, number agreement ("5-10 miles"), fractions and dates, special numbers, zip codes read digit
   by digit ("01234", "01234-5678"). *feet is set after a feet mark; *quote is the caller's "inside quotes" flag, which
   a closing '"' clears instead of reading it as inches. The expansion goes to out_text or into word[i+1].
   @0x10035940 */
void fe_NumberExpander(TextProc *t, short i, short *feet, short *quote)
{
    unsigned char Fb[FRAME];
    char *F = (char *)Fb;
    const unsigned char *fr = fe_frame_of(0x10035940);
    short a = *feet, b = *quote, pct = 0, deg, res, k;
    int n = i;

    if (fr) memcpy(Fb, fr, FRAME); else memset(Fb, 0, FRAME);
    if (t->cur_word == i) goto l_6768;

    /* the mark after the number: ' or " (possibly as the word's punctuation) */
    strcpy(FA8, t->word[n + 1]);
    F6C[0] = 0;
    FE4[0] = 0;
    if (strcmp(t->punc[n], "\"") == 0) {
        strcat(F6C, t->word[n] + strlen(t->word[n]) - 1);
        strncat(FE4, t->word[n], strlen(t->word[n]) - 1);
    } else if (!fe_fixes || bare_len(t, n) > 0) {
        /* A punctuation-only word can arrive here with a NUM flag left over from an earlier paragraph (ClassifyWord
           does not reset it); the original then reads the byte in front of the word [original]. */
        strncat(F6C, t->word[n] + bare_len(t, n) - 1, 1);
        strncat(FE4, t->word[n], bare_len(t, n) - 1);
    }
    if (strcmp(F6C, "'") != 0 && strcmp(F6C, "\"") != 0) { a = 0; goto l_6120; }

    deg = 0;
    if (n - 1 >= 1) {
        if (fe_strpos(t->word[n - 1], "deg")) deg = 1;
        if (n - 2 >= 1 && fe_strpos(t->word[n - 2], "deg")) deg = 1;
    }
    (void)atoi(FE4);
    if (strcmp(F6C, "'") == 0) {
        strcpy(t->out_text, FE4);
        F30[0] = 0;
        if (t->cur_word >= n + 1) strncat(F30, t->word[n + 1], bare_len(t, n + 1));
        fe_strlwr(F30);
        if (!deg && t->cur_word >= n + 1 && strcmp(F30, "s") == 0) {
            /* "1990's" */
            strcat(t->out_text, t->word[n + 1]);
            strcpy(t->word[n + 1], "");
        } else {
            short j = i;
            strcpy(t->measure_sfx, FE4);
            strcpy(F30, FE4);
            strcpy(t->out_text, deg ? "minute" : "foot");
            if (t->cur_word >= n + 1 && fe_strpos(t->word[n + 1], "\"")) j++;
            fe_Rule_MeasureModifiers(t, j);
            strcat(F30, " ");
            strcat(F30, t->out_text);
            strcpy(t->out_text, F30);
        }
        a = 1;
        strcat(t->out_text, t->punc[n]);
    } else {
        if (b != 0 && a == 0) { b = 0; a = 0; goto l_6120; }   /* the closing quote */
        strcpy(t->measure_sfx, FE4);
        strcpy(F30, FE4);
        strcpy(t->out_text, deg ? "second" : "inch");
        fe_Rule_MeasureModifiers(t, i);
        strcat(F30, " ");
        strcat(F30, t->out_text);
        strcpy(t->out_text, F30);
        a = 0;
        if (strcmp(t->punc[n], "\"") != 0) strcat(t->out_text, t->punc[n]);
    }
    if (t->cur_word >= n + 1 && fe_strpos(t->punc[n], ".") && t->iuc[n + 1]) strcat(t->out_text, " ");

l_6120:
    /* "n/50%": the percentage after a slash */
    F30[0] = 0;
    strncat(F30, t->word[n + 1], bare_len(t, n + 1));
    if (strcmp(t->sepr[n], "/") == 0 && F30[strlen(F30) - 1] == '%') {
        pct = 1;
        for (k = 1; (short)strlen(F30) - 1 >= k; k++)
            if (F30[k - 1] < '0' || F30[k - 1] > '9') { pct = 0; break; }
        if (pct) {
            t->num[n + 1] = 1;
            strcpy(t->word[n + 1], "");
            strncat(t->word[n + 1], F30, strlen(F30) - 1);
            strcat(t->word[n + 1], t->punc[n + 1]);
        }
    }

    if ((t->num[n + 1] == 0 && strcmp(t->sepr[n], "-") == 0) ||
        (t->cur_word >= n + 2 && strcmp(t->word[n + 1], "-") == 0 && t->num[n + 2] == 0)) {
        res = 0;
        fe_Num_YearsRanges(t, i, &res);
        goto l_6ef1;
    }
    if (t->cur_word < n + 2) goto l_6596;
    /* "Mon 1/16/95", "95-01-16" */
    if (strcmp(t->sepr[n], "/") == 0 && strcmp(t->sepr[n + 1], "/") == 0 && t->num[n + 2] &&
        strcmp(t->sepr[n + 2], "/") != 0) {
        if (i <= 1 || strcmp(t->qual[n - 1], "weekday") != 0) goto l_66d3;
        res = 0;
        fe_Num_Date_MDY(t, i, &res);
        if (res) goto done;
    }
    if (strcmp(t->sepr[n], "-") == 0 && strcmp(t->sepr[n + 1], "-") == 0 && t->num[n + 2] &&
        bare_len(t, n + 2) == 2 && strcmp(t->sepr[n + 2], "-") != 0) {
        res = 0;
        fe_Num_Date_MDY(t, i, &res);
        if (res) goto done;
    }
l_6596:
    if (t->num[n + 1] != 0 || strcmp(t->word[n + 1], "") == 0 || strcmp(t->word[n + 1], "(") == 0) {
        res = 0;
        fe_Num_Phone(t, i, &res);
        if (res) goto done;
        fe_Num_DigitString(t, i, &res);
        if (res) goto done;
    }
    if (strcmp(t->sepr[n], "-") == 0) goto l_6ccb;
    /* num[i+2] is read even when word i+2 does not exist [original] */
    if (strcmp(t->word[n + 1], "-") == 0 && t->sep_dash[n + 1] && t->num[n + 2] && (!fe_fixes || n + 2 <= t->cur_word))
        goto l_6ccb;
l_66d3:
    if (strcmp(t->sepr[n], "/") == 0 && t->num[n + 1]) {
        fe_Num_DatesFractionsOrdinals(t, i, FA8);
        if (strcmp(FA8, "DATEFLAG") == 0) goto done;
        t->out_text[0] = 0;
    }

l_6768:
    if (t->cur_word == i) goto l_684c;
    if (t->num[n + 1] && strlen(t->punc[n]) == 0 && strlen(t->sepr[n]) != 0) goto l_695d;
    if ((strcmp(t->word[n + 1], "-") == 0 || strcmp(t->word[n + 1], "/") == 0) && t->cur_word - n != 1 &&
        t->num[n + 2])
        goto l_695d;
l_684c:
    /* a number that is not part of a group: special numbers (911, 747), phone context */
    F6C[0] = 0;
    if (n - 1 >= 1 && (!fe_fixes || bare_len(t, n - 1) > 0)) strncat(F6C, t->word[n - 1] + bare_len(t, n - 1) - 1, 1);
    if (i != 1) {
        if (t->num[n - 1] && strlen(t->punc[n - 1]) == 0 && strlen(t->sepr[n - 1]) != 0) goto l_695d;
        if ((F6C[0] == '-' || F6C[0] == '/') && i != 2 && t->num[n - 2]) goto l_695d;
    }
    res = 0;
    fe_Num_SpecialNumbers(t, i, &res);
    if (res) goto done;
    fe_Num_PhoneContext(t, i, &res);
    if (res) goto done;

l_695d:
    /* zip codes: "01234" and "nnnnn-nnnn" digit by digit */
    if (bare_len(t, n) == 5 && strncmp(t->word[n], "0", 1) == 0) {
        t->out_text[0] = 0;
        strcpy(F30, t->word[n]);
        for (k = 0; k <= 4; k++) {
            strncat(t->out_text, F30 + k, 1);
            if (k <= 3) strcat(t->out_text, " ");
        }
        strcat(t->out_text, t->punc[n]);
        goto done;
    }
    if (bare_len(t, n) != 10 || strncmp(t->word[n] + 5, "-", 1) != 0) goto l_6ef1;
    t->out_text[0] = 0;
    strncat(t->out_text, t->word[n], 5);
    strcat(t->out_text, " dash ");
    strcpy(F30, t->word[n] + 6);
    for (k = 0; k <= 3; k++) {
        strncat(t->out_text, F30 + k, 1);
        if (k <= 3) strcat(t->out_text, " ");
    }
    /* The punctuation is appended twice (here and below) [original]. */
    if (!fe_fixes) strcat(t->out_text, t->punc[n]);
    if (strncmp(t->out_text, "0", 1) == 0) {
        strcpy(F30, t->out_text);
        t->out_text[0] = 0;
        for (k = 0; k <= 4; k++) {
            strncat(t->out_text, F30 + k, 1);
            if (k <= 3) strcat(t->out_text, " ");
        }
        strcat(t->out_text, F30 + 5);
    }
    strcat(t->out_text, t->punc[n]);
    goto done;

l_6ccb:
    /* "5-10 miles" -> "5 to 10", "between 5-10" -> "5 and 10" */
    res = 0;
    fe_Num_Agreement(t, i, &res);
    if (res) {
        for (k = 1; n - k >= 1; ) {
            if (strncmp(t->word[n - k], "betw", 4) == 0) { res = 2; break; }
            if (++k > 4) break;
        }
        strcat(t->out_text, res == 2 ? " and" : " to");
        goto l_6ef1;
    }
    strcpy(F6C, t->word[n + 1]);
    strcpy(FA8, t->word[n]);
    if (strcmp(F6C + strlen(F6C) - strlen(t->punc[n + 1]) - 1, "-") != 0) strcat(FA8, "-");
    if ((size_t)0x3a - strlen(F6C) > strlen(FA8)) strcat(FA8, F6C);
    t->out_text[0] = 0;

l_6ef1:
    /* the next word takes the rewritten form */
    if (t->cur_word < n + 1) goto done;
    if (strcmp(FA8, t->word[n + 1]) == 0 || strcmp(t->word[n + 1], "") == 0) goto done;
    if (pct) {
        F30[0] = 0;
        strncat(F30, FA8, strlen(FA8) - strlen(t->punc[n + 1]));
        strcat(F30, " %");
        strcat(F30, t->punc[n + 1]);
        strcpy(FA8, F30);
    }
    if (strlen(FA8) < 0x1e) {
        strcpy(t->word[n + 1], FA8);
    } else if (strlen(FA8) < 0x3a) {
        fe_free(t->word[n + 1]);
        t->word[n + 1] = fe_strdup(FA8);
    }
done:
    *feet = a;
    *quote = b;
}
