/* Email stage: the "Date:" header line (patent header/body analyser 134, REFERENCE 8.4). Written from the listings.

   Email_DateLine rewrites para_buf into spoken form and records the mail system its date style shows in mail_system:
     "12/25/94 10:15" (cc:Mail, "ccmail")        -> Email_DateMDY, the numeric month/day/2-digit-year form
     "12-25-94 10:15" (Lotus Notes, "lnmail")    -> Email_DateMDY
     "1994/12/25 10:15" (MS Mail, "msmail")      -> Email_DateYMD, the numeric year/month/day form
     anything else (RFC 822 "Fri, 27 May 94 10:15:00 PST", the long form "Friday, May 27, 1994 10:15AM"): the
     line is upper-cased and day names, month names, times and time zones are rewritten in place.
   Email_OrdinalSuffix makes a day number an ordinal ("27" -> "27th"); the engine reads "27th" as "twenty-seventh".

   Every function keeps its locals in a byte array laid out like the original stack frame (byte x = [F+x] of
   tools/asm_frame.py), with the tables the original copies onto its stack at entry at the same offsets, so that
   buffer run-ons and out-of-range table indexes read and write what the original does. With fe_fixes the text
   buffers are separate and large. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

#pragma GCC diagnostic ignored "-Wrestrict"

static const char MONTHS3[] = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
static const char MONTHS[] =
    "January  February March    April    May      June     July     August   SeptemberOctober  November December ";
static const char DAYS3[] = "SUNMONTUEWEDTHUFRISAT";
static const char DAYS[] = "Sunday   Monday   Tuesday  WednesdayThursday Friday   Saturday ";

#define FBUF 2048   /* fe_fixes: size of each text buffer */

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

static char *trim(char *s) { return fe_str_trim_spaces(s); }

/* d = "" + at most n characters of s (the original's strcpy(d, "") + strncat(d, s, n); a negative n copies all) */
static void take(char *d, const char *s, int n)
{
    d[0] = 0;
    strncat(d, s, (size_t)n);
}

/* ---- Email_OrdinalSuffix -------------------------------------------------------------------------------------- */
/* s is a day number: "1" -> "1st", "22" -> "22nd", "13" -> "13th", "05" -> "5th". Nothing happens when atoi(s), as a
   short, is 0 or above 31. The suffix is chosen by the last character alone: "st"/"nd"/"rd" for 1/2/3 (not 11-13),
   "th" for 0 and for anything that sorts at or above '4' (letters too: "5a" -> "5ath"). When the last character is
   below '4' and not a digit ("5,", "5 "), the original never writes its 4-byte suffix buffer and appends what is
   there up to a NUL: the stale bytes, then its return address and the argument (a stack address, whose high byte
   is 0). With fe_fixes nothing is appended. @0x10074c50 */
void fe_Email_OrdinalSuffix(TextProc *t, char *s)
{
    const unsigned char *fr = fe_frame_of(0x10074c50);   /* [F+x] of the listing is fr[x - 4] */
    char last[2], sfx[16];
    short n = (short)tv_atoi(s);
    size_t l;

    (void)t;
    if (n == 0 || n > 31) return;
    memset(sfx, 0, sizeof sfx);
    if (fr) {
        memcpy(sfx, fr + 0x10, 4);                  /* [F+0x14] */
        if (!memchr(sfx, 0, 4))                     /* then the return address (under the harness, its exit stub) */
            memcpy(sfx + 4, fr + 0x14, 4);
    }
    l = strlen(s);
    last[0] = s[l - 1];
    last[1] = 0;
    if (l == 2 && s[0] == '1')
        strcpy(sfx, "th");
    else {
        if (!strcmp(last, "1")) strcpy(sfx, "st");
        if (!strcmp(last, "2")) strcpy(sfx, "nd");
        if (!strcmp(last, "3")) strcpy(sfx, "rd");
        if (strcmp(last, "4") >= 0 || !strcmp(last, "0")) strcpy(sfx, "th");
    }
    if (l == 2 && s[0] == '0') memmove(s, s + 1, 2);   /* "05" -> "5" */
    strcat(s, sfx);
}

/* ---- Email_DateMDY / Email_DateYMD -------------------------------------------------------------------- */
#define FA_FRAME 0x1b4
/* DateYMD's month index is not checked: an index past the table reads on into its return address, its arguments
   and Email_DateLine's frame, which sits 0x1c0 bytes above its own. The port keeps DateLine's frame here meanwhile. */
static const unsigned char *dl_cur;
static unsigned char fa_frame[0x1c0 + 0x338 + 0x800 + 5 * FBUF] __attribute__((aligned(16)));

/* The rest of the line after the date: the original takes para_buf + k even when k is past the end of the string
   and reads the stale bytes there. With fe_fixes it is the end of the string. */
static const char *rest(const char *pb, int k)
{
    int l = (int)strlen(pb);
    return fe_fixes && k > l ? pb + l : pb + k;
}

/* fe_fixes: a year of two digits yy is 20yy below 50 and 19yy above (RFC 2822 4.3); the original always writes 19 */
static const char *century(const char *yy)
{
    if (fe_fixes && yy[0] >= '0' && yy[0] <= '4' && yy[1] >= '0' && yy[1] <= '9') return "20";
    return "19";
}

/* The numeric month/day/year date: sep1 (a) is the position of the first separator, sep2 (b) of the second
   (1-based); "12/25/94 10:15" -> "December 25th, 1994 at 10:15". The month is the 1 or 2 characters before a, the day
   is between a and b (1 to 31, made an ordinal), the year is the 2 characters after b and always gets "19" (with
   fe_fixes "20" for 00-49). " at" and the text from b + 2 on follow; "PM"/"AM" become "p.m."/"a.m.". A month outside
   1-12 or a day outside 1-31 leaves the line as it is, and so does a line longer than 225 characters (which
   FrontEnd_ProcessText's line split does not let through). A 4-digit year loses its last two digits: "10/21/1994" ->
   "October 21st, 1919 at94"; with fe_fixes the whole year is used. Called for cc:Mail ('/') and Lotus Notes ('-')
   dates. @0x100741a0 */
void fe_Email_DateMDY(TextProc *t, short sep1, short sep2)
{
    const unsigned char *fr = fe_frame_of(0x100741a0);
    unsigned char *F = fa_frame;
    char *pb = t->para_buf;
    char *B14, *B28, *B30, *B38, *B48;
    short a = sep1, b = sep2, bp, w12, dx;
    int d, y4 = 0;

    memset(F, 0, 0x1c0 + 0x338 + 0x800);
    if (fr) memcpy(F, fr, FA_FRAME);
    memcpy(F + 0x144, MONTHS, sizeof MONTHS);
    if (fe_fixes) {
        B14 = (char *)F + 0x1c0 + 0x338 + 0x800; B28 = B14 + FBUF; B30 = B28 + FBUF; B38 = B30 + FBUF;
        B48 = B38 + FBUF;
    } else {
        B14 = (char *)F + 0x14; B28 = (char *)F + 0x28; B30 = (char *)F + 0x30; B38 = (char *)F + 0x38;
        B48 = (char *)F + 0x48;
    }

    if ((short)strlen(pb) > 0xe1) return;
    bp = a;
    if (!strncmp(pb + a - 3, " ", 1)) bp -= 1;          /* " 5/": one digit */
    else bp -= 2;
    w12 = (short)(b + 2);
    take(B28, pb + bp - 1, a - bp);                      /* month */
    take(B14, pb + a, b - a - 1);                        /* day */
    d = tv_atoi(B14);
    if (d > 31 || d < 1) return;
    fe_Email_OrdinalSuffix(t, B14);
    take(B30, pb + b, 2);                                /* year */
    if (fe_fixes && strlen(B30) == 2 && pb[b + 2] >= '0' && pb[b + 2] <= '9' && pb[b + 3] >= '0' && pb[b + 3] <= '9')
        y4 = 1, take(B30, pb + b, 4), w12 += 2;
    dx = (short)tv_atoi(B28);
    if (dx > 12 || dx < 1) return;
    take(B38, (char *)F + 0x13b + 9 * dx, 9);
    strcpy(B38, trim(B38));
    take(B48, pb, bp - 1);
    strcat(B48, B38);
    strcat(B48, " ");
    strcat(B48, B14);
    strcat(B48, ", ");
    if (!y4) strcat(B48, century(B30));
    strcat(B48, B30);
    strcat(B48, " at");
    strcat(B48, rest(pb, w12));
    strcpy(pb, trim(B48));
    fe_str_replace_all(pb, "PM", "p.m.", pb);
    fe_str_replace_all(pb, "AM", "a.m.", pb);
}

/* The numeric year/month/day date: sep1 (a) is the position of the first '/', sep2 (b) of the second (1-based);
   "1994/12/25 10:15" -> "December 25th, 1994 at 10:15". The year is the 4 characters before a, the month is
   between a and b, the day the 1 or 2 characters after b (2 unless a space or tab follows the first); the day is made
   an ordinal (a leading 0 goes). " at " and the text after the day follow; "PM"/"AM" become "p.m."/"a.m.". A line
   longer than 225 characters is left as it is. The month is not checked: 0 or 13 and above index past the month
   table into the stack (with fe_fixes the line is left as it is); the day is not checked either ("May 45, 1994").
   Called for MS Mail dates, which Email_DateLine recognises by a "19" or "20" four characters before the first '/'.
   @0x100745e0 */
void fe_Email_DateYMD(TextProc *t, short sep1, short sep2)
{
    const unsigned char *fr = fe_frame_of(0x100745e0);
    unsigned char *F = fa_frame;
    char *pb = t->para_buf;
    char *B18, *B28, *B30, *B38, *B48;
    short a = sep1, b = sep2, bp, si, dx;
    uint32_t v;

    memset(F, 0, 0x1c0 + 0x338 + 0x800);
    if (fr) memcpy(F, fr, FA_FRAME);
    memcpy(F + 0x144, MONTHS, sizeof MONTHS);
    /* above the frame: the return address into Email_DateLine, the arguments, then DateLine's frame */
    v = 0x10072eef; memcpy(F + 0x1b4, &v, 4);
    v = (uint16_t)a; memcpy(F + 0x1b8, &v, 4);
    v = (uint16_t)b; memcpy(F + 0x1bc, &v, 4);
    if (dl_cur) memcpy(F + 0x1c0, dl_cur, 0x338);
    if (fe_fixes) {
        B18 = (char *)F + 0x1c0 + 0x338 + 0x800; B28 = B18 + FBUF; B30 = B28 + FBUF; B38 = B30 + FBUF;
        B48 = B38 + FBUF;
    } else {
        B18 = (char *)F + 0x18; B28 = (char *)F + 0x28; B30 = (char *)F + 0x30; B38 = (char *)F + 0x38;
        B48 = (char *)F + 0x48;
    }

    if ((short)strlen(pb) > 0xe1) return;
    bp = (short)(a - 4);
    si = b;
    if (!strncmp(pb + b + 1, " ", 1) || !strncmp(pb + b + 1, "\t", 1)) si += 2;
    else si += 3;
    take(B30, pb + bp - 1, a - bp);                      /* year */
    take(B28, pb + a, b - a - 1);                        /* month */
    take(B18, pb + b, si - b - 1);                       /* day */
    fe_Email_OrdinalSuffix(t, B18);
    dx = (short)tv_atoi(B28);
    if (fe_fixes && (dx < 1 || dx > 12)) return;
    take(B38, (char *)F + 0x13b + 9 * dx, 9);
    strcpy(B38, trim(B38));
    if (!strncmp(B18, "0", 1)) {
        B48[0] = 0;
        strcat(B48, B18 + 1);
        strcpy(B18, B48);
    }
    take(B48, pb, bp - 1);
    strcat(B48, B38);
    strcat(B48, " ");
    strcat(B48, B18);
    strcat(B48, ", ");
    strcat(B48, B30);
    strcat(B48, " at ");
    strcat(B48, rest(pb, si));
    strcpy(pb, trim(B48));
    fe_str_replace_all(pb, "PM", "p.m.", pb);
    fe_str_replace_all(pb, "AM", "a.m.", pb);
}

/* ---- Email_DateLine ------------------------------------------------------------------------------------------- */
#define DL_FRAME 0x338
static unsigned char dl_frame[DL_FRAME + 0x800 + 2 * FBUF] __attribute__((aligned(16)));

static int one_of(const char *c, const char *set) { return c[0] && !c[1] && strchr(set, c[0]); }
static int alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
static int digit(char c) { return c >= '0' && c <= '9'; }

/* fe_fixes: " dd00" / " 00" at k is removed only after the time (a numeric zone), not a year "1900" or the hour of
   "00:00:00" */
static int after_time(const char *pb, int k)
{
    const char *c = strchr(pb + 6, ':');
    return c && c - pb < k;
}

/* fe_fixes: "xST" / "xDT" is spelled "x-S-T" only as a word of its own (an unknown zone), not inside "21ST",
   "AUGUST", "TEST" */
static void spell_zone(char *pb, char z)
{
    char buf[1100], *p;
    for (p = pb; (p = strchr(p, z)) != NULL; p++)
        if (p[1] == 'T' && p - pb >= 1 && alpha(p[-1]) && (p - pb < 2 || (!alpha(p[-2]) && !digit(p[-2])))
            && !alpha(p[2]) && !digit(p[2]) && strlen(pb) + 2 < sizeof buf) {
            strcpy(buf, p + 2);
            p[0] = '-'; p[1] = z; p[2] = '-'; p[3] = 'T';
            strcpy(p + 4, buf);
            p += 3;
        }
}

/* fe_fixes: the day after the month ("MAY 27, 1994", "AUG 21ST 1995"): its 1-2 digits go to day, and the number of
   characters to skip (the day, an ordinal suffix, a comma, spaces) is returned; 0 when there is none */
static int day_after(const char *s, char *day)
{
    int n = 0, k;
    while (n < 2 && digit(s[n])) n++;
    if (!n || digit(s[n])) return 0;
    k = n;
    if ((s[k] == 'S' && s[k + 1] == 'T') || (s[k] == 'N' && s[k + 1] == 'D') || (s[k] == 'R' && s[k + 1] == 'D')
        || (s[k] == 'T' && s[k + 1] == 'H'))
        k += 2;
    if (s[k] == ',') k++;
    if (s[k] != ' ' && s[k] != '\n' && s[k]) return 0;
    if (tv_atoi(s) < 1 || tv_atoi(s) > 31) return 0;
    memcpy(day, s, (size_t)n);
    day[n] = 0;
    while (s[k] == ' ') k++;
    return k;
}

/* "at " before the time; the original drops the space in front of a one-digit hour ("1995at 9:05"), fe_fixes keeps
   one */
static void at(char *b)
{
    size_t l = strlen(b);
    if (fe_fixes && l && b[l - 1] != ' ') strcat(b, " ");
    strcat(b, "at ");
}

/* The "Date:" line in para_buf. Numeric dates go to DateMDY/YMD (see the top of the file) and set mail_system to the mail
   system. Every other line, in this order:
   - '-' becomes ' ' and the line is upper-cased;
   - a line ending in ']' (only the last line of a message, which has no "\n"; the test meant ')' too, but it only
     lets letters through) loses the last "(...", "[..." (not when the third last character is ':');
   - a line ending in "00\n" loses its last word (a numeric zone "-0700"; also the time "10:15:00" when no zone
     follows it; with no space in the line, a second "\n" is added);
   - the first " dd00" (a numeric zone, "+0100", or a year "1900") or " 00" is removed;
   - EDT CDT MDT PDT EST CST MST PST GMT become their names, any other "DT" / "ST" is spelled "-D-T" / "-S-T" (also
     inside words: "21ST", "AUGUST");
   - unless the line is longer than 238 characters: a day abbreviation (SUN..SAT, anywhere) becomes the day's name
     plus ',', replacing the word up to the next space or comma ("FRI," / "FRIDAY" -> "Friday,"); a month abbreviation
     after a space becomes the month's name, with the 1- or 2-digit day in front of it moved behind it as an
     ordinal and ", " after it ("27 MAY" / "MAY" -> "May 27th, "); a following year of 1-2 characters (or a short one
     at the end of the line) gets "19";
   - a time "hh:mm:ss" (the first ':' from position 17 on, also past the end of a shorter line, in the stale bytes
     there) gets "at " in front and loses the seconds; "hh:mm" gets "at " when digits are around the ':'; a one-digit
     hour loses the space in front ("1995at 9:05");
   - leading zeros after a space and every '#' go ("09:15" -> "9:15"), and "DATE" becomes "Date".
   "Fri, 27 May 94 10:15:00 PST" -> "Friday, May 27th, 1994 at 10:15 Pacific Standard Time". @0x10072c30
   With fe_fixes: the time is kept when a "00\n" last word holds a ':'; only a " dd00" / " 00" after the time is
   removed; "-S-T" / "-D-T" only for a 3-letter word; a 1-2 digit day after the month ("May 27, 1994") is taken as
   the day ("May 27th, 1994", not "May, 1927, 1994"); 2-digit years 00-49 get "20"; the space before "at" stays;
   no time is looked for past the end of a line shorter than 17. */
void fe_Email_DateLine(TextProc *t)
{
    const unsigned char *fr = fe_frame_of(0x10072c30);
    unsigned char *F = dl_frame;
    char *pb = t->para_buf;
    char *B12, *B16, *B24, *B40, *B48, *B50, *B18c;
    const char *q;
    short bx, p2, si, w20, k, di, ax, cx, dx, bpp;
    int e, L, esi;
    uint32_t v, self = (uint32_t)(uintptr_t)fe_frame_this;

    memset(F, 0, DL_FRAME + 0x800);
    if (fr) memcpy(F, fr, DL_FRAME);
    memcpy(F + 0x164, MONTHS3, sizeof MONTHS3);
    memcpy(F + 0x2c8, MONTHS, sizeof MONTHS);
    memcpy(F + 0x14c, DAYS3, sizeof DAYS3);
    memcpy(F + 0x288, DAYS, sizeof DAYS);
    v = self + 0x7d2; memcpy(F + 0x18, &v, 4);
    memcpy(F + 0x1c, &self, 4);
    B12 = (char *)F + 0x12; B16 = (char *)F + 0x16; B24 = (char *)F + 0x24; B40 = (char *)F + 0x40;
    B48 = (char *)F + 0x48;
    if (fe_fixes) {
        B50 = (char *)F + DL_FRAME + 0x800; B18c = B50 + FBUF;
    } else {
        B50 = (char *)F + 0x50; B18c = (char *)F + 0x18c;
    }

    /* numeric dates */
    bx = fe_strpos(pb, "/");
    if (bx) {
        v = self + (uint32_t)bx; memcpy(F + 0x20, &v, 4);
        p2 = (short)(bx + fe_strpos(pb + bx, "/"));
        e = bx - p2;
        if (e == -2 || e == -3) {
            take(B24, pb + bx - 5, 2);
            if (!strcmp(B24, "19") || !strcmp(B24, "20")) {       /* "1994/12/25" */
                strcpy(t->mail_system, "msmail");
                dl_cur = F;
                fe_Email_DateYMD(t, bx, p2);
                dl_cur = NULL;
                return;
            }
            take(B24, pb + bx - 4, 1);
            if (one_of(B24, " \t:")) {                            /* "12/25/94" */
                strcpy(t->mail_system, "ccmail");
                fe_Email_DateMDY(t, bx, p2);
                return;
            }
        }
    } else if ((bx = fe_strpos(pb, "-")) != 0) {
        v = self + (uint32_t)bx; memcpy(F + 0x20, &v, 4);
        p2 = (short)(bx + fe_strpos(pb + bx, "-"));
        e = bx - p2;
        if (e == -2 || e == -3) {
            take(B24, pb + bx - 4, 1);
            if (one_of(B24, " \t:")) {                            /* "12-25-94" */
                strcpy(t->mail_system, "lnmail");
                fe_Email_DateMDY(t, bx, p2);
                return;
            }
        }
    }

    fe_str_replace_all(pb, "-", " ", pb);
    fe_strupr(pb);

    /* a trailing "(...)" / "[...]" comment */
    L = (int)strlen(pb);
    take(B16, pb + L - 1, 1);
    if (strcmp(B16, "A") >= 0 && strcmp(B16, "Z") && strcmp(B16, " ") && strncmp(pb + L - 3, ":", 1)
        && (!strcmp(B16, ")") || !strcmp(B16, "]"))) {
        for (bx = (short)(strlen(pb) - 1); bx >= 1; bx--) {
            take(B12, pb + bx - 1, 1);
            if (!strcmp(B12, "(") || !strcmp(B12, "[")) {
                take(B50, pb, bx - 1);
                strcpy(pb, trim(B50));
                break;
            }
        }
    }

    /* a last word ending in "00" */
    for (si = (short)((short)strlen(pb) - 3); si >= 1; si--)
        if (!strncmp(pb + si - 1, " ", 1)) break;
    L = (int)strlen(pb);
    if (!strcmp(pb + L - 3, "00\n") && (!fe_fixes || (si >= 1 && !strchr(pb + si, ':')))) {
        take(B50, pb, si - 1);
        strcat(B50, "\n");
        strcpy(pb, B50);
    }

    /* the first " dd00" or " 00" */
    for (w20 = 0;;) {
        bx = fe_strpos(pb + w20, "00");
        if (bx <= 0) break;
        if (fe_fixes && !after_time(pb, w20 + bx - 1)) break;
        esi = w20;
        take(B16, pb + w20 + bx - 2, 1);
        if (B16[0] >= '0' && B16[0] <= '9') {
            bpp = (short)(bx - 1);
            take(B16, pb + w20 + bpp - 2, 1);
            if (B16[0] >= '0' && B16[0] <= '9') {
                bpp--;
                esi = w20 + bpp;
                take(B16, pb + esi - 2, 1);
                if (B16[0] == ' ') {
                    esi--;
                    take(B50, pb, esi);
                    strcat(B50, pb + w20 + bx + 1);
                    strcpy(pb, B50);
                    break;
                }
            }
        } else if (B16[0] == ' ') {
            esi = w20 + bx;
            take(B50, pb, esi - 1);
            strcat(B50, pb + esi + 1);
            strcpy(pb, B50);
            break;
        }
        w20 = (short)(bx + w20 + 2);
    }

    /* time zones */
    if (fe_strpos(pb, "DT")) {
        fe_str_replace_all(pb, "EDT", "Eastern Daylight Time", pb);
        fe_str_replace_all(pb, "CDT", "Central Daylight Time", pb);
        fe_str_replace_all(pb, "MDT", "Mountain Daylight Time", pb);
        fe_str_replace_all(pb, "PDT", "Pacific Daylight Time", pb);
        if (fe_fixes) spell_zone(pb, 'D');
        else fe_str_replace_all(pb, "DT", "-D-T", pb);
    }
    if (fe_strpos(pb, "ST")) {
        fe_str_replace_all(pb, "EST", "Eastern Standard Time", pb);
        fe_str_replace_all(pb, "CST", "Central Standard Time", pb);
        fe_str_replace_all(pb, "MST", "Mountain Standard Time", pb);
        fe_str_replace_all(pb, "PST", "Pacific Standard Time", pb);
        if (fe_fixes) spell_zone(pb, 'S');
        else fe_str_replace_all(pb, "ST", "-S-T", pb);
    }
    fe_str_replace_all(pb, "GMT", "Greenwich Mean Time", pb);

    if ((short)strlen(pb) <= 0xee) {
        /* day names */
        for (k = 1; k <= 7; k++) {
            take(B24, (char *)F + 0x14c + 3 * (k - 1), 3);
            bx = fe_strpos(pb, B24);
            if (!bx) continue;
            take(B50, pb, bx - 1);
            take(B24, (char *)F + 0x288 + 9 * (k - 1), 9);
            strcat(B50, trim(B24));
            strcat(B50, ",");
            q = pb + bx - 1;
            di = fe_strpos(q, " ");
            ax = fe_strpos(q, ",");
            cx = (short)(bx + 2);
            if ((di < ax || ax == 0) && di != 0) cx = (short)(bx + di - 2);
            if (di > ax && ax != 0) cx = (short)(bx + ax - 2);
            strcat(B50, pb + cx);
            strcpy(pb, trim(B50));
        }
        fe_str_replace_all(pb, ",,", ",", pb);

        /* month names */
        for (k = 1; k <= 12; k++) {
            take(B24, (char *)F + 0x164 + 3 * (k - 1), 3);
            ax = fe_strpos(pb, B24);
            if (!ax) continue;
            e = ax;
            if (strncmp(pb + e - 2, " ", 1)) continue;
            B18c[0] = 0;
            B24[0] = 0;
            if (!strncmp(pb + e - 4, " ", 1)) {                  /* " 5 MAY" */
                strncat(B18c, pb, (size_t)(e - 3));
                strncat(B24, pb + e - 3, 1);
            } else if (!strncmp(pb + e - 5, " ", 1)) {           /* " 27 MAY" */
                strncat(B18c, pb, (size_t)(e - 4));
                strncat(B24, pb + e - 4, 2);
            } else
                strncat(B18c, pb, (size_t)(e - 1));
            take(B50, (char *)F + 0x2c8 + 9 * (k - 1), 9);
            strcat(B18c, trim(B50));
            bx = fe_strpos(pb + e, " ");                         /* the end of the month word */
            if (fe_fixes && !B24[0]) bx = (short)(bx + day_after(pb + e + bx, B24));
            if (strcmp(B24, "") && tv_atoi(B24)) {
                fe_Email_OrdinalSuffix(t, B24);
                strcat(B18c, " ");
                strcat(B18c, B24);
            }
            strcat(B18c, ", ");
            strcpy(pb, trim(pb));
            if (!fe_fixes) bx = fe_strpos(pb + e, " ");
            q = pb + e + bx;                                     /* the year */
            dx = fe_strpos(q, " ");
            if (((dx == 0 && (short)strlen(pb) - e < bx + 4) || (dx >= 2 && dx <= 3)) && (!fe_fixes || digit(q[0])))
                strcat(B18c, century(q));
            strncat(B18c, q, (size_t)((int)strlen(pb) - bx - e));
            strcpy(pb, B18c);
        }
        fe_str_replace_all(pb, "  ", " ", pb);
    }

    /* the time */
    ax = !fe_fixes || strlen(pb) > 0x10 ? fe_strpos(pb + 0x10, ":") : 0;   /* the original looks past a short line */
    if (ax) {
        e = (short)(ax + 0x10);
        if (!strncmp(pb + e + 2, ":", 1)) {                      /* "hh:mm:ss" */
            take(B50, pb, e - 3);
            at(B50);
            strncat(B50, pb + e - 3, 5);
            strcat(B50, pb + e + 5);
            strcpy(pb, trim(B50));
        } else {
            take(B40, pb + e - 2, 1);
            take(B48, pb + e, 1);
            if (strcmp(B40, "0") >= 0 && strcmp(B40, "9") <= 0 && strcmp(B48, "0") >= 0 && strcmp(B48, "9") <= 0) {
                take(B50, pb, e - 3);
                at(B50);
                strcat(B50, pb + e - 3);
                strcpy(pb, trim(B50));
            }
        }
    }

    fe_str_replace_all(pb, "ATat", "at", pb);
    fe_str_replace_all(pb, "AT at", " at", pb);
    fe_str_replace_all(pb, "  ", " ", pb);
    fe_str_replace_all(pb, " 0", " #", pb);
    fe_str_replace_all(pb, "#", "", pb);
    fe_str_replace_all(pb, "DATE", "Date", pb);
    if (fe_fixes) fe_str_replace_all(pb, "  ", " ", pb);   /* a '#' removed next to a space */
}
