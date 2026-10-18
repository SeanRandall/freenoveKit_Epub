/*
    frontend.c - what happens to text before the original engine sees it.

    The 2004 engine was written for ISO-8859-1 text typed at a shell. Under
    a screen reader it is handed anything at all, and three things in it are
    not safe with that:

    1. Bytes above 0x7F. charset.c's deaccent() calls abort() on any such
       byte that is not one of the accented letters it knows, and say.c
       reaches it when spelling. In a DLL inside NVDA, abort() takes the
       screen reader down. So the text is decoded from UTF-8 and made pure
       ASCII here: accented Latin letters lose their accents, typographic
       quotes and dashes become their ASCII forms, everything else becomes
       a space. Nothing above 0x7F reaches the engine.

    2. '[' and ']'. say.c reads "[...]" as literal SAMPA phones, which is a
       feature at a shell and an accident in a web page. User brackets
       become spaces; this file uses the syntax itself, for ordinals.

    3. Digit runs. say.c accumulates digits into a `long`, which is 32 bits
       on Windows and 64 on Linux, and overflow is undefined. A run of more
       than nine digits is read digit by digit instead, which keeps every
       value below 10^9 on every platform.

    And two things the original reads badly, both about numbers:

    - "1,234,567" reached say.c as "1", ",", "234", ... and was read as
      three numbers. Thousands separators between well-formed groups of
      three are removed.
    - "21st" was read as "twenty one" and then "st" spelt as letters. An
      ordinal suffix now turns the number into the ordinal, by asking the
      English front end's own ordinal() for the phones and handing them on
      inside brackets.

    docs/04-front-end.md records the cases and the tests that pin them.

    Copyright (c) 2026 the rsynth-nvda contributors.
    Licensed under the GNU Library General Public License, version 2 or
    (at your option) any later version.
*/
#include "config.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "useconfig.h"
#include "darray.h"
#include "lang.h"
#include "rs_internal.h"

/* --- a growable byte buffer -------------------------------------------- */

typedef struct {
    char *p;
    size_t n, cap;
    int oom;
} buf_t;

static void
put(buf_t *b, const char *s, size_t n)
{
    if (b->oom)
        return;
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 256;
        char *p;
        while (b->n + n + 1 > cap)
            cap *= 2;
        p = (char *) realloc(b->p, cap);
        if (!p) {
            b->oom = 1;
            return;
        }
        b->p = p;
        b->cap = cap;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = '\0';
}

static void
putc_(buf_t *b, char c)
{
    put(b, &c, 1);
}

static void
puts_(buf_t *b, const char *s)
{
    put(b, s, strlen(s));
}

/* --- UTF-8 to ASCII ----------------------------------------------------- */

/* U+00C0..U+00FF. "" means a space. */
static const char *const latin1[64] = {
    "A", "A", "A", "A", "A", "A", "AE", "C",       /* C0 */
    "E", "E", "E", "E", "I", "I", "I", "I",        /* C8 */
    "D", "N", "O", "O", "O", "O", "O", "",         /* D0, D7 times */
    "O", "U", "U", "U", "U", "Y", "Th", "ss",      /* D8 */
    "a", "a", "a", "a", "a", "a", "ae", "c",       /* E0 */
    "e", "e", "e", "e", "i", "i", "i", "i",        /* E8 */
    "d", "n", "o", "o", "o", "o", "o", "",         /* F0, F7 divide */
    "o", "u", "u", "u", "u", "y", "th", "y",       /* F8 */
};

/* U+0100..U+017F, one letter each; '?' marks the digraphs handled below. */
static const char latin_ext_a[129] =
    "AaAaAaCcCcCcCcDd"      /* 0100 */
    "DdEeEeEeEeEeGgGg"      /* 0110 */
    "GgGgHhHhIiIiIiIi"      /* 0120 */
    "Ii??JjKkkLlLlLlL"      /* 0130: 0132 IJ, 0133 ij */
    "lLlNnNnNnnNnOoOo"      /* 0140 */
    "Oo??RrRrRrSsSsSs"      /* 0150: 0152 OE, 0153 oe */
    "SsTtTtTtUuUuUuUu"      /* 0160 */
    "UuUuWwYyYZzZzZzs";     /* 0170 */

/* One code point to ASCII. Returns the replacement, "" to drop it, or
   NULL for "a space". */
static const char *
ascii_for(unsigned long cp, char *one)
{
    if (cp < 0x80) {
        one[0] = (char) cp;
        one[1] = '\0';
        return one;
    }
    if (cp >= 0xC0 && cp <= 0xFF)
        return latin1[cp - 0xC0][0] ? latin1[cp - 0xC0] : NULL;
    if (cp >= 0x100 && cp <= 0x17F) {
        switch (cp) {
        case 0x132: return "IJ";
        case 0x133: return "ij";
        case 0x152: return "OE";
        case 0x153: return "oe";
        }
        one[0] = latin_ext_a[cp - 0x100];
        one[1] = '\0';
        return one;
    }
    switch (cp) {
    case 0x00A1: return "!";
    case 0x00BF: return "?";
    case 0x00AB: case 0x00BB:
    case 0x201C: case 0x201D: case 0x201E: case 0x201F: case 0x2033:
        return "\"";
    case 0x00B4:
    case 0x2018: case 0x2019: case 0x201A: case 0x201B: case 0x2032:
        return "'";
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014:
    case 0x2015: case 0x2212:
        return "-";
    case 0x2026:
        return "...";
    case 0x200B: case 0x200C: case 0x200D: case 0x2060: case 0xFEFF:
    case 0x00AD:                    /* soft hyphen */
        return "";
    }
    return NULL;
}

/* Decode one UTF-8 sequence. Malformed input yields U+FFFD and consumes
   one byte, so decoding always moves forward. */
static unsigned long
utf8_next(const unsigned char **ps)
{
    const unsigned char *s = *ps;
    unsigned long cp;
    int extra, i;
    if (s[0] < 0x80) {
        *ps = s + 1;
        return s[0];
    }
    if ((s[0] & 0xE0) == 0xC0) {
        cp = s[0] & 0x1F;
        extra = 1;
    } else if ((s[0] & 0xF0) == 0xE0) {
        cp = s[0] & 0x0F;
        extra = 2;
    } else if ((s[0] & 0xF8) == 0xF0) {
        cp = s[0] & 0x07;
        extra = 3;
    } else {
        *ps = s + 1;
        return 0xFFFD;
    }
    for (i = 1; i <= extra; i++) {
        if ((s[i] & 0xC0) != 0x80) {
            *ps = s + 1;
            return 0xFFFD;
        }
        cp = (cp << 6) | (s[i] & 0x3F);
    }
    *ps = s + extra + 1;
    return cp;
}

static void
to_ascii(buf_t *out, const char *utf8, int spell)
{
    const unsigned char *s = (const unsigned char *) utf8;
    while (*s) {
        char one[2];
        unsigned long cp = utf8_next(&s);
        const char *r = ascii_for(cp, one);
        if (r && r[0] && !r[1]) {
            unsigned char c = (unsigned char) r[0];
            /* Control characters and DEL have no names worth saying. */
            if (c < 0x20 || c == 0x7F)
                r = NULL;
            else if (!spell && (c == '[' || c == ']'))
                r = NULL;
        }
        if (r)
            puts_(out, r);
        else
            putc_(out, ' ');
    }
}

/* --- numbers ------------------------------------------------------------ */

/* Longest digit run read as one number: 999,999,999 fits a 32-bit long. */
#define MAX_DIGITS 9

static int
is_ordinal_suffix(const char *s)
{
    char a = (char) tolower((unsigned char) s[0]);
    char b = a ? (char) tolower((unsigned char) s[1]) : '\0';
    if (!a || !b || isalnum((unsigned char) s[2]))
        return 0;
    return (a == 's' && b == 't') || (a == 'n' && b == 'd')
        || (a == 'r' && b == 'd') || (a == 't' && b == 'h');
}

static void
numbers(buf_t *out, const char *s)
{
    size_t i = 0;
    while (s[i]) {
        unsigned char c = (unsigned char) s[i];
        unsigned char prev = i ? (unsigned char) s[i - 1] : ' ';
        if (!isdigit(c) || isdigit(prev)) {
            putc_(out, (char) c);
            i++;
            continue;
        }

        /* The digits after a decimal point: say.c spells them out one by
           one already, so they are left exactly as they are. */
        if (prev == '.' && i >= 2 && isdigit((unsigned char) s[i - 2])) {
            while (isdigit((unsigned char) s[i]))
                putc_(out, s[i++]);
            continue;
        }

        {
            char digits[64];
            size_t nd = 0, j = i;
            int standalone = !isalpha(prev);
            while (isdigit((unsigned char) s[j])) {
                if (nd < sizeof(digits) - 1)
                    digits[nd] = s[j];
                nd++;
                j++;
            }
            /* Thousands separators: 1-3 digits, then ",ddd" groups. */
            if (standalone && nd <= 3) {
                while (s[j] == ',' && isdigit((unsigned char) s[j + 1])
                       && isdigit((unsigned char) s[j + 2])
                       && isdigit((unsigned char) s[j + 3])
                       && !isdigit((unsigned char) s[j + 4])) {
                    if (nd + 3 < sizeof(digits)) {
                        memcpy(digits + nd, s + j + 1, 3);
                    }
                    nd += 3;
                    j += 4;
                }
            }
            if (nd > MAX_DIGITS) {
                /* Too long to be one number: read it digit by digit. A
                   run longer than the buffer was not collected, so it is
                   taken from the source; it has no separators in it. */
                size_t k;
                if (nd < sizeof(digits)) {
                    for (k = 0; k < nd; k++) {
                        if (k)
                            putc_(out, ' ');
                        putc_(out, digits[k]);
                    }
                } else {
                    for (k = i; k < j; k++) {
                        if (k > i)
                            putc_(out, ' ');
                        putc_(out, s[k]);
                    }
                }
                i = j;
                continue;
            }
            digits[nd] = '\0';
            if (standalone && is_ordinal_suffix(s + j)) {
                darray_t phone;
                long value = strtol(digits, NULL, 10);
                darray_init(&phone, sizeof(char), 64);
                xlate_ordinal(value, &phone);
                putc_(out, '[');
                if (phone.items)
                    put(out, (const char *) darray_find(&phone, 0), phone.items);
                putc_(out, ']');
                darray_free(&phone);
                i = j + 2;
                continue;
            }
            puts_(out, digits);
            i = j;
        }
    }
}

/* --- entry point -------------------------------------------------------- */

char *
rs_frontend(const char *utf8, int spell)
{
    buf_t ascii = { 0 }, out = { 0 };
    to_ascii(&ascii, utf8 ? utf8 : "", spell);
    putc_(&ascii, '\0');            /* make sure the buffer exists */
    if (ascii.oom)
        return NULL;
    ascii.n--;
    if (spell)
        return ascii.p;
    numbers(&out, ascii.p);
    free(ascii.p);
    putc_(&out, '\0');
    if (out.oom) {
        free(out.p);
        return NULL;
    }
    out.n--;
    return out.p;
}
