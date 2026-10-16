/*
 * Mode 2 of FrontEnd_ProcessText (REFERENCE 3.12): every input line is read on its own, with its numbers read the way
 * a table or a formula would be: "3 1/2" -> "3 and 1 half", "1/16/95" -> "January 16, 95", "Jan-95" -> "January 95",
 * "-5" -> "minus 5", "(5)" -> "minus 5", "1.5E-3" -> "1.5 times 10 to the minus 3". The line then goes to the output
 * unexpanded. Neither tts_Speak nor tts_SaveFile uses mode 2 (both pass 1), so in the DLL this is dead code.
 *
 * The fraction and date routines keep their buffers at the original's frame offsets (F + x), because long words
 * run from one buffer into the next. No fixes: nothing here is reachable.
 */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

static void rep(char *s, const char *find, const char *repl) { fe_str_replace_all(s, find, repl, s); }

/* strncat and strcat copying forward byte by byte, as the DLL's (a negative count copies everything). They stop at
   end, the end of the routine's frame: the original runs on into its caller's frame. */
static void ncat_to(char *d, const char *s, int n, const char *end)
{
    size_t c = (size_t)n;

    d += strlen(d);
    while (c-- && d < end - 1)
        if (!(*d++ = *s++)) return;
    *d = 0;
}
#define ncat(d, s, n) ncat_to(d, s, n, end)
#define cat(d, s) ncat_to(d, s, -1, end)

static void trim(char *s)
{
    char *r = fe_str_trim_spaces(s);
    if (r != s) strcpy(s, r);
}

/* "a/b" (the first '/' of the line): the numerator is the word before the '/' (after a '-' in it), the denominator
   the word after it without its final ",.;:!?". 1-19 become "half", "third" ... "nineteenth", 20, 30 ... 90
   "twentieth" ..., other two-digit ones "20 first" ..., longer ones stay "/n"; an "s" follows unless the numerator is
   1 or the denominator has three digits or more. A blank in front of the fraction becomes " and ": the first blank
   of the line, so "I ate 1/2" reads "I and ate 1 half". A numerator or denominator that is 0 leaves the line alone.
   @0x10045320 */
void fe_NumLine_Fraction(TextProc *t)
{
    char F[0x424 + 0x400];
    char *text = t->text, *c1 = F + 0x16, *den = F + 0x20, *buf = F + 0x5c, *tmp = F + 0x158, *punct = F + 0x254;
    char *tens = F + 0x290, *ords = F + 0x2ec, *fracs = F + 0x350;
    short num, slash, i, k, n, d, v, sp;

    const char *end = F + sizeof F;

    memset(F, 0, sizeof F);
    memcpy(fracs, fe_nl_fractions, sizeof fe_nl_fractions);
    memcpy(ords, fe_nl_ordinals, sizeof fe_nl_ordinals);
    memcpy(tens, fe_nl_tens, sizeof fe_nl_tens);
    strcpy(buf, " ");
    cat(buf, text);
    cat(buf, " ");
    slash = fe_strpos(text, "/");        /* the '/' is buf[slash] */
    i = slash - 2;
    while (i >= 0 && buf[i] != ' ') i--;
    tmp[0] = 0;
    ncat(tmp, buf + i + 1, slash - i - 1);
    k = fe_strpos(tmp, "-");
    num = (short)atoi(k ? tmp + k : tmp);
    if (num == 0) return;
    k = slash + 1;
    while ((short)strlen(buf) >= k && buf[k] != ' ') k++;
    tmp[0] = 0;
    ncat(tmp, buf + slash + 1, k - slash - 1);
    punct[0] = 0;
    for (n = (short)strlen(tmp); n >= 1; ) {
        c1[0] = 0;
        ncat(c1, tmp + n - 1, 1);
        if (c1[0] != ',' && c1[0] != '.' && c1[0] != ';' && c1[0] != ':' && c1[0] != '!' && c1[0] != '?') break;
        n--;
        cat(punct, c1);
    }
    den[0] = 0;
    ncat(den, tmp, (int)(strlen(tmp) - strlen(punct)));
    d = (short)atoi(den);
    if (d == 0) return;
    if (strlen(den) == 1 || (strlen(den) == 2 && den[0] == '1')) {
        buf[0] = 0;
        ncat(buf, fracs + (d - 1) * 11, 11);
    } else if (strlen(den) == 2) {
        tmp[1] = 0;
        if (den[1] == '0') {
            tmp[0] = den[0];
            v = (short)atoi(tmp);
            buf[0] = 0;
            ncat(buf, tens + (v - 1) * 10, 10);
        } else {
            tmp[0] = den[1];
            v = (short)atoi(tmp);    /* 0 for a letter: ords[-11], the end of the tens table ("inetieth ") */
            buf[0] = den[0];
            buf[1] = '0';
            buf[2] = ' ';
            buf[3] = 0;
            ncat(buf, ords + (v - 1) * 11, 11);
        }
    } else {
        buf[0] = '/';
        buf[1] = 0;
        cat(buf, den);
    }
    trim(buf);
    if (num != 1 && strlen(den) <= 2) cat(buf, "s");
    strcpy(den, buf);
    cat(den, punct);
    buf[0] = 0;
    sp = fe_strpos(text, " ");
    if (sp > 0 && sp < slash) {
        ncat(buf, text, sp);
        cat(buf, "and ");
        ncat(buf, text + sp, slash - sp - 1);
    } else {
        ncat(buf, text, slash - 1);
    }
    cat(buf, " ");
    cat(buf, den);
    if ((short)strlen(text) > k) cat(buf, text + k - 1);
    strcpy(text, buf);
    rep(text, " and -", " -");
    rep(text, "- and ", "- ");
}

/* The first of "Jan" ... "Dec" in the line becomes the month's name (every "X" of the line too), and every '-' a
   blank: "Jan-95" -> "January 95". @0x100459e0 */
void fe_NumLine_MonthAbbrev(TextProc *t)
{
    char F[0xb0];
    char *abbr = F + 0x10, *name = F + 0x18;
    const char *end = F + sizeof F;
    short m;

    for (m = 1; m <= 12; m++) {
        abbr[0] = 0;
        ncat(abbr, fe_nl_months3 + 3 * (m - 1), 3);
        if (fe_strpos(t->text, abbr)) break;
    }
    if (m > 12) return;
    name[0] = 0;
    ncat(name, fe_nl_months + 9 * (m - 1), 9);
    trim(name);
    rep(t->text, abbr, "X");
    rep(t->text, "X", name);
    rep(t->text, "-", " ");
}

/* Every '-' becomes " minus ". @0x10045af0 */
void fe_NumLine_Minus(TextProc *t)
{
    char *r;

    rep(t->text, "-", " minus ");
    rep(t->text, "  ", " ");
    r = fe_str_trim_leading(t->text);
    if (r != t->text) strcpy(t->text, r);
}

/* A number in parentheses is negative: "(" becomes "minus ", ")" goes. @0x10045b50 */
void fe_NumLine_Parens(TextProc *t)
{
    rep(t->text, "(", "minus ");
    rep(t->text, ")", "");
    rep(t->text, "  ", " ");
}

/* "E+", "E-" and "E minus " become " times 10 to the (minus) ". @0x10045b90 */
void fe_NumLine_Exponent(TextProc *t)
{
    rep(t->text, "E+", " times 10 to the ");
    rep(t->text, "E-", " times 10 to the minus ");
    rep(t->text, "E minus ", " times 10 to the minus ");
}

/* "m/d/y" (the first '/' of the line): the word before the first '/' is the month, 1-12, and becomes its name, and
   every '/' after it ", ": "1/16/95" -> "January 16, 95". @0x10045bd0 */
void fe_NumLine_Date(TextProc *t)
{
    char F[0x180 + 0x400];
    char *text = t->text, *mm = F + 0x10, *buf = F + 0x14, *months = F + 0x110;
    short s, i, m;

    const char *end = F + sizeof F;

    memset(F, 0, sizeof F);
    memcpy(months, fe_nl_months, sizeof fe_nl_months);
    strcpy(buf, " ");
    cat(buf, text);
    s = fe_strpos(buf, "/");
    i = s - 2;
    while (i >= 0 && buf[i] != ' ') i--;
    mm[0] = 0;
    ncat(mm, buf + i + 1, s - i - 2);    /* mm is 4 bytes: a longer month runs on into buf */
    m = (short)atoi(mm);
    if (m < 1 || m > 12) return;
    buf[0] = 0;
    ncat(buf, text, i);
    ncat(buf, months + 9 * (m - 1), 9);
    trim(buf);
    cat(buf, " ");
    cat(buf, text + s - 1);
    fe_str_replace_all(buf, "/", ", ", text);
}

/* One line in mode 2 (para_buf; FrontEnd_ProcessText): a date if the line has two '/', else a fraction; then a '-'
   at the start or between digits is "minus", between a letter and anything or before a letter it joins a month
   abbreviation ("Jan-95"), after "E" and a digit it is an exponent; then "(n)" and "E+n"/"E-n". The line goes to
   the output as it is then. A blank line sets line_no to 0. @0x100450e0 */
void fe_NumLine_Read(TextProc *t)
{
    char *text = t->text, *line = t->para_buf;
    short nl = 0, p, d, e;
    char c;
    const char *end = text + sizeof t->text;

    strcpy(text, line);
    p = fe_strpos(text, "\n");
    if (p) {
        text[0] = 0;
        ncat(text, line, p - 1);
        nl = 1;
    }
    if (strlen(line) < 1 || strcmp(line, "\n") == 0) {
        t->line_no = 0;
        goto out;
    }
    p = fe_strpos(text, "/");
    if (p) {
        if (fe_strpos(text + p, "/")) {
            fe_NumLine_Date(t);
            goto out;
        }
        fe_NumLine_Fraction(t);
    }
    d = fe_strpos(text, "-");
    if (d) {
        if (d == 1) {
            fe_NumLine_Minus(t);
        } else {
            if ((short)strlen(text) == d) goto out;
            c = text[d - 2];
            if ((signed char)c >= 'A') {
                if (c == 'E' && d - 3 >= 0 &&
                    ((text[d - 3] >= '0' && text[d - 3] <= '9') || text[d - 3] == '.')) {
                    fe_NumLine_Exponent(t);
                } else {
                    fe_NumLine_MonthAbbrev(t);
                    goto out;
                }
            } else if ((signed char)text[d] >= 'A') {
                fe_NumLine_MonthAbbrev(t);
                goto out;
            } else {
                fe_NumLine_Minus(t);
            }
        }
    }
    if (fe_strpos(text, "(") && fe_strpos(text, ")")) fe_NumLine_Parens(t);
    e = fe_strpos(text, "E");
    if (e && (short)strlen(text) > e) {
        c = text[e];
        if (c == '+' || c == '-' || strncmp(text + e, " minus ", 7) == 0) fe_NumLine_Exponent(t);
    }
out:
    if (nl == 1) strcat(text, "\n");
    fe_Output_AppendText(t);
}
