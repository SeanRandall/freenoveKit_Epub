#include "openbst_numbers.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *const ones[] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
    "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
    "seventeen", "eighteen", "nineteen",
};
static const char *const tens[] = {
    "", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety",
};

typedef struct { char *p; char *end; } sink_t;

static void put(sink_t *s, const char *w)
{
    while (*w && s->p < s->end) *s->p++ = *w++;
}

static void put_word(sink_t *s, const char *w)
{
    if (s->p > s->end - 1) return;
    put(s, w);
    put(s, " ");
}

/* 1..999 as words (no "zero"). */
static void under_thousand(sink_t *s, unsigned n)
{
    if (n >= 100) {
        put_word(s, ones[n / 100]);
        put_word(s, "hundred");
        n %= 100;
    }
    if (n >= 20) {
        put_word(s, tens[n / 10]);
        n %= 10;
        if (n) put_word(s, ones[n]);
    } else if (n) {
        put_word(s, ones[n]);
    }
}

static void spell_number(sink_t *s, const char *digits, size_t len)
{
    /* Leading zero ("007") or very long (phone numbers): digit by digit. */
    if (len > 9 || (len > 1 && digits[0] == '0')) {
        for (size_t i = 0; i < len; i++) put_word(s, ones[digits[i] - '0']);
        return;
    }
    /* Four digits are said in pairs, as the original BeST does (its NVDA add-on): 8086 "eighty
     * eighty six", 2026 "twenty twenty six", 1905 "nineteen oh five", 2020 "twenty twenty". Ending
     * in two zeros, as hundreds: 1900 "nineteen hundred", 1200 "twelve hundred", but 2000 "two
     * thousand". Any other two zeros side by side: one number, 2007 "two thousand seven", 1005. */
    if (len == 4 && digits[2] == '0' && digits[3] == '0' && digits[1] != '0') {
        under_thousand(s, (unsigned)((digits[0] - '0') * 10 + (digits[1] - '0')));
        put_word(s, "hundred");
        return;
    }
    if (len == 4) {
        bool double_zero = false;
        for (size_t i = 0; i + 1 < 4; i++) double_zero |= digits[i] == '0' && digits[i + 1] == '0';
        if (!double_zero) {
            under_thousand(s, (unsigned)((digits[0] - '0') * 10 + (digits[1] - '0')));
            if (digits[2] == '0') { put_word(s, "oh"); put_word(s, ones[digits[3] - '0']); }
            else under_thousand(s, (unsigned)((digits[2] - '0') * 10 + (digits[3] - '0')));
            return;
        }
    }
    unsigned long n = 0;
    for (size_t i = 0; i < len; i++) n = n * 10 + (unsigned)(digits[i] - '0');
    if (n == 0) { put_word(s, "zero"); return; }
    unsigned millions = (unsigned)(n / 1000000UL);
    unsigned thousands = (unsigned)((n / 1000) % 1000);
    unsigned rest = (unsigned)(n % 1000);
    if (millions) { under_thousand(s, millions); put_word(s, "million"); }
    if (thousands) { under_thousand(s, thousands); put_word(s, "thousand"); }
    if (rest) under_thousand(s, rest);
}

void openbst_spell_numbers(const char *in, char *out, size_t out_len)
{
    if (out_len == 0) return;
    sink_t s = { out, out + out_len - 1 };
    for (const char *p = in; *p && s.p < s.end;) {
        if (*p == '-' && isdigit((unsigned char)p[1]) && (p == in || p[-1] == ' ')) {
            if (s.p > out && s.p[-1] != ' ') put(&s, " ");   /* "-5" at a word start is "minus five" */
            put(&s, "minus ");
            p++;
        }
        if (isdigit((unsigned char)*p)) {
            const char *q = p;
            while (isdigit((unsigned char)*q)) q++;
            if (s.p > out && s.p[-1] != ' ') put(&s, " ");
            spell_number(&s, p, (size_t)(q - p));
            /* A decimal point between digits: "5.5" is "five point five", the digits after it one at a
               time ("3.14": "three point one four"). Before, each side was a number of its own and the
               dot a full stop: "five. five". */
            while (*q == '.' && isdigit((unsigned char)q[1])) {
                put_word(&s, "point");
                for (q++; isdigit((unsigned char)*q); q++) put_word(&s, ones[*q - '0']);
            }
            if (s.p > out && s.p[-1] == ' ') s.p--;          /* no space before punctuation */
            if (isalnum((unsigned char)*q)) put(&s, " ");    /* "5th" -> "five th" */
            p = q;
        } else if (*p == '.' && p > in && isalnum((unsigned char)p[-1]) && isalnum((unsigned char)p[1]) &&
                   (isalpha((unsigned char)p[-1]) || isalpha((unsigned char)p[1]))) {
            /* A dot inside a word ("notes.txt", "feeds.npr.org"): BeST says nothing at all for the
             * whole utterance if it meets one, so it is said as "dot". Single letters on both
             * sides ("e.g.", "U.S.") are abbreviations and stay as they are. */
            size_t before = 0, after = 0;
            for (const char *q = p - 1; q >= in && isalnum((unsigned char)*q); q--) before++;
            for (const char *q = p + 1; isalnum((unsigned char)*q); q++) after++;
            if (before == 1 && after == 1) *s.p++ = *p++;
            else { put(&s, " dot "); p++; }
        } else {
            *s.p++ = *p++;
        }
    }
    *s.p = 0;
}
