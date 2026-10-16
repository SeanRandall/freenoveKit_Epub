/* Years, decades and ranges ("5-10" -> "5 to 10", "1950s", "between 5 and 10"): the number stage's range readers
   (patent 142; REFERENCE 3.8, 8.4). */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

#pragma GCC diagnostic ignored "-Wrestrict"   /* the work buffers share one frame array */

static int rd32(const char *p) { int v; memcpy(&v, p, 4); return v; }
static void wr32(char *p, int v) { memcpy(p, &v, 4); }

/* atoi 0x10078370 (MSVC atol): no overflow check, the value wraps */
static int ms_atoi(const char *s)
{
    unsigned v = 0;
    int neg;
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    neg = *s == '-';
    if (*s == '-' || *s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (unsigned)(*s++ - '0');
    return neg ? -(int)v : (int)v;
}

/* word[j] = s. The original copies in place, one character longer than the word it replaces (the sign prefix);
   with fe_fixes a longer string gets a new block. */
static void set_word(TextProc *t, int j, const char *s)
{
    if (fe_fixes && strlen(s) > strlen(t->word[j])) {
        fe_free(t->word[j]);
        t->word[j] = fe_strdup(s);
    } else {
        strcpy(t->word[j], s);
    }
}

/* num[k], read as the original does when k is past the table (it indexes the object with the saved i, which the
   overrun in Num_YearsRanges can clobber); 0 outside the object */
static short num_at(const TextProc *t, int k)
{
    long off = (long)offsetof(TextProc, num) + 2L * k;
    short v = 0;
    if (off >= 0 && off + 2 <= (long)sizeof *t) memcpy(&v, (const char *)t + off, 2);
    return v;
}

/* the " to" / " and" of an accepted range: " and" when "betw..." is one of the four words before i */
static short range_word(TextProc *t, int i)
{
    short r = 1, si;
    for (si = 1; i - si >= 1; ) {
        /* (i may be garbage after the overrun in Num_YearsRanges; the original then crashes here) */
        if (i - si >= FE_MAXWORDS || !t->word[i - si]) break;
        if (strncmp(t->word[i - si], "betw", 4) == 0) { r = 2; break; }
        if (++si > 4) break;
    }
    if (r == 2) {
        strcat(t->out_text, " and");
        r = 1;
    } else {
        strcat(t->out_text, " to");
    }
    return r;
}

/* The work buffers keep their frame offsets ([F+x] of the listing): some are read before they are written in this
   call, and the colon test overruns S28 into the saved index. */
#define YR_FRAME 0x184
#define SIGN (F + 0x16)   /* "$" or the first character of word i */
#define S1C  (F + 0x1c)   /* 8 bytes */
#define S28  (F + 0x28)   /* 8 bytes, followed by the index i (F+0x30) */
#define S34  (F + 0x34)   /* "", "year", "$" or "#" */
#define S40  (F + 0x40)   /* the second number */
#define S7C  (F + 0x7c)   /* its digits */
#define SB8  (F + 0xb8)   /* the first number's digits */
#define SF4  (F + 0xf4)
#define Q130 (F + 0x130)  /* its QUAL (12 bytes) */
#define Q13C (F + 0x13c)  /* the QUAL after it (12 bytes) */
#define S148 (F + 0x148)
#define FI   rd32(F + 0x30)
#define FI2  rd32(F + 0x3c)

/* strip the first modifier of fe_num_mods ("K", "million", ...) that ends s; returns the length of the last one tried */
static short strip_mod(char *F, char *s)
{
    short bp, bx = 0;
    for (bp = 0; bp <= fe_num_mods_count - 1; bp++) {
        strcpy(SF4, fe_num_mods[bp]);
        bx = (short)strlen(SF4);
        if ((short)strlen(s) > bx && strncmp(s + strlen(s) - bx, SF4, bx) == 0) {
            S148[0] = 0;
            strncat(S148, s, strlen(s) - bx);
            strcpy(s, S148);
            break;
        }
    }
    return bx;
}

/* digits, '.' and ',' only */
static int all_digits(char *F, const char *s)
{
    short dx;
    if ((short)strlen(s) >= 1) F[0xf5] = 0;
    for (dx = 1; (short)strlen(s) >= dx; dx++) {
        signed char c = s[dx - 1];
        F[0xf4] = c;
        if (!(c >= '0' && c <= '9') && c != '.' && c != ',') return 0;
    }
    return 1;
}

/* Word i and the next number (word i+1, or word i+2 after a "-" word) form a range when the second is larger:
   "5-10", "5%-10%", "1950s-60s"/"1950's-60's" (years), "$5-10K", "#5-#10", "9:30-10:45", "5th-10th" (QUAL ordnl),
   "Jan 1990 - Feb 1991" (QUAL month year month year). It then appends " to" (" and" after "betw...") to out_text,
   removes the "-" word, gives the second number the first one's '$' or '#' and sets *res = 1. @0x1003df70 */
void fe_Num_YearsRanges(TextProc *t, short i, short *res)
{
    unsigned char Fb[YR_FRAME + 0x100];
    char *F = (char *)Fb;
    const unsigned char *fr = fe_frame_of(0x1003df70);
    short r = *res, f14 = 0, bx = 0, si, k;
    int n = i;
    char **W = t->word + n, **Q = t->qual + n;
    signed char c;

    if (fr) memcpy(Fb, fr, YR_FRAME); else memset(Fb, 0, YR_FRAME);
    memset(Fb + YR_FRAME, 0, 0x100);
    F[0x17] = 0;
    S34[0] = 0;
    SIGN[0] = '$';
    wr32(F + 0x30, n);
    if (t->cur_word < n + 1) return;

    wr32(F + 0x3c, n + 2);
    if (strcmp(W[1], "-") != 0) {
        strcpy(S40, W[1]);
        strcpy(Q130, Q[1]);
        if (t->cur_word >= n + 2) strcpy(Q13C, Q[2]);
    } else {
        if (t->cur_word < n + 2) goto out;
        strcpy(S40, W[2]);
        strcpy(Q130, Q[2]);
        if (n + 3 <= t->cur_word) strcpy(Q13C, Q[3]);   /* (else Q13C is stale; tested below all the same) */
    }
    for (;;) {
        c = *fe_tail(S40, 1);
        S1C[0] = c;
        S1C[1] = 0;
        if (!c || !strchr(".,;:!?)]", c)) break;
        SF4[0] = 0;
        strncat(SF4, S40, strlen(S40) - 1);
        strcpy(S40, SF4);
    }

    S1C[0] = 0;
    strcat(S1C, fe_tail(W[0], 1));
    if (S1C[0] == '%') {                                   /* "5%-10%" */
        c = *fe_tail(S40, 1);
        S28[0] = c;
        S28[1] = 0;
        if (S1C[0] != c) goto out;
        SB8[0] = 0;
        strncat(SB8, W[0], strlen(W[0]) - 1);
        S7C[0] = 0;
        strncat(S7C, S40, strlen(S40) - 1);
        goto l_eae9;
    }
    if (strcmp(S1C, "s") == 0) {                           /* "1950's-60's", "1950s-60s" */
        if (strncmp(fe_tail(W[0], 2), "'", 1) == 0) {
            S28[0] = 0;
            strncat(S28, fe_tail(S40, 2), 2);
            if (strcmp(S28, "'s") != 0) goto out;
            SB8[0] = 0;
            strncat(SB8, W[0], strlen(W[0]) - 2);
            S7C[0] = 0;
            strncat(S7C, S40, strlen(S40) - 2);
        } else {
            if (strcmp(fe_tail(S40, 1), "s") != 0) goto out;
            SB8[0] = 0;
            strncat(SB8, W[0], strlen(W[0]) - 1);
            S7C[0] = 0;
            strncat(S7C, S40, strlen(S40) - 1);
        }
        strcpy(S34, "year");
        goto l_eae9;
    }
    if (strcmp(S1C, "0") >= 0 && strcmp(S1C, "9") <= 0) {
        c = *fe_tail(S40, 1);
        S28[0] = c;
        S28[1] = 0;
        if (c == '%') {                                    /* "5-10%" */
            strcpy(SB8, W[0]);
            S7C[0] = 0;
            strncat(S7C, S40, strlen(S40) - 1);
            goto l_eae9;
        }
        if (strcmp(S28, "s") == 0) {                       /* "1950-60s" */
            strcpy(SB8, W[0]);
            S7C[0] = 0;
            strcpy(S34, "year");
            if (strncmp(fe_tail(S40, 2), "'", 1) == 0) strncat(S7C, S40, strlen(S40) - 2);
            else strncat(S7C, S40, strlen(S40) - 1);
            goto l_eae9;
        }
        if (FI - 1 >= 1 && t->cur_word >= FI2 && strcmp(t->qual[n - 1], "month") == 0 && strcmp(Q[0], "year") == 0 &&
            strcmp(Q130, "month") == 0 && strcmp(Q13C, "year") == 0)
            goto l_accept;
    }

    if (strncmp(W[0], "$", 1) == 0 || strncmp(W[0], "#", 1) == 0) {
        SIGN[0] = 0;
        strncat(SIGN, W[0], 1);
        strcpy(SB8, W[0] + 1);
        strcpy(S7C, S40[0] == SIGN[0] ? S40 + 1 : S40);
        strcpy(S34, SIGN);
        goto l_eae9;
    }
    if (strcmp(Q[0], "ordnl") == 0 && strcmp(Q130, "ordnl") == 0) goto l_accept;

    /* times: the parts before the colons are compared (a part of 5 or more characters fails: "9999" > "0") */
    bx = fe_strpos(W[0], ":");
    f14 = fe_strpos(S40, ":");
    if (bx == 0) {
        if (f14 == 0) goto out;
        bx = (short)(strlen(W[0]) + 1);
    }
    SB8[0] = 0;
    strncat(SB8, W[0], bx - 1);
    strcpy(S1C, bx <= 5 ? SB8 : "9999");
    if ((short)strlen(W[0]) > bx) strcat(SB8, W[0] + bx);
    if (f14 == 0) f14 = (short)(strlen(S40) + 1);
    k = f14;
    S7C[0] = 0;
    strncat(S7C, S40, k - 1);
    /* The original tests the length of the first part here too, so a long second part ("5:00-123456789") runs over
       the 8-byte S28 into the saved index i. fe_fixes tests the second part. */
    {
        const char *s = (fe_fixes ? f14 : bx) <= 5 ? S7C : "0";
        memmove(S28, s, strlen(s) + 1);
    }
    if ((short)strlen(S40) > f14) strcat(S7C, S40 + k);

l_eae9:
    if (strcmp(S34, "$") == 0) {                           /* "$5-10K": the modifiers go */
        bx = strip_mod(F, SB8);
        bx = strip_mod(F, S7C);
    }
    if (!all_digits(F, SB8) || !all_digits(F, S7C)) goto out;
    if (strcmp(S34, "year") == 0 && strlen(S7C) == 2) {   /* "1950s-60s": 50 < 60 */
        SF4[0] = 0;
        strcat(SF4, fe_tail(SB8, 2));
        strcpy(SB8, SF4);
    }
    if (bx != 0 && f14 != 0) {
        si = (short)ms_atoi(S1C);
        if ((short)ms_atoi(S28) < si) goto out;
    } else {
        si = (short)ms_atoi(SB8);
        if ((short)ms_atoi(S7C) <= si) goto out;
    }

l_accept:
    r = 1;
    if (strcmp(W[1], "-") == 0) strcpy(W[1], "");
    r = range_word(t, FI);
    if (strcmp(S34, "$") != 0 && strcmp(S34, "#") != 0) goto out;
    if (num_at(t, FI + 1) != 0 && S40[0] != SIGN[0]) {
        strcpy(S40, S34);
        strcat(S40, W[1]);
        set_word(t, n + 1, S40);
    }
    if (strcmp(W[1], "") == 0 && t->cur_word >= FI2 && num_at(t, FI + 2) != 0 && S40[0] != SIGN[0]) {
        strcpy(S40, S34);
        strcat(S40, W[2]);
        set_word(t, n + 2, S40);
    }
out:
    *res = r;
}
#undef SIGN
#undef S1C
#undef S28
#undef S34
#undef S40
#undef S7C
#undef SB8
#undef SF4
#undef Q130
#undef Q13C
#undef S148
#undef FI
#undef FI2

#define QC_FRAME 0x124
#define S1C (F + 0x1c)    /* QUAL of the second word (12 bytes) */
#define S28 (F + 0x28)    /* the QUAL after it (12 bytes) */
#define S34 (F + 0x34)
#define S70 (F + 0x70)    /* the second word */
#define SAC (F + 0xac)
#define SE8 (F + 0xe8)

/* digits and '.' only */
static int digits_dot(char *F, const char *s)
{
    short dx;
    if ((short)strlen(s) >= 1) F[0x35] = 0;
    for (dx = 1; (short)strlen(s) >= dx; dx++) {
        signed char c = s[dx - 1];
        F[0x34] = c;
        if (!(c >= '0' && c <= '9') && c != '.') return 0;
    }
    return 1;
}

/* Ranges recognised by QUAL: word i and the next word (i+1, or i+2 after "-") are "numod" numbers ("$5 - $10 million"),
   "bcad" years, an "ampm" time after a number or "timenum", single letters or digits after a "prerange"/"range" word
   or a "p"/"pt" measure ("pages 5-9", "A-Z"), two "roman" numerals, two months or two weekdays. It then appends " to"
   (" and" after "betw...") to out_text, removes the "-" word and sets *res = 1. @0x1003f2a0 */
void fe_Num_QualContext(TextProc *t, short i, short *res)
{
    unsigned char Fb[QC_FRAME + 0x100];
    char *F = (char *)Fb;
    const unsigned char *fr = fe_frame_of(0x1003f2a0);
    short r = *res, num2, tn, si;
    int n = i;

    if (fr) memcpy(Fb, fr, QC_FRAME); else memset(Fb, 0, QC_FRAME);
    memset(Fb + QC_FRAME, 0, 0x100);
    if (t->cur_word < n + 1) goto out;
    SAC[0] = 0;
    SE8[0] = 0;
    if (strcmp(t->word[n + 1], "-") != 0) {
        strcpy(S70, t->word[n + 1]);
        strcpy(S1C, t->qual[n + 1]);
        num2 = t->num[n + 1];
        tn = strcmp(t->qual[n + 1], "timenum") == 0;
        if (t->cur_word >= n + 2) strcpy(S28, t->qual[n + 2]);
    } else {
        if (t->cur_word < n + 2) goto out;
        strcpy(S70, t->word[n + 2]);
        strcpy(S1C, t->qual[n + 2]);
        num2 = t->num[n + 2];
        tn = strcmp(t->qual[n + 2], "timenum") == 0;
        if (n + 3 <= t->cur_word) strcpy(S28, t->qual[n + 3]);   /* (else S28 is stale; tested below all the same) */
    }
    for (;;) {
        S34[0] = 0;
        if (strlen(S70) != 0) S34[0] = S70[strlen(S70) - 1];
        S34[1] = 0;
        if (!S34[0] || !strchr(".,;:?!", S34[0])) break;
        S34[0] = 0;
        strncat(S34, S70, strlen(S70) - 1);
        strcpy(S70, S34);
    }

    if (n - 1 >= 1) {
        if (t->cur_word >= n + 2) {
            if (strcmp(t->qual[n], "numod") == 0 && strcmp(S28, "numod") == 0) {
                if (t->num[n - 1] != 0 && num2 != 0) goto accept;
                if (strncmp(t->word[n - 1], "$", 1) == 0 && strncmp(S70, "$", 1) == 0) {
                    strcpy(SAC, t->word[n - 1] + 1);
                    strcpy(SE8, S70 + 1);
                    goto l_fc7d;
                }
            }
            if (strcmp(t->qual[n], "bcad") == 0 && strcmp(S28, "bcad") == 0 && t->num[n - 1] != 0 && num2 != 0)
                goto accept;
        }
        if (t->cur_word >= n + 1) {
            if (strcmp(t->qual[n], "ampm") == 0 && (t->num[n - 1] != 0 || strcmp(t->qual[n - 1], "timenum") == 0) &&
                (num2 != 0 || tn != 0))
                goto accept;
            si = 0;
            if (strncmp(t->qual[n - 1], "measure", 7) == 0 && strncmp(t->word[n - 1], "p", 1) == 0) si = 1;
            else if (strcmp(t->qual[n - 1], "street") == 0 && strncmp(t->word[n - 1], "pt", 2) == 0) si = 1;
            if (strcmp(t->qual[n - 1], "prerange") == 0 || strcmp(t->qual[n - 1], "range") == 0 || si != 0) {
                const char *w = t->word[n];
                if (strlen(w) == 1 && strlen(S70) == 1) {
                    if (strcmp(w, "A") >= 0 && strcmp(w, "Z") <= 0 && strcmp(S70, "A") >= 0 && strcmp(S70, "Z") <= 0)
                        goto accept;
                    if (strcmp(w, "1") >= 0 && strcmp(w, "9") <= 0 && strcmp(S70, "1") >= 0 && strcmp(S70, "9") <= 0 &&
                        (strcmp(t->qual[n], "roman") != 0 || strcmp(S1C, "roman") == 0))
                        goto accept;
                }
                if (strcmp(t->qual[n], "roman") == 0 && strcmp(S1C, "roman") == 0) goto accept;
            }
        }
    }
    if (t->cur_word > i) {
        if (strcmp(t->qual[n], "month") == 0 && strcmp(S1C, "month") == 0) goto accept;
        if (strcmp(t->qual[n], "weekday") == 0 && strcmp(S1C, "weekday") == 0) goto accept;
    }
l_fc7d:
    if (SAC[0] == 0 || SE8[0] == 0) goto out;
    if (!digits_dot(F, SAC) || !digits_dot(F, SE8)) goto out;

accept:
    if (strcmp(t->word[n + 1], "-") == 0) strcpy(t->word[n + 1], "");
    r = range_word(t, n);
out:
    *res = r;
}
