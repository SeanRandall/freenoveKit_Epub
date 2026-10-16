/* Number stage: "a/b" pairs (fractions, month/day dates, ordinals of the denominator) and numeric m/d/y dates
   (patent number expander 142, REFERENCE 3.8, 8.4). Written from the listings.

   Both functions keep their locals in a byte array laid out like the original stack frame (byte x = [F+x] of
   tools/asm_frame.py), with the word tables the original copies onto its stack at entry at the same offsets: an
   index outside a table reads the neighbouring local as in the original. */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

#pragma GCC diagnostic ignored "-Wrestrict"

static const char MONTHS[] =
    "January  February March    April    May      June     July     August   SeptemberOctober  November December ";

/* the DLL's atoi (0x10078370): no overflow check, the value wraps */
static int tv_atoi(const char *s)
{
    unsigned v = 0;
    int neg;
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    neg = *s == '-';
    if (*s == '-' || *s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (unsigned)(*s++ - '0');
    return neg ? -(int)v : (int)v;
}

static void trim_into(char *s)
{
    char *r = fe_str_trim_spaces(s);
    if (r != s) strcpy(s, r);
}

/* MonthDay_Ordinal(j) called from the frame F (fr = its snapshot). The original never writes its suffix buffer when
   no suffix applies ("25'": the last character is below '4' and not a digit) and appends what is there up to a NUL:
   the 4 bytes at [F-0x8c], then its saved `this` and j. Called from here, that slot is not the one the port's static
   models, so under the harness the real bytes are passed. */
static void month_day(TextProc *t, short j, const unsigned char *fr)
{
    char st[16];
    int jj = j;

    if (fr && fe_frame_this) {
        memcpy(st, fr - 0x8c, 4);
        memcpy(st + 4, &fe_frame_this, 4);
        memcpy(st + 8, &jj, 4);
        st[12] = 0;
        fe_mdo_sfx = st;
    }
    fe_MonthDay_Ordinal(t, j);
}

/* ---- Num_Date_MDY --------------------------------------------------------------------------------------------- */
/* "12/25/95" -> "December 25th, 1995": word[i] is the month number, word[i+1] the day (made an ordinal),
   word[i+2] the year (its leading digits; two digits get "19"). The result goes to out_text; word[i+1],
   word[i+2] and the separators after word[i] and word[i+1] are emptied, *res = 1. NumberExpander calls it for
   "m/d/y" after a weekday and for "m-d-yy". @0x1003a1e0 */
void fe_Num_Date_MDY(TextProc *t, short i, short *res)
{
    const unsigned char *fr = fe_frame_of(0x1003a1e0);
    char F[0x110];
    char *tmp = F + 0x18, *yr = F + 0x28, *m = F + 0x64, *out = t->out_text;
    const char *y;
    short k, mo;

    if (fr) memcpy(F, fr, sizeof F);
    else memset(F, 0, sizeof F);
    memcpy(F + 0xa0, MONTHS, sizeof MONTHS);
    if (t->cur_word < i + 2) return;
    y = t->word[i + 2];
    yr[0] = 0;
    strncat(yr, y, 1);
    for (k = 2; (short)strlen(t->word[i + 2]) >= k; k++) {
        y = t->word[i + 2] + k - 1;
        if (strncmp(y, "0", 1) < 0 || strncmp(y, "9", 1) > 0) break;
        strncat(yr, y, 1);
    }
    strcpy(m, t->word[i]);
    if (strlen(m) > 2 || strlen(t->word[i + 1]) > 2 || strlen(yr) == 3 || strlen(yr) > 4) return;
    mo = (short)tv_atoi(m);
    if (mo > 12) return;
    /* month 0 ("00", "0") reads the 9 bytes in front of the month table, the uninitialised end of m */
    if (fe_fixes && mo < 1) return;
    if ((short)tv_atoi(t->word[i + 1]) > 31) return;
    tmp[0] = 0;
    strncat(tmp, F + 0x97 + 9 * mo, 9);
    trim_into(tmp);
    out[0] = 0;
    strcat(out, tmp);
    strcat(out, " ");
    month_day(t, (short)(i + 1), fr);
    strcat(out, t->word[i + 1]);
    strcat(out, ", ");
    if (strlen(yr) == 2) strcat(out, "19");
    strcat(out, yr);
    strcat(out, t->punc[i + 2]);
    if (strcmp(t->punc[i + 2], "")) strcat(out, " ");
    strcpy(t->word[i + 1], "");
    strcpy(t->word[i + 2], "");
    strcpy(t->sepr[i], "");
    strcpy(t->sepr[i + 1], "");
    *res = 1;
}

/* ---- Num_DatesFractionsOrdinals -------------------------------------------------------------------------------- */
/* frame offsets */
#define TMP   0x18
#define T54   0x54
#define NXT   0x90    /* word[i+1] without PUNC, then its reading */
#define P1    0xcc    /* word[i-1] .. word[i-3] and word[i+2], word[i+3]: lower case, without PUNC */
#define N2    0x108
#define P2    0x144
#define N3    0x180
#define P3    0x1bc
#define CUR   0x1f8   /* word[i] without PUNC (after a '-': the part after it) */
#define LC    0x234
#define W1    0x2c4
#define LA    0x300
#define LB    0x3bc
#define TENS  0x490   /* 9 x 10 */
#define ORDS  0x4ec   /* 9 x 11 */
#define MON   0x550   /* 12 x 9 */
#define FRACS 0x5c0
#define DENOM 0x660   /* 19 x 11 */
#define FSIZE 0x734

static const char s_denom[] = "one        half       third      fourth     fifth      sixth      seventh    eighth     "
                              "ninth      tenth      eleventh   twelfth    thirteenth fourteenth fifteenth  sixteenth  "
                              "seventeentheighteenth nineteenth ";
static const char s_ords[] =
    "first      second     third      fourth     fifth      sixth      seventh    eighth     ninth      ";
static const char s_tens[] = "one       twentieth thirtieth fourtieth fiftieth  sixtieth  seventietheightieth ninetieth ";
static const char s_fracs[] = "#1/2#1/3#2/3#1/4#3/4#1/5#2/5#3/5#4/5#1/6#5/6#1/7#1/8#3/8#5/8#7/8#1/9#1/10#2/10#3/10#4/10#"
                              "6/10#7/10#8/10#9/10#1/12#1/16#3/16#5/16#7/16#9/16#11/16#13/16#15/16#";
/* words that make "a/b" next to them a date */
static const char s_la[] = "#arrival#arrive#afternoon#anniversary#appointment#begin#beginning#birth#birthday#born#"
                           "business#breakfast#coming#communication#conference#conversation#correspondence#date#"
                           "deadline#depart#";
static const char s_lb[] = "#departure#died#dinner#discussion#draft#due#during#early#earnings#edition#ending#end#"
                           "evening#expiration#expire#fax#late#letter#lunch#meeting#memo#memorandum#message#month#"
                           "morning#new#midnight#night#nite#noon#";
static const char s_lc[] = "#party#period#plan#prior#program#project#later#sooner#reply#report#revision#schedule#"
                           "scheduled#session#statement#supper#talk#till#valid#week#";

static int and_or(const char *w) { return !strcmp(w, "and") || !strcmp(w, "&") || !strcmp(w, "or"); }
static int more_less(const char *w) { return !strcmp(w, "more") || !strcmp(w, "less") || !strcmp(w, "fewer"); }

/* word j without PUNC, lower case */
static void bare_lc(char *d, const TextProc *t, int j)
{
    d[0] = 0;
    strncat(d, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
    fe_strlwr(d);
}

/* TMP = "#word": drop a final 's', add "#"; whether it is in one of the date-context lists */
static int date_word(char *F)
{
    char *tmp = F + TMP, *t54 = F + T54;
    if (!strcmp(tmp + strlen(tmp) - 1, "s")) {
        t54[0] = 0;
        strncat(t54, tmp, strlen(tmp) - 1);
        strcpy(tmp, t54);
    }
    strcat(tmp, "#");
    return fe_strpos(F + LA, tmp) || fe_strpos(F + LB, tmp) || fe_strpos(F + LC, tmp);
}

/* "#word[i-1]", or "#word[i-2]" after "of" / "to" */
static void prev_ctx(char *F, int i)
{
    char *tmp = F + TMP;
    strcpy(tmp, "#");
    strcat(tmp, F + P1);
    if ((!strcmp(tmp, "#of") || !strcmp(tmp, "#to")) && i - 2 >= 1) {
        strcpy(tmp, "#");
        strcat(tmp, F + P2);
    }
}

/* "#word[i+2]", or "#word[i+3]" after "or" */
static void next_ctx(char *F, int i, int cur)
{
    char *tmp = F + TMP;
    strcpy(tmp, "#");
    strcat(tmp, F + N2);
    if (!strcmp(tmp, "#or") && i + 3 <= cur) {
        strcpy(tmp, "#");
        strcat(tmp, F + N3);
    }
}

/* "m/d/yy" three dates later: "1/2 and 3/4/95" */
static int date_follows(const TextProc *t, int i, int cur)
{
    return i + 3 <= cur && t->num[i + 3] && !strcmp(t->sepr[i + 3], "/") && i + 4 <= cur && t->num[i + 4] &&
           !strcmp(t->sepr[i + 4], "/") && i + 5 <= cur && t->num[i + 5] &&
           strlen(t->word[i + 5]) - strlen(t->punc[i + 5]) == 2;
}

/* word[i] / word[i+1] (sepr[i] is "/"). next holds a copy of word[i+1] and receives the reading of the pair:
   "3/4" -> "3 fourths", "1/2" -> "1 half", "2 1/2" -> "2 and 1 half", "3/32" -> "3 30 seconds",
   "1/100" -> "1 hundredths", "5/200" -> "5 slash 200"; a month/day pair in a date context ("Mon 1/5", "1/5 at 3 pm",
   "due 1/5") -> out_text "January 5th" and next "DATEFLAG"; "12/25/95" -> out_text "12, 25, 95" and "DATEFLAG";
   a pair that is neither -> "13 , 14". "3/4%" drops the '%' from next. A '-' in word[i] ("1-3/4") becomes
   " and ". @0x100370f0 */
void fe_Num_DatesFractionsOrdinals(TextProc *t, short i, char *next)
{
    const unsigned char *fr = fe_frame_of(0x100370f0);
    char F[FSIZE];
    char *tmp = F + TMP, *t54 = F + T54, *nxt = F + NXT, *p1 = F + P1, *n2 = F + N2, *p2 = F + P2, *n3 = F + N3,
         *p3 = F + P3, *cw = F + CUR, *w1 = F + W1;
    int cur = t->cur_word, off;
    char big[256], *d;
    int a, b;   /* 16 bits in the original: "40000/5" is -25536/5 */
    short k, p, pct = 0, dash = 0;
    size_t l;

#define W(k) t->word[i + (k)]
#define Q(k) t->qual[i + (k)]
#define P(k) t->punc[i + (k)]
#define S(k) t->sepr[i + (k)]
#define N(k) t->num[i + (k)]
    if (fr) memcpy(F, fr, sizeof F);
    else memset(F, 0, sizeof F);
    memcpy(F + DENOM, s_denom, sizeof s_denom);
    memcpy(F + ORDS, s_ords, sizeof s_ords);
    memcpy(F + TENS, s_tens, sizeof s_tens);
    memcpy(F + FRACS, s_fracs, sizeof s_fracs);
    memcpy(F + LA, s_la, sizeof s_la);
    memcpy(F + LB, s_lb, sizeof s_lb);
    memcpy(F + LC, s_lc, sizeof s_lc);
    memcpy(F + MON, MONTHS, sizeof MONTHS);

    nxt[0] = 0;
    strncat(nxt, W(1), strlen(W(1)) - strlen(P(1)));
    cw[0] = 0;
    strncat(cw, W(0), strlen(W(0)) - strlen(P(0)));
    a = fe_fixes ? tv_atoi(cw) : (short)tv_atoi(cw);
    b = fe_fixes ? tv_atoi(nxt) : (short)tv_atoi(nxt);
    /* "3/4%": the character before PUNC; the original reads in front of next when it is shorter than PUNC + 1 */
    off = (int)strlen(next) - (int)strlen(P(1)) - 1;
    if (!(fe_fixes && off < 0) && !strncmp(next + off, "%", 1)) {
        t54[0] = 0;
        pct = 1;
        strncat(t54, next, strlen(next) - 1);
        strcpy(next, t54);
        strcat(next, P(1));
    }
    /* "1-3/4": the fraction is 3/4, word[i] becomes "1 and 3" */
    p = fe_strpos(W(0), "-");
    if (p) {
        cw[0] = 0;
        strncat(cw, W(0) + p, strlen(W(0)) - strlen(P(0)) - p);
        a = fe_fixes ? tv_atoi(cw) : (short)tv_atoi(cw);
        fe_str_replace_all(W(0), "-", " and ", W(0));
        dash = 1;
    }
    /* next is NumberExpander's 60-byte buffer and out_text has 60 bytes too: long numbers overrun both
       (two 30-digit numbers and "/95"); fe_fixes cuts the text */
    d = fe_fixes ? big : next;
    /* "12/25/95" */
    if (i + 2 <= cur && !strcmp(S(0), "/") && !strcmp(S(1), "/") && N(2) && strlen(W(2)) - strlen(P(2)) == 2 &&
        strcmp(S(2), "/")) {
        strcpy(d, W(0));
        strcat(d, ", ");
        strcat(d, W(1));
        strcat(d, ", ");
        strcat(d, W(2));
        strcat(d, P(2));
        strcpy(W(1), "");
        N(1) = 0;
        strcpy(S(1), "");
        strcpy(W(2), "");
        N(2) = 0;
        if (fe_fixes) {
            t->out_text[0] = 0;
            strncat(t->out_text, d, sizeof t->out_text - 1);
        } else {
            strcpy(t->out_text, next);
        }
        strcpy(next, "DATEFLAG");
        return;
    }

    if (b > 31 && b < 75) {
        if (b == 32 || b == 64) goto ord;
        goto slash;
    }
    if (a > 12 && !(b == 16 && (a == 13 || a == 15))) {
        if (a - b != -1) goto slash;
        goto comma;
    }
    if (b >= 75 && b <= 99) goto comma;
    if (b == 100 || b == 1000) goto ord;
    if (b > 100) goto slash;
    if (nxt[0] == '0' && strlen(nxt) > 1) {   /* "5/07" -> "5 , oh 7" */
        strcpy(tmp, "oh ");
        strcat(tmp, nxt + 1);
        strcpy(nxt, tmp);
        goto comma;
    }

    p1[0] = p2[0] = p3[0] = n2[0] = n3[0] = 0;
    if (i - 1 >= 1) {
        bare_lc(p1, t, i - 1);
        if (i - 2 >= 1) {
            bare_lc(p2, t, i - 2);
            if (i - 3 >= 1) bare_lc(p3, t, i - 3);
        }
    }
    if (i + 2 <= cur) {
        bare_lc(n2, t, i + 2);
        if (i + 3 <= cur) bare_lc(n3, t, i + 3);
    }

    /* a common fraction: read as one unless the context makes it a date */
    if (b <= 16) {
        strcpy(tmp, "#");
        strcat(tmp, cw);
        strcat(tmp, "/");
        strcat(tmp, nxt);
        strcat(tmp, "#");
        if (fe_strpos(F + FRACS, tmp)) {
            if (i + 2 <= cur && !strncmp(Q(2), "measure", 7)) goto ord;
            if (i + 3 <= cur && !strncmp(Q(3), "measure", 7)) goto ord;
            if (i - 1 >= 1 && N(-1) && (!strcmp(S(-1), "-") || !strcmp(W(-1), "-") || dash)) goto ord;
            if (!strcmp(n2, "of")) goto ord;
            if (!strcmp(n2, "or") && more_less(n3)) goto ord;
            if (more_less(n2) || !strcmp(n2, "full") || !strcmp(n2, "empty")) goto ord;
            if (i + 2 <= cur && !strcmp(Q(2), "compmod")) goto ord;
            if (!strcmp(p1, "than") && (!strcmp(p2, "more") || !strcmp(p2, "less") || !strcmp(p3, "greater")))
                goto ord;
            if (!strcmp(p1, "fully") || !strcmp(p1, "nearly") || !strcmp(p1, "over") || !strcmp(p1, "under") ||
                !strcmp(p1, "another"))
                goto ord;
            if (!strcmp(n2, "%") || pct) goto ord;
            if (i <= 1) goto dmy;
            if (!strcmp(Q(-1), "weekday")) goto dmy;
            if (i + 2 <= cur && !strcmp(Q(2), "weekday")) goto dmy;
            if (!strcmp(Q(-1), "time") || fe_strpos(p1, ":") || !strcmp(Q(-1), "ampm") || !strcmp(p1, "o'clock"))
                goto dmy;
            if (i + 2 <= cur) {
                if (!strcmp(Q(2), "timenum") || fe_strpos(n2, ":")) goto dmy;
                if (i + 3 <= cur && (!strcmp(Q(3), "ampm") || !strcmp(n3, "o'clock") || !strcmp(Q(3), "time")))
                    goto dmy;
            }
            /* "1/5/95 and 1/2": dates before or after. The test for " " never matches (probably meant "") */
            if (and_or(p1) && i - 2 >= 1 && (!strcmp(p2, " ") || (N(-2) && strlen(p2) == 2)) && i - 3 >= 1 &&
                (!strcmp(p3, " ") || (N(-3) && !strcmp(S(-3), "/"))) && i - 4 >= 1 && N(-4) && !strcmp(S(-4), "/"))
                goto dmy;
            if (i + 2 <= cur && and_or(n2) && date_follows(t, i, cur)) goto dmy;
            prev_ctx(F, i);
            if (date_word(F)) goto dmy;
            if (i + 2 > cur) goto ord;
            next_ctx(F, i, cur);
            if (!date_word(F)) goto ord;
        }
    }

dmy:
    /* month/day in a date context */
    if (a > 12 || b > 31) goto comma;
    if (i > 1 && !strcmp(Q(-1), "weekday")) goto month;
    if (i + 2 <= cur && !strcmp(Q(2), "weekday")) goto month;
    if (i - 1 >= 1 && (!strcmp(Q(-1), "ampm") || !strcmp(p1, "o'clock") || !strcmp(Q(-1), "time"))) {
        /* "3 pm 1/5", "at 3 o'clock 1/5" */
        if (i - 2 >= 1 && (N(-2) || !strcmp(Q(-2), "timenum") || fe_strpos(p2, ":"))) goto month;
        /* qual[i-2] is read also when i is 2 (the unused slot 0) */
        if (!strcmp(Q(-1), "time") && !(fe_fixes && i - 2 < 1) && (!strcmp(Q(-2), "ampm") || !strcmp(p2, "o'clock")) &&
            i - 3 >= 1 && (N(-3) || !strcmp(Q(-3), "timenum") || fe_strpos(p3, ":")))
            goto month;
    }
    if (i + 2 <= cur && (N(2) || !strcmp(Q(2), "timenum") || fe_strpos(n2, ":")) && i + 3 <= cur &&
        (!strcmp(Q(3), "ampm") || !strcmp(n3, "o'clock") || !strcmp(Q(3), "time")))
        goto month;
    if (i - 1 >= 1 && and_or(p1) && i - 2 >= 1 && N(-2) && strlen(p2) == 2 && i - 3 >= 1 && N(-3) &&
        !strcmp(S(-3), "/") && i - 4 >= 1 && N(-4) && !strcmp(S(-4), "/"))
        goto month;
    if (i - 4 >= 1 && and_or(p1) && !strcmp(p2, " ") && !strcmp(p3, " ") && N(-4) && !strcmp(S(-4), "/"))
        goto month;
    if (i + 2 <= cur && and_or(n2) && date_follows(t, i, cur)) goto month;
    if (i - 1 >= 1) {
        prev_ctx(F, i);
        if (date_word(F) && strcmp(tmp, "#morning#") && strcmp(tmp, "#week#")) goto month;
    }
    if (i + 1 > cur) goto comma;
    next_ctx(F, i, cur);
    if (!date_word(F) || !strcmp(tmp, "#morning#") || !strcmp(tmp, "#week#")) goto comma;

month:
    /* "January 5th"; month 0 or below reads in front of the month table (the end of the ordinal table) */
    off = MON - 9 + 9 * a;
    tmp[0] = 0;
    if (fe_fixes && a < 1) goto comma;   /* "Mon 0/5" -> "th 5th" (the end of "ninth") */
    if (off >= 0) strncat(tmp, F + off, 9);
    trim_into(tmp);
    t->out_text[0] = 0;
    strcat(t->out_text, tmp);
    strcat(t->out_text, " ");
    month_day(t, (short)(i + 1), fr);
    strcat(t->out_text, W(1));
    strcpy(W(1), "");
    strcpy(S(1), "");
    strcpy(next, "DATEFLAG");
    return;

comma:
    strcpy(t54, nxt);
    strcpy(nxt, ", ");
    strcat(nxt, t54);
    strcat(nxt, P(1));
    goto fin;

slash:
    strcpy(t54, nxt);
    strcpy(nxt, "slash ");
    strcat(nxt, t54);
    strcat(nxt, P(1));
    goto fin;

ord:
    /* the denominator as an ordinal: "4" -> "fourth", "20" -> "twentieth", "32" -> "30 second" */
    l = strlen(nxt);
    if (l == 1 || (l == 2 && nxt[0] == '1')) {
        t54[0] = 0;
        strncat(t54, F + DENOM - 11 + 11 * b, 11);
    } else if (l == 2 && nxt[1] == '0') {
        tmp[1] = 0;
        tmp[0] = nxt[0];
        k = (short)tv_atoi(tmp);
        t54[0] = 0;
        strncat(t54, F + TENS - 10 + 10 * k, 10);
    } else if (l == 2) {
        tmp[1] = 0;
        tmp[0] = nxt[1];
        k = (short)tv_atoi(tmp);
        t54[0] = nxt[0];
        t54[1] = '0';
        t54[2] = ' ';
        t54[3] = 0;
        strncat(t54, F + ORDS - 11 + 11 * k, 11);
    } else if (b == 100) {
        strcpy(t54, "hundredths");
    } else if (b == 1000) {
        strcpy(t54, "thousandths");
    } else {
        strcpy(t54, "/");
        strcat(t54, nxt);
    }
    trim_into(t54);
    if (!(dash && a == 1) && strcmp(W(0), "1") && strlen(nxt) <= 2) strcat(t54, "s");
    strcpy(nxt, t54);
    strcat(nxt, P(1));
    /* "2 3/4" -> "2 and 3 fourths" */
    if (i - 1 >= 1 && N(-1) && !dash) {
        strcpy(t54, "and ");
        strcat(t54, W(0));
        strcpy(W(0), t54);
    }

fin:
    if (!strcmp(nxt, W(1))) {
        strcpy(w1, W(1));
        strcpy(d, W(0));
        strcat(d, "/");
        strcat(d, w1);
    } else {
        strcpy(d, W(0));
        strcat(d, " ");
        strcat(d, nxt);
    }
    if (fe_fixes) {
        next[0] = 0;
        strncat(next, d, 59);
    }
#undef W
#undef Q
#undef P
#undef S
#undef N
}
