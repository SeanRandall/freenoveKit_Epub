/* The From / Author / To lines of a mail header: which part of the line is spoken as the sender's name
   (patent US 5,634,084 item 134; REFERENCE 8.4). Called by EmailHeaderBodyAnalyzer 1006fe00:
   - Email_FromLine for "From:" and ">From:" lines (the analyser passes the keyword it found);
   - Email_AddressList for "Author:", "TO:" and "To:" lines, for "CC:", "Cc:", "cc:", "BCC:" and "Priority:" lines from
     the second header of a mail on (a forwarded message), and for the continuation lines of all of these;
   - Email_UncapWords, from Email_AddressList: all-capital words get lower-case letters after the first. */
#include <string.h>
#include "frontend.h"

#define FBUF 4096          /* fe_fixes: size of each work buffer */
#define PMAX 499           /* fe_fixes: para_buf holds 500 bytes */

/* ---- MSVC string idioms ------------------------------------------------------------------------------------- */
/* tv_strncat 10078400: the count is unsigned, so a negative count copies the whole string; 0 leaves d alone */
static char *ncat(char *d, const char *s, int n)
{
    unsigned c = (unsigned)n;
    char *p = d;

    if (!c) return d;
    while (*p) p++;
    for (; c; c--)
        if (!(*p++ = *s++)) return d;
    *p = 0;
    return d;
}

/* The inlined strcpy/strcat: both lengths are taken first, then the bytes are copied forward (REP MOVSD, so a
   destination above an overlapping source reads what it has just written) */
static void icpy(char *d, const char *s)
{
    size_t n = strlen(s) + 1;
    for (size_t k = 0; k < n; k++) d[k] = s[k];
}

static void icat(char *d, const char *s)
{
    size_t n = strlen(s) + 1;
    d += strlen(d);
    for (size_t k = 0; k < n; k++) d[k] = s[k];
}

/* strcpy into para_buf; with fe_fixes cut to fit */
static void put(char *P, const char *s)
{
    if (fe_fixes) {
        size_t n = strlen(s);
        if (n > PMAX) n = PMAX;
        memmove(P, s, n);
        P[n] = 0;
    } else
        icpy(P, s);
}

/* str_replace_all 10044560 on para_buf in place; it may grow it to 1000 characters (fe_fixes: cut to fit; a
   result that fits is made in place as by the original, so that the bytes after the terminator stay the same) */
static void repl(char *P, const char *find, const char *rep)
{
    if (fe_fixes) {
        static char tmp[FBUF];
        fe_str_replace_all(P, find, rep, tmp);
        if (strlen(tmp) > PMAX) {
            put(P, tmp);
            return;
        }
    }
    fe_str_replace_all(P, find, rep, P);
}

/* fe_fixes: a para_buf without a terminator (never seen) is cut */
static void clamp(char *P)
{
    if (fe_fixes && !memchr(P, 0, PMAX + 1)) P[PMAX] = 0;
}

static short slen(const char *s) { return (short)strlen(s); }

/* ---- Email_UncapWords ----------------------------------------------------------------------------------------- */
/* Every word of para_buf that has no lower-case letter keeps its first character and gets the rest in lower case
   ("JOHN SMITH" -> "John Smith", "O'BRIEN" -> "O'brien"); other words are left alone. Words longer than 59
   characters are cut into pieces of 59, and the character after each piece is lost. para_buf is trimmed at both
   ends of the work, not at the front. @0x10072950 */
#define NC_FRAME (0x180 + 16)
static unsigned char nc_frame[NC_FRAME + 0x4000];
/* Harness only: Email_AddressList's frame at its entry, seen from the UncapWords it calls (0x194 bytes below it: the
   return address and the 0x190-byte frame). AddressList's other callees do not write where UncapWords reads stale bytes
   (their buffers are deeper down, and strpos's frame lies in UncapWords's result buffer), so it holds what UncapWords
   finds there when the harness has no frame of its own for it. */
static const unsigned char *nc_parent;

void fe_Email_UncapWords(TextProc *t)
{
    const unsigned char *fr = fe_frame_of(0x10072950);
    unsigned char *F = nc_frame;
    char *P = t->para_buf, *word, *up, *out;
    short pos, n;

    /* word [F+0x1c] (60), its upper-case copy [F+0x58] (60), the result [F+0x94] (252, then the return address) */
    if (!fr) fr = nc_parent;
    nc_parent = NULL;
    memset(F, 0, sizeof nc_frame);
    if (fr) memcpy(F, fr, NC_FRAME);
    if (fe_fixes) {
        word = (char *)F + NC_FRAME; up = word + FBUF; out = up + FBUF;
        _Static_assert(sizeof nc_frame >= NC_FRAME + 3 * FBUF, "UncapWords buffers");
    } else {
        word = (char *)F + 0x1c; up = (char *)F + 0x58; out = (char *)F + 0x94;
    }
    clamp(P);

    put(P, fe_str_trim_spaces(P));
    if (!fe_fixes || strlen(P) < PMAX) icat(P, " ");
    pos = 1;
    icpy(out, "");
    do {
        const char *s = P + pos - 1;
        n = fe_strpos(s, " ");
        if (n == 0) break;
        if (n > 60 && !fe_fixes) n = 60;       /* the word buffer holds 59 characters */
        icpy(word, "");
        ncat(word, s, n - 1);
        icpy(up, word);
        fe_strupr(up);
        /* An empty word (para_buf starting with a blank) counts as all capitals, and its "rest" is read from
           word + 1, past the terminator: what an earlier word or call left there is put in front of the name.
           fe_fixes: an empty word stays empty. */
        if (strcmp(word, up) == 0 && !(fe_fixes && !word[0])) {
            icpy(up, "");
            ncat(up, word, 1);
            icat(up, fe_strlwr(word + 1));
            icpy(word, up);
        }
        icat(out, word);
        icat(out, " ");
        pos += n;
    } while (slen(P) >= pos);
    put(P, fe_str_trim_spaces(out));
}

/* ---- Email_FromLine ------------------------------------------------------------------------------------------ */
/* One "From:" line: para_buf starts with "From: " (the analyser has made sure of the blank after the colon). kw is
   the keyword the analyser found, "From:" or ">From:" (a ">From:" line only comes here inside a header; a leading
   '>' quote mark is normally stripped by the analyser, so this is a line that started with ">>From:").
   1. ">From:": the '>' is dropped with strncpy(p, p + 1, strlen(p) - 1), which leaves the last character
      doubled ("Jane Doe" -> "Jane Doee").
   2. "addr (Name)": when the text between "From: " and the '(' is one word holding an '@' (or the '@' comes
      first) and a ')' follows, the line becomes "From: Name".
   3. "Name <addr>": everything from the '<' on is dropped, and so are the double quotes. Without a '<', the line
      is cut at the first '@', then at '!', '%' or "::", as long as one is left ("From: bob@x.com" -> "From: bob").
   4. It is cut at the first character after "From: " that is not a letter, digit, blank, quote, apostrophe,
      underscore or one of & ( ) * , - . ("From: J. Smith; Sales" -> "From: J. Smith").
   5. " (" with no ") " after it: cut there ("From: Bob (Sales)" -> "From: Bob").
   6. "(" -> "( " (through the placeholder "$#@! ", which the filter of step 4 cannot let through).
   7. A lower-case letter followed by a capital gets a blank in between ("JohnSmith" -> "John Smith"), except
      after "Mc" ("McDonald"; "MacDonald" -> "Mac Donald").
   8. Every word after "From: " gets a capital first character and the rest in lower case ("JOHN SMITH",
      "john smith" -> "John Smith", "McDonald" -> "Mcdonald", "O'Brien" -> "O'brien").
   9. The character after each '-' is made a capital ("Smith-jones" -> "Smith-Jones"). The same loop is run again
      from the first double quote, but it tests for '-' again, so a quoted name keeps its lower case
      ('"john smith"' -> '"john Smith"').
  10. " A " -> " Aye " (the initial "A" is read as the letter, not as the article).
   The words start at the 7th character, so the first 6 are always replaced by "From: ". @0x100712a0 */
static char fl_a[16384], fl_b[16384];   /* [F+0x2c], [F+0x128]: 252 bytes each in the original (see below) */

/* the loop of step 9 */
static void fl_caps_after(char *P, char *A, short dx, char mark)
{
    char ch[2];
    short i;

    if (dx == 0 || slen(P) < dx) return;
    i = dx;
    do {
        if (P[i - 1] == mark) {
            ch[0] = P[i];
            ch[1] = 0;
            A[0] = 0;
            ncat(A, P, i);
            fe_strupr(ch);
            icat(A, ch);
            /* a '-' at the end: the original appends what follows the terminator (fe_fixes: nothing) */
            icat(A, fe_fixes && !P[i] ? "" : P + i + 1);
            put(P, fe_str_trim_spaces(A));
        }
        i++;
    } while (slen(P) >= i);
}

void fe_Email_FromLine(TextProc *t, const char *kw)
{
    char *P = t->para_buf;
    char *A = fl_a, *B = fl_b;
    char ch[2];
    short si, ax, dx, i, n, cnt, pos;

    /* The original's two work buffers are 252 bytes each, the second one right under the return address. Lines
       reach it with at most about 210 characters, but step 7 can make them longer: "From: AbAbAb..." of 200
       characters crashes it. No local is read after an overflow, so the port keeps the buffers apart. fe_fixes:
       para_buf is cut to 499 characters at each step. */
    clamp(P);

    /* 1 */
    if (strcmp(kw, ">From:") == 0) {
        size_t l = strlen(P);
        if (fe_fixes)
            memmove(P, P + 1, l);
        else if (l > 0) {
            /* tv_strncpy 10078fd0 with count strlen - 1: the terminator is not copied */
            size_t c = l - 1;
            char *d = P;
            const char *s = P + 1;
            while (c && (*d++ = *s++)) c--;
            if (c) while (--c) *d++ = 0;
        }
    }

    /* 2 */
    si = fe_strpos(P, "(");
    ax = fe_strpos(P, "@");
    if (ax < si) {
        A[0] = 0;
        ncat(A, fe_fixes && strlen(P) < 6 ? "" : P + 6, si - 8);
        if (fe_strpos(A, " ") == 0 && fe_strpos(A, "@") != 0) {
            short rp = fe_strpos(P, ")");
            if (si + 1 < rp) {
                icpy(A, "From: ");
                ncat(A, P + si, rp - si - 1);
                put(P, fe_str_trim_spaces(A));
            }
        }
    }

    /* 3 */
    dx = fe_strpos(P, "<");
    if (dx != 0) {
        A[0] = 0;
        ncat(A, P, dx - 1);
        put(P, fe_str_trim_spaces(A));
        repl(P, "\"", "");
    } else {
        for (;;) {
            si = fe_strpos(P, "@");
            if (si == 0) {
                si = fe_strpos(P, "!");
                if (si == 0) {
                    si = fe_strpos(P, "%");
                    if (si == 0) si = fe_strpos(P, "::");
                }
            }
            if (si == 0) break;
            /* A blank as the second character loops for ever; para_buf starts with "From: " here, so it never
               happens (fe_fixes: the cut is made anyway). */
            if (strncmp(P + 1, " ", 1) == 0 && !fe_fixes) continue;
            A[0] = 0;
            ncat(A, P, si - 1);
            put(P, fe_str_trim_spaces(A));
        }
    }

    /* 4 */
    for (dx = 7; slen(P) >= dx; dx++) {
        signed char c = (signed char)P[dx - 1];
        if (c == ' ' || c == '"' || c == '\'') continue;
        if (c < '&' || c > 'z' || (c > '9' && c < 'A') || c == '/' || (c > 'Z' && c < 'a' && c != '_') ||
            c == '+') {
            A[0] = 0;
            ncat(A, P, dx - 1);
            put(P, fe_str_trim_spaces(A));
            break;
        }
    }

    /* 5 */
    dx = fe_strpos(P, " (");
    if (dx >= 9 && fe_strpos(P + dx, ") ") == 0) {
        B[0] = 0;
        ncat(B, P, dx - 1);
        icpy(P, B);
    }

    /* 6 */
    repl(P, "(", "$#@! ");
    repl(P, "$#@!", "(");

    /* 7 (cnt, the length of a run of capitals, does not change the result) */
    cnt = 0;
    for (i = 1; slen(P) - 1 >= i; i++) {
        char c1 = P[i - 1], l1 = c1 >= 'A' && c1 <= 'Z' ? c1 + 32 : c1;
        if ((signed char)l1 >= 'a' && (signed char)l1 <= 'z' && (l1 == c1 || cnt == 0)) {
            char c2 = P[i], l2 = c2 >= 'A' && c2 <= 'Z' ? c2 + 32 : c2;
            if (l1 == c1) cnt = 0; else cnt++;
            if (l2 != c2 && l1 == c1 && (i < 2 || strncmp(P + i - 2, "Mc", 2) != 0)) {
                if (fe_fixes && strlen(P) >= PMAX) continue;
                B[0] = 0;
                ncat(B, P, i);
                icat(B, " ");
                icat(B, P + i);
                icpy(P, B);
                i++;
            }
        } else if (l1 != c1 && (signed char)l1 >= 'a' && (signed char)l1 <= 'z')
            cnt++;
    }

    /* 8 */
    icpy(B, "From: ");
    put(P, fe_str_trim_spaces(P));
    if (!fe_fixes || strlen(P) < PMAX) icat(P, " ");
    pos = 7;
    do {
        const char *s = fe_fixes && strlen(P) < 6 ? "" : P + pos - 1;
        n = fe_strpos(s, " ");
        if (n == 0) break;
        A[0] = 0;
        ncat(A, s, n);
        ch[0] = A[0];
        ch[1] = 0;
        fe_strupr(ch);
        icat(B, ch);
        icat(B, fe_strlwr(A + 1));
        pos += n;
    } while (slen(P) > pos);
    put(P, fe_str_trim_spaces(B));

    /* 9 */
    fl_caps_after(P, A, fe_strpos(P, "-"), '-');
    /* The second loop starts at the first '"' but was copied with the '-' test (fe_fixes: it capitalises the
       character after each '"', as meant). */
    fl_caps_after(P, A, fe_strpos(P, "\""), fe_fixes ? '"' : '-');

    /* 10 */
    repl(P, " A ", " Aye ");
}

/* ---- Email_AddressList ------------------------------------------------------------------------------------------ */
/* An "Author:", "To:"/"TO:" or Cc/BCC/Priority line, or a continuation line of one of them (cc:Mail's
   "Author:  John Smith at CENTIGRAM", "To: Pat O'Brien; Al Maslin", Internet "To: bob@x.com (Bob Jones)").
   1. Double quotes are dropped and '-' becomes a blank.
   2. A line ending in ",\n" or ", \n" continues on the next line: hdr_cont_key = the keyword position (fe_hdr_key), so
      the analyser treats the next line as more of this header. Otherwise hdr_cont_key = 0.
   3. The keyword up to its colon, plus a blank, is the prefix ("To: "; " " when there is no colon). Each
      "addr (Name)" group, with the '@' before the '(', puts "Name, " after the prefix; if any did, that is the
      line ("To: a@b (Al), c@d (Bo)" -> "To: Al, Bo,").
   4. Addresses are cut out, item by item. The items end at " at " or ',' (the last one at the end of the line):
      in each item, the text from the first '<', then from the first '@', then from the first '.' up to the end of
      the item is dropped (not when that character starts the line). So "To: John Smith <js@x.com>, bob@y.com" ->
      "To: John Smith , bob", "Author: John.Smith at HQ" -> "Author: John at HQ", but also "J. R. Smith at HQ" ->
      "J at HQ".
   5. Double blanks go, then Email_UncapWords ("JOHN SMITH at HQ" -> "John Smith at Hq").
   6. The text between the prefix and the first " at " (or the end), if it is one word: a capital after its first
      character gets a blank before it ("jSmith" -> "j Smith"; "bjmaslin" stays as it is).
   7. "gateway" gets a blank before it. @0x10071dd0 */
#define FN_FRAME (0x1b8 + 16)
static unsigned char fn_frame[FN_FRAME + 0x4000];

/* Step 4 for one mark. bx: where the current item ends (1-based position of its " at " or ',', or past the end).
   The original's loop, with its three copies merged: the removal uses the item end found before, then the next
   item end is searched from the old one. */
static short fn_strip(char *P, const char *mark, short bx, char *out)
{
    short dx, si, l;

    for (;;) {
        dx = fe_strpos(P, mark);
        if (dx <= 1) return bx;
        if (dx < bx) {
            icpy(out, "");
            ncat(out, P, dx - 1);
            icat(out, P + bx - 1);
            put(P, fe_str_trim_spaces(out));
        }
        si = bx;
        l = slen(P);
        if (l <= bx) si = l - 1;
        if (fe_fixes && si < 0) si = 0;     /* an emptied line: the original reads the byte in front of it */
        bx = fe_strpos(P + si, " at ");
        if (bx == 0) bx = fe_strpos(P + si, ",");
        if (bx != 0) bx += si;
        else bx = slen(P) + 1;
        if (slen(P) + 1 < bx) bx = slen(P) + 1;
    }
}

void fe_Email_AddressList(TextProc *t)
{
    const unsigned char *fr = fe_frame_of(0x10071dd0);
    unsigned char *F = fn_frame;
    char *P = t->para_buf, *pre, *name, *out, *user;
    char c1[2], c2[2];
    short off, dp, rp, at, bx, dx;
    int c, d;
    size_t l;

    /* The prefix [F+0x20] holds 12 bytes, the name [F+0x2c] 80, the result [F+0x7c] 252 and the split name
       [F+0x178] 80, right under the return address. A prefix over 11 characters runs into the name (harmless); a
       colon after the 91st character (a continuation line) makes the prefix run into the result, which is then
       copied onto itself. A result over 252 characters or a one-word name of 80 or more would crash the original,
       but the lines are shorter than that and UncapWords has cut the words to 59 characters. The frame is modelled so
       that the runs match. */
    memset(F, 0, sizeof fn_frame);
    if (fr) memcpy(F, fr, FN_FRAME);
    if (fe_fixes) {
        pre = (char *)F + FN_FRAME; name = pre + FBUF; out = name + FBUF; user = out + FBUF;
        _Static_assert(sizeof fn_frame >= FN_FRAME + 4 * FBUF, "AddressList buffers");
    } else {
        pre = (char *)F + 0x20; name = (char *)F + 0x2c; out = (char *)F + 0x7c; user = (char *)F + 0x178;
    }
    clamp(P);

    /* 1 */
    repl(P, "\"", "");
    repl(P, "-", " ");

    /* 2: the last 2 (3) characters; the original reads in front of a shorter line (fe_tail) */
    l = strlen(P);
    if (strcmp(fe_fixes ? fe_tail(P, 2) : P + l - 2, ",\n") == 0 ||
        strcmp(fe_fixes ? fe_tail(P, 3) : P + l - 3, ", \n") == 0)
        t->hdr_cont_key = fe_hdr_key;
    else
        t->hdr_cont_key = 0;

    /* 3 */
    c = fe_strpos(P, ":");
    pre[0] = 0;
    ncat(pre, P, c);
    icat(pre, " ");
    off = 0;
    out[0] = 0;
    icat(out, pre);
    for (;;) {
        const char *s = P + off;
        dp = fe_strpos(s, "(");
        rp = fe_strpos(s, ")");
        at = fe_strpos(s, "@");
        if (dp <= 0 || rp <= dp || at <= 0 || dp <= at) break;
        dp += off;
        rp += off;
        ncat(out, P + dp, rp - dp - 1);
        icat(out, ", ");
        off = rp;
    }
    if (slen(out) > slen(pre)) put(P, out);

    /* 4 */
    bx = fe_strpos(P, " at ");
    if (bx == 0) {
        bx = fe_strpos(P, ",");
        if (bx == 0) bx = slen(P);
    }
    bx = fn_strip(P, "<", bx, out);
    bx = fn_strip(P, "@", bx, out);
    fn_strip(P, ".", bx, out);

    /* 5 */
    repl(P, "  ", " ");
    nc_parent = fr ? fr - 0x194 : NULL;
    fe_Email_UncapWords(t);

    /* 6: the name runs from 2 characters after the colon to the " at ". With nothing after the colon ("To:") the
       count is negative and the name is read from past the terminator, whatever an earlier line left there; with
       the " at " in front of the colon the count is negative too (fe_fixes: no name in either case). */
    dx = fe_strpos(P, " at ");
    if (dx == 0) dx = slen(P) + 1;
    d = dx;
    icpy(name, "");
    if (fe_fixes && ((size_t)c + 1 > strlen(P) || d - c - 2 < 0)) goto gateway;
    ncat(name, P + c + 1, d - c - 2);
    if (fe_strpos(name, " ") == 0) {
        icpy(user, name);
        bx = 2;
        if (slen(name) >= bx) {
            do {
                icpy(c1, "");
                ncat(c1, name + bx - 1, 1);
                icpy(c2, c1);
                fe_strupr(c2);
                if (strcmp(c1, c2) == 0) break;     /* not a lower-case letter */
                bx++;
            } while (slen(name) >= bx);
        }
        if (slen(name) > bx) {
            icpy(user, "");
            ncat(user, name, bx - 1);
            icat(user, " ");
            icat(user, name + bx - 1);
        }
        icpy(out, "");
        ncat(out, P, c + 1);
        icat(out, user);
        icat(out, P + d - 1);
        put(P, fe_str_trim_spaces(out));
    }

gateway:
    /* 7 */
    dx = fe_strpos(P, "gateway");
    if (dx != 0) {
        icpy(out, "");
        ncat(out, P, dx - 1);
        icat(out, " ");
        icat(out, P + dx - 1);
        put(P, fe_str_trim_spaces(out));
    }
}
