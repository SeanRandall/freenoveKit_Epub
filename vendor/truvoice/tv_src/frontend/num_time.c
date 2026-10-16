/* Times with seconds, "10:30:45" (QUAL "timenum"; REFERENCE 3.8, 8.4). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

/* word_lc + n; the original reads past the terminator into the stale rest of word_lc ("100:30" asks for the seconds
   at +7). With fe_fixes a position past the end gives "". */
static const char *lc_at(TextProc *t, int n)
{
    if (fe_fixes && n > (int)strlen(t->word_lc)) return "";
    return t->word_lc + n;
}

/* "10:30:45" -> "10 hours, 30 minutes, 45 seconds", or "10:30 and 45 seconds" before am/pm, a weekday or a time
   word; "10:00" before am/pm -> "10". Whatever follows "h:m:s" in the word (up to 2 digits after the second
   colon) is appended to out_text unchanged. A trailing '.' sets lc_period. @0x10041230 */
void fe_Num_Time(TextProc *t, short i)
{
    /* Frame mirror: B [F+0x10] 60 bytes, tail [F+0x4c] only 12, H [F+0x58] 60. A long rest of the word runs from
       tail into H before H is used, and a long B runs into tail. fe_fixes gives each its own 256 bytes. */
    unsigned char F[0x300];
    char *B, *tail, *H;
    short p1, p2;
    int n;

    memset(F, 0, sizeof F);
    if (fe_fixes) {
        B = (char *)F;
        tail = (char *)F + 0x100;
        H = (char *)F + 0x200;
    } else {
        B = (char *)F + 0x10;
        tail = (char *)F + 0x4c;
        H = (char *)F + 0x58;
    }

    p1 = fe_strpos(t->word_lc, ":");
    p2 = fe_strpos(t->word_lc + p1, ":");
    /* A second colon without a digit after it is punctuation: "18:35:" gives "18 hours, 35 minutes,  seconds" in
       the original. With fe_fixes it stays "18:35" + ":". */
    if (fe_fixes && p2 && !(t->word_lc[p1 + p2] >= '0' && t->word_lc[p1 + p2] <= '9')) p2 = 0;
    H[0] = 0;
    strncat(H, t->word_lc, p1 + p2 + 2);
    tail[0] = 0;
    if ((short)strlen(t->word_lc) > p1 + p2 + 2) strcat(tail, t->word_lc + p1 + p2 + 2);
    if (!strcmp(fe_tail(t->word_lc, 1), ".")) t->lc_period = 1;
    strcpy(t->word_lc, H);
    B[0] = 0;

    if (strlen(t->word_lc) > 5) {
        n = (int)strlen(t->word_lc);
        if (!strncmp(fe_tail(t->word_lc, 2), "0", 1)) {
            /* "10:30:05" -> "10:30:5" */
            strncat(B, t->word_lc, n - 2);
            strcat(B, fe_tail(t->word_lc, 1));
            strcpy(t->word_lc, B);
            B[0] = 0;
        }
        if (t->cur_word >= i + 1 &&
            (!strcmp(t->qual[i + 1], "ampm") || !strcmp(t->qual[i + 1], "weekday") || !strcmp(t->qual[i + 1], "time"))) {
            strncat(B, t->word_lc, p1 + 2);
            strcat(B, " and ");
            strcat(B, lc_at(t, p1 + 3));
            strcat(B, " seconds");
        } else {
            strncat(B, t->word_lc, p1 - 1);
            strcat(B, " hours, ");
            if (!strncmp(lc_at(t, p1), "0", 1)) strncat(B, lc_at(t, p1 + 1), 1);
            else strncat(B, lc_at(t, p1), 2);
            strcat(B, " minutes, ");
            strcat(B, lc_at(t, p1 + 3));
            strcat(B, " seconds ");
        }
    } else if (!strcmp(lc_at(t, p1), "00") && t->cur_word >= i + 1 && !strcmp(t->qual[i + 1], "ampm")) {
        strncat(B, t->word_lc, p1 - 1);
    } else {
        /* out_text is the word as typed, which already ends in the tail: "14:11)" -> "14:11))" in the original */
        strcpy(B, t->out_text);
        if (fe_fixes && strlen(B) >= strlen(tail) && !strcmp(B + strlen(B) - strlen(tail), tail)) tail[0] = 0;
    }
    strcpy(t->out_text, fe_str_trim_spaces(B));
    strcat(t->out_text, tail);
}
