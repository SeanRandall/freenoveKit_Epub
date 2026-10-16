/* phone numbers: "(415) 555-1212" -> "area code 4 1 5, 5 5 5, 1 2 1 2,", "1-800-555-1212" -> "1, 8 oh oh, ...",
   "555-1212 ext 42" -> "... extension 4 2," (number expander 142; REFERENCE 3.8, 8.4). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

/* The original's buffers are 60 and 100 bytes, enough for words of at most 30 characters; these are larger. */
#define PH_BIG 512

static size_t bare_len(const TextProc *t, int j) { return strlen(t->word[j]) - strlen(t->punc[j]); }

/* the first n characters of w contain '.' or ',' */
static int has_sep(char *buf, const char *w, size_t n)
{
    buf[0] = 0;
    strncat(buf, w, n);
    return fe_strpos(buf, ".") || fe_strpos(buf, ",");
}

/* The last two groups, word[j] and word[j + 1], each read digit by digit, onto out. Sets *res and adds 2 to *count
   (nothing when word[j] is the last word, which the caller's checks exclude). @0x1003b4e0 */
static void Num_Phone_Group(TextProc *t, short j, short *res, short *count, char *buf, char *out, char *digits)
{
    short n = *count;

    buf[0] = 0;
    strncat(buf, t->word[j], bare_len(t, j));
    fe_Num_SayDigitGroups(buf, digits);
    strcat(out, digits);
    strcat(out, t->punc[j]);
    strcat(out, " ");
    n++;
    if (t->cur_word < j + 1) return;
    buf[0] = 0;
    strncat(buf, t->word[j + 1], bare_len(t, j + 1));
    fe_Num_SayDigitGroups(buf, digits);
    strcat(out, digits);
    strcat(out, t->punc[j + 1]);
    n++;
    *res = 1;
    *count = n;
}

/* After the number (word[i + *count]): "ext 42", "ext. 42", "x 42" or "x42" ("(" removed, up to 4 digits) appends
   " extension 4 2," to out_text and adds the words used to *count. @0x1003bcf0 */
static void Num_Phone_Extension(TextProc *t, short i, short *count, char *digits, unsigned char stale)
{
    short n = *count, s, ok;
    int k = i + n;
    char b1[PH_BIG], b3[PH_BIG];
    /* b2 follows b1 in the original's frame: Num_SayDigitGroups("") reads the byte in front of it, b1[59], which is
       never written (stale: Num_Phone's [F-0x89], passed in as stale) */
    char b12[60 + PH_BIG], *b2 = b12 + 60;

    if (t->cur_word < k) goto done;
    b1[0] = 0;
    strncat(b1, t->word[k], bare_len(t, k));
    fe_strlwr(b1);
    fe_str_replace_all(b1, "(", "", b1);
    if (!strcmp(b1, "ext") || !strcmp(b1, "ext.") || (b1[0] == 'x' && strlen(b1) == 1 && k + 1 <= t->cur_word)) {
        if (t->cur_word < k + 1 || t->num[k + 1] == 0 || bare_len(t, k + 1) > 4) goto done;
        b2[0] = 0;
        strncat(b2, t->word[k + 1], bare_len(t, k + 1));
        fe_Num_SayDigitGroups(b2, digits);
        strcat(t->out_text, " extension ");
        strcat(t->out_text, digits);
        n += 2;
        goto done;
    }
    if (b1[0] != 'x') goto done;
    strcpy(b3, b1 + 1);
    if (strlen(b3) > 4) goto done;
    /* a lone "x" as the last word passes with no digits: the original says " extension ," */
    if (fe_fixes && b3[0] == 0) goto done;
    ok = 1;
    for (s = 1; (short)strlen(b3) >= s; s++)
        if (b3[s - 1] < '0' || b3[s - 1] > '9') { ok = 0; break; }
    if (!ok) goto done;
    b12[59] = (char)stale;
    strcpy(b2, b3);
    fe_Num_SayDigitGroups(b2, digits);
    strcat(t->out_text, " extension ");
    strcat(t->out_text, digits);
    n++;
done:
    *count = n;
}

/* A phone number starting at word[i]: ["1"] 3 digits, then 3 + 4 digits (an area code: its middle digit is 0 or 1,
   as before 1995) or 4 digits, no '.' or ',' in the groups. out_text gets the digits read in groups, the following
   words are emptied and the last one's punctuation is kept. *res is set when it is a phone number. @0x1003a690 */
void fe_Num_Phone(TextProc *t, short i, short *res)
{
    const unsigned char *fr = fe_frame_of(0x1003a690);
    short ok = *res, j = i, count = 0, d;
    char buf[PH_BIG], out[PH_BIG], digits[PH_BIG];

    if (strcmp(t->word[i], "1") && strlen(t->word[i]) != 3) goto done;
    out[0] = 0;
    digits[0] = 0;
    if (!strcmp(t->word[i], "1")) {
        j = i + 1;
        strcpy(out, "1, ");
        count = 1;
    }
    if (t->cur_word < j) goto done;
    if (strlen(t->word[j]) != 3 || strcmp(t->punc[j], "")) goto done;
    if (j + 2 <= t->cur_word) {
        if (t->num[j + 1] == 0) goto emit;
        if (strlen(t->word[j + 1]) == 3 && !strcmp(t->punc[j + 1], "") && t->num[j + 2] &&
            bare_len(t, j + 2) == 4) {
            /* area code + 3 + 4 */
            if (strncmp(t->word[j] + 1, "0", 1) && strncmp(t->word[j] + 1, "1", 1)) goto done;
            if (has_sep(buf, t->word[j], 3) || has_sep(buf, t->word[j + 1], 3) || has_sep(buf, t->word[j + 2], 4))
                goto done;
            if (strcmp(out, "1, ")) strcat(out, "area code ");
            buf[0] = 0;
            strncat(buf, t->word[j], bare_len(t, j));
            fe_Num_SayDigitGroups(buf, digits);
            strcat(out, digits);
            strcat(out, t->punc[j]);
            strcat(out, " ");
            count++;
            j++;
            goto group;
        }
    } else if (j + 1 > t->cur_word || t->num[j + 1] == 0) {
        goto emit;
    }
    /* 3 + 4 */
    if (bare_len(t, j + 1) != 4) goto emit;
    if (has_sep(buf, t->word[j], 3) || has_sep(buf, t->word[j + 1], 4)) goto done;
group:
    Num_Phone_Group(t, j, &ok, &count, buf, out, digits);
emit:
    if (ok == 0) goto done;
    strcpy(t->out_text, out);
    Num_Phone_Extension(t, i, &count, digits, fr ? fr[-0x89] : 0);
    digits[0] = 0;
    for (d = 1; count - 1 >= d; d++) {
        int k = i + d;
        if (t->cur_word < k) break;
        strcpy(t->word[k], "");
        t->num[k] = 0;
        strcpy(digits, t->punc[k]);
    }
    /* the last word's punctuation, unless out_text already ends in it (with *res set on entry and nothing matched,
       out_text is "" and the original compares from the byte in front of it) */
    if (strcmp(fe_tail(t->out_text, 1), digits)) strcat(t->out_text, digits);
done:
    *res = ok;
}
