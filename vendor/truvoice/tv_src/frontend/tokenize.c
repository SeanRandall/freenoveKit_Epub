/* Tokenize_FirstPass (1002e800) and Tokenize_Dots (1003fe90): the per-word pass of TextExpander_Paragraph that
   runs before ClassifyWord. The driver has copied the next token of the paragraph text into word[cur_word] (at most
   30 characters, up to the end found by Tokenize_SplitSeparators). This pass decides whether the token is really
   several words, rewrites the paragraph text (this+0) and shortens *end accordingly, and rewrites the word:
   "US$5" -> "$US5", "U.S.A." / "e.mail" / "user@host.com" (" dot "), "*.txt" ("star dot TXT"), "a,b", camelCase,
   "?!" after a word, "(s)", *emphasis* and _emphasis_ ("~" marks), "$5K", "#" (pound/number), "3x5" (by),
   "5kg" -> "5 kg", "ext12" -> "ext 12". Written from the listing (REFERENCE 3.2b).

   The locals live in a byte array laid out like the original stack frame (fr + 0x2c is the word, and so on), so a
   read before the start of a string or past its terminator sees the same bytes as the original does. */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

#pragma GCC diagnostic ignored "-Wrestrict"   /* the locals share one frame array on purpose */

short fe_emph_state;   /* the DLL's global at 0x10137074: inside *...* / _..._ emphasis; the driver clears it */
const unsigned char *fe_tok_stack;   /* test harness only: the 0x260 bytes of the original's frame at entry */

/* ---- Tokenize_Dots ------------------------------------------------------------------------------------------------ */
/* word_lc: drop a final quote ('"..' -> '..'), a final '.' (lc_period) and further closing punctuation, an opening
   bracket; keep the result as word_as_typed; lc_initcap if only the first letter is a capital; lower-case it.
   @0x1003fe90 */
void fe_Tokenize_Dots(TextProc *t)
{
    char fr[0x100];
    char *base = (char *)t, *wl = t->word_lc;
    char *prev = fr + 0x10, *ch = fr + 0x12, *tmp = fr + 0x14;
    const unsigned char *st = fe_frame_of(0x1003fe90);   /* [F+x] = fr[x]; the harness passes the real bytes */

    memset(fr, 0, sizeof fr);
    if (st) memcpy(fr, st, 0x50);
    ch[0] = base[0x2a17 + strlen(wl)];
    ch[1] = 0;
    if (ch[0] == '"') {
        tmp[0] = 0;
        strncat(tmp, wl, strlen(wl) - 1);
        strcpy(wl, tmp);
    } else if (!strcmp(base + 0x2a15 + strlen(wl), "\"..")) {
        tmp[0] = 0;
        strncat(tmp, wl, strlen(wl) - 3);
        strcat(tmp, "..");
        strcpy(wl, tmp);
    }
    t->lc_period = 0;
    if (!strcmp(base + 0x2a17 + strlen(wl), ".")) {
        tmp[0] = 0;
        t->lc_period = 1;
        strncat(tmp, wl, strlen(wl) - 1);
        strcpy(wl, tmp);
    }
    for (;;) {
        ch[1] = 0;
        ch[0] = base[0x2a17 + strlen(wl)];
        if (!(ch[0] == '.' || ch[0] == ',' || ch[0] == ';' || ch[0] == ':' || ch[0] == '?' || ch[0] == '!' ||
              ch[0] == ')' || ch[0] == ']'))
            break;
        tmp[0] = 0;
        strncat(tmp, wl, strlen(wl) - 1);
        strcpy(wl, tmp);
        prev[0] = 0;
        strcat(prev, ch);
    }
    if (prev[0] == '.') t->lc_period = 1;   /* prev is uninitialised stack in the original when nothing was stripped */
    ch[1] = 0;
    ch[0] = wl[0];
    if (ch[0] == '(' || ch[0] == '[') {
        strcpy(tmp, wl + 1);
        strcpy(wl, tmp);
    }
    strcpy(t->word_as_typed, wl);
    t->lc_initcap = 0;
    strcpy(tmp, wl);
    fe_strlwr(tmp);
    if (wl[0] != tmp[0] && !strcmp(tmp + 1, wl + 1)) t->lc_initcap = 1;
    strcpy(wl, tmp);
}

/* ---- Tokenize_FirstPass ------------------------------------------------------------------------------------------- */
#define W     (fr + 0x2c)    /* the word being rewritten */
#define T68   (fr + 0x68)
#define TA4   (fr + 0xa4)
#define WL    (fr + 0xe0)    /* the word in lower case */
#define T11C  (fr + 0x11c)
#define PUNCT (fr + 0x158)   /* ".,:;!?" */
#define MC    (fr + 0x160)   /* "#Mc#" */
#define AMPM  (fr + 0x168)   /* "#a.m#p.m#a.d#b.c#" */
#define CONS  (fr + 0x17c)   /* "#b#c#d#...#z#" */
#define T1A8  (fr + 0x1a8)
#define EXT   (fr + 0x1e4)   /* "ext hwy rte rt  rm  apt ste jan ... dec ", 4 characters each */
#define T234  (fr + 0x234)   /* trailing punctuation taken off the word, last character first */
#define KIND  (fr + 0x14)    /* "N" (digits before the first dot), "L" (letters) */
#define LAST  (fr + 0x16)    /* the last character of the word */
#define C1    (fr + 0x1e)    /* one character */

static int upper(char c) { return c >= 'A' && c <= 'Z'; }
static int lower(char c) { return c >= 'a' && c <= 'z'; }
static int digit(char c) { return c >= '0' && c <= '9'; }

/* one character of s (1-based k) into C1 */
static void char_at(char *fr, const char *s, int k)
{
    C1[0] = 0;
    strncat(C1, s + k - 1, 1);
}

/* the paragraph text becomes text[0 .. keep) + ins + text from `from` (0-based) */
static void splice(TextProc *t, char *buf, int keep, const char *ins, const char *from)
{
    buf[0] = 0;
    strncat(buf, t->text, keep);
    strcat(buf, ins);
    strcat(buf, from);
    strcpy(t->text, buf);
}

/* @0x1002e800. pos is the 1-based position of the word in the text; *end its last position. Returns 0, or -1 when
   the work buffer cannot be allocated. */
short fe_Tokenize_FirstPass(TextProc *t, short pos, short *end)
{
    char fr[0x280];
    char *buf;
    int c = t->cur_word;
    short fA = 0, fB = 0, s18 = 0, s1a = 0, s1c = 0, s28 = 0;
    short bp, si, bx, k;

    /* Some paths read locals the original never wrote in this call ("word_lc = T68 + 2" for a one-letter word), i.e.
       what earlier calls left on the stack. The port starts from zeros; the test harness passes the real bytes. */
    const unsigned char *st = fe_tok_stack ? fe_tok_stack : fe_frame_of(0x1002e800);   /* nested: the harness's replay */
    if (st && !fe_fixes) memcpy(fr, st, 0x260);
    else memset(fr, 0, sizeof fr);
    strcpy(CONS, "#b#c#d#f#g#h#j#k#l#m#n#p#q#r#s#t#v#w#x#z#");
    strcpy(EXT, "ext hwy rte rt  rm  apt ste jan feb mar apr may jun jul aug sep oct nov dec ");
    strcpy(AMPM, "#a.m#p.m#a.d#b.c#");
    strcpy(PUNCT, ".,:;!?");
    strcpy(MC, "#Mc#");
    buf = fe_malloc(0x7d1);
    if (!buf) return -1;
    if ((short)strlen(t->text) > 0x794) {
        /* the original returns at once: the buffer is not freed and PREPUNC not set (ClassifyWord then reads a
           stale pointer). fe_fixes leaves the word as it is but does both. */
        if (fe_fixes) {
            fe_free(buf);
            t->prepunc[t->cur_word] = fe_strdup("    ");
        }
        return 0;
    }
#define AFTER_WORD() (t->text + pos - 1 + strlen(t->word[c]))

    strcpy(W, t->word[c]);
    strcpy(WL, t->word[c]);
    fe_strlwr(WL);
    LAST[0] = 0;
    strcat(LAST, fe_tail(W, 1));
    if (!strncmp(W, "US$", 3)) {
        strcpy(T68, "$US");
        if ((short)strlen(W) > 3) strcat(T68, W + 3);
        strcpy(W, T68);
        strcpy(t->word[c], T68);
        fe_strlwr(WL);
    }
    if (!strncmp(W, "$", 1)) goto dollar;

    /* ---- words with dots: numbers ("1.2.3"), initials ("U.S.A."), "e.mail", host names ---------------------- */
    strcpy(T68, W);
    T234[0] = 0;
    for (bp = (short)strlen(T68); bp >= 1; bp--) {
        char_at(fr, T68, bp);
        if (!fe_strpos(PUNCT, C1)) break;
        T11C[0] = 0;
        strncat(T11C, T68, bp - 1);
        strcpy(T68, T11C);
        strcat(T234, C1);
    }
    if (!fe_strpos(T68, ".") || (short)strlen(T68) <= 3) goto rest;
    strcpy(WL, T68);
    fe_strlwr(WL);
    strcat(T68, ".");
    KIND[0] = 0;
    s1a = 1;
    for (;;) {
        s1c = fe_strpos(T68 + s1a - 1, ".");
        if (s1c == 0) goto seg_done;
        T11C[0] = 0;
        strncat(T11C, T68 + s1a - 1, s1c - 1);
        if (atoi(T11C) != 0 || !strcmp(T11C, "0")) {
            strcat(KIND, "N");
            goto seg_done;
        }
        if (s1c - 1 >= 2) {
            s1a += s1c;
            if ((short)strlen(T68) >= s1a) continue;
            goto seg_done;
        }
        if (s1c == 2 && s1a == 1) {
            /* one letter and a dot first ("e.mail"): "e. mail" */
            W[0] = 0;
            strcat(T11C, ". ");
            strncat(T11C, T68 + 2, strlen(T68) - 3);
            strncat(W, T11C, 2);
            strcpy(WL, W);
            fe_strlwr(WL);
            buf[0] = 0;
            strncat(buf, t->text, pos - 1);
            strcat(buf, T11C);
            strcat(buf, t->text + pos - 1 + strlen(t->word[c]) - strlen(T234));
            strcpy(t->text, buf);
            *end = pos + 2;
            goto rest;
        }
        s1a += s1c;
        if (s1c == 1) goto rest;
        if ((short)strlen(T68) >= s1a) continue;
        goto rest;
    }
seg_done:
    T68[0] = 0;
    strcat(T68, "#");
    strcat(T68, WL + strlen(WL) - 3);
    strcat(T68, "#");
    if (fe_strpos(AMPM, T68)) goto rest;
    s18 = fe_strpos(WL, ".");
    if (!strncmp(WL + s18, "^[", 2) || !strncmp(WL + s18, "\x1b[", 2)) goto rest;
    if (s18 <= 1 || (short)strlen(WL) <= s18) goto rest;
    s28 = 0;
    strcpy(T68, W);
    fe_strupr(T68);
    KIND[0] = 0;
    for (bp = 1; bp <= s18 - 1; bp++) {
        char_at(fr, T68, bp);
        if (upper(C1[0])) {
            KIND[0] = 0;
            strcat(KIND, "L");
            s28 = 1;
            break;
        }
        if (digit(C1[0])) {
            KIND[0] = 0;
            strcat(KIND, "N");
        }
    }
    if (s28 == 0 && KIND[0] != 'N') goto rest;

    if (fe_strpos(W, "@")) {
        /* e-mail address: "USER @ HOST dot COM" */
        if (fe_strpos(W, "<") && fe_strpos(W, ">")) {
            fe_str_replace_all(W, "<", "", W);
            fe_str_replace_all(W, ">", "", W);
        }
        strcpy(T68, W);
        fe_strupr(T68);
        fe_str_replace_all(T68, "@", " %$# ", T68);
        fe_str_replace_all(T68, "%$#", "@", T68);
    } else {
        /* a known abbreviation with dots stays one word; "abbr.rest": split after the abbreviation */
        strcpy(t->word_lc, WL);
        fe_Tokenize_Dots(t);
        fe_Abbrev_LetterRange(t, &s1a, &s1c);
        fe_str_replace_all(t->word_lc, ".", " ", t->word_lc);
        s18 = fe_strpos(t->word_lc, " ");
        for (bp = s1a; bp <= s1c; bp++) {
            strcpy(T11C, fe_abbrev[bp].key);
            fe_strlwr(T11C);
            if (strlen(t->word_lc) == strlen(T11C) && !strcmp(T11C, t->word_lc)) goto rest;
            if ((short)strlen(T11C) - s18 == -1 && !strncmp(T11C, t->word_lc, s18 - 1)) {
                strcpy(T68, W);
                W[0] = 0;
                strncat(W, T68, s18);
                strcpy(WL, W);
                fe_strlwr(WL);
                splice(t, buf, pos - 1, T68, AFTER_WORD());
                *end = pos + s18 - 1;
                goto rest;
            }
        }
    }
    /* trailing non-alphanumerics go to TA4 */
    TA4[0] = 0;
    for (bp = (short)strlen(T68); s18 < bp; bp--) {
        char_at(fr, T68, bp);
        if (upper(C1[0]) || digit(C1[0])) break;
        strcat(TA4, C1);
    }
    T11C[0] = 0;
    strncat(T11C, T68, strlen(T68) - strlen(TA4));
    strcpy(T68, T11C);
    T11C[0] = 0;
    for (si = (short)strlen(TA4); si >= 1; si--) strncat(T11C, TA4 + si - 1, 1);
    strcpy(TA4, T11C);
    /* s18 -> the last dot that is not the last character */
    k = 0;
    for (;;) {
        short p;
        s18 += k;
        p = fe_strpos(T68 + s18, ".");
        if (p == 0) break;
        if ((short)strlen(T68) - p == s18) break;
        k = p;
    }
    if (KIND[0] == 'N' && (atoi(T68 + s18) > 0 || T68[s18] == '0')) goto rest;
    si = 0;
    for (bp = s18 + 1; (short)strlen(T68) >= bp; bp++) {
        char_at(fr, T68, bp);
        if (upper(C1[0]) || (KIND[0] == 'L' && digit(C1[0]))) {
            si = 1;
            break;
        }
    }
    if (si == 0) goto rest;
    fe_str_replace_all(T68, ".", " dot ", T68);
    strcat(T68, TA4);
    W[0] = 0;
    bp = fe_strpos(T68, " ");
    strncat(W, T68, bp - 1);
    splice(t, buf, pos - 1, T68, AFTER_WORD());
    *end = pos + bp - 1;
    goto done;

rest:
    /* the extension of "*.ext" (see the star rule below): two bytes into the work buffer, which is past its end for
       a one-letter word (the original then reads what earlier calls left there; fe_fixes takes "") */
    if (fe_fixes && strlen(T68) < 2) t->word_lc[0] = 0;
    else strcpy(t->word_lc, T68 + 2);
    /* "*.ext" */
    if (!strcmp(W, "*.") && (short)strlen(t->word_lc) >= 3) {
        for (bx = 1; (short)strlen(t->word_lc) >= bx; bx++) {
            C1[0] = 0;
            strncat(C1, t->word_lc + bx - 1, 1);
            fe_strlwr(C1);
            if (lower(C1[0])) {
                if (bx > 3) goto comma;
            } else {
                if (bx <= 3) goto comma;
                break;
            }
        }
        buf[0] = 0;
        strncat(buf, t->text, pos - 1);
        strcat(buf, "star dot ");
        strcpy(W, "star dot");
        T68[0] = 0;
        strncat(T68, t->word_lc, 3);
        fe_strupr(T68);
        strcat(buf, T68);
        strcat(buf, t->text + pos + 5);
        strcpy(t->text, buf);
        *end = pos + 8;
        goto done;
    }
comma:
    /* "abc,def": the word ends at the comma (numbers such as "1,000" stay) */
    strcpy(T68, W);
    bp = fe_strpos(T68, ",");
    if (bp >= 2) {
        short allnum = 0;
        if (bp <= 4) {
            allnum = 1;
            for (si = 1; (short)strlen(T68) >= si; si++) {
                char_at(fr, T68, si);
                if (!(digit(C1[0]) || C1[0] == '.' || C1[0] == ',')) {
                    allnum = 0;
                    break;
                }
            }
        }
        if (!allnum) {
            W[0] = 0;
            strncat(W, T68, bp);
            strcpy(WL, W);
            fe_strlwr(WL);
            splice(t, buf, pos - 1, T68, AFTER_WORD());
            *end = pos + bp - 1;
        }
    }
    /* "5Hz", "pF": no camelCase split */
    if (strlen(W) >= 3 && !strncmp(W + 1, "Hz", 2)) goto qmark;
    if (strlen(W) <= 3 && !strncmp(W, "pF", 2)) goto qmark;
    /* camelCase: "someThing" -> "some" + "Thing" (not "McDonald", not before a second capital) */
    TA4[0] = 0;
    strncat(TA4, W, 1);
    if (atoi(TA4) > 0) goto qmark;
    strcpy(T68, TA4);
    fe_strlwr(T68);
    for (bp = 2; (short)strlen(W) >= bp; bp++) {
        C1[0] = 0;
        strncat(C1, W + bp - 1, 1);
        strcpy(T11C, C1);
        fe_strlwr(T11C);
        if (TA4[0] == T68[0] && lower(T68[0]) && C1[0] != T11C[0]) {
            TA4[0] = 0;
            strcat(TA4, "#");
            strncat(TA4, W, bp - 1);
            strcat(TA4, "#");
            if (fe_strpos(MC, TA4)) goto qmark;
            if ((short)strlen(W) != bp) {
                TA4[0] = 0;
                strncat(TA4, W + bp, 1);
                strcpy(T68, TA4);
                fe_strlwr(T68);
                if (TA4[0] != T68[0]) goto qmark;
            }
            strcpy(T68, W);
            W[0] = 0;
            strncat(W, T68, bp - 1);
            strcpy(WL, W);
            fe_strlwr(WL);
            strcpy(t->sepr[c], "");
            buf[0] = 0;
            strncat(buf, t->text, pos - 1);
            strcat(buf, T68);
            strcat(buf, t->text + pos - 1 + strlen(t->word[c]));
            strcpy(t->text, buf);
            *end = pos + bp - 2;
            goto qmark;
        }
        strcpy(TA4, C1);
        strcpy(T68, T11C);
    }
qmark:
    /* a word of only "?" and "!" is appended to the previous word and its PUNC */
    strcpy(T68, W);
    if (c > 1 && (fe_strpos(T68, "?") || fe_strpos(T68, "!"))) {
        for (si = 1; (short)strlen(T68) >= si; si++) {
            char_at(fr, T68, si);
            if (C1[0] != '?' && C1[0] != '!') goto parens;
        }
        if ((short)strlen(T68) > 4) {
            T11C[0] = 0;
            strncat(T11C, T68, 4);
            strcpy(T68, T11C);
        }
        strcat(t->word[c - 1], T68);
        if (strlen(T68) + strlen(t->punc[c - 1]) <= strlen("    ")) {
            strcat(t->punc[c - 1], T68);
        } else {
            strcpy(T11C, t->punc[c - 1]);
            strcat(T11C, T68);
            fe_free(t->punc[c - 1]);
            t->punc[c - 1] = fe_strdup(T11C);
        }
        W[0] = 0;
        WL[0] = 0;
        buf[0] = 0;
        strncat(buf, t->text, pos - 2);
        strcat(buf, T68);
        strcat(buf, t->text + pos - 1 + strlen(t->word[c]));
        strcpy(t->text, buf);
        *end = pos + (short)strlen(T68) - 1;
    }
parens: {
        /* "book(s)" -> "books" */
        short p = fe_strpos(W, "(s)");
        if (p && ((short)strlen(W) - p == 2 ||
                  ((short)strlen(W) - p == 3 && (!strcmp(LAST, ".") || !strcmp(LAST, ",")))))
            fe_str_replace_all(W, "(s)", "s", W);
    }
    /* *emphasis*: "~" before the first word, emph_state until the closing star */
    TA4[0] = 0;
    if (t->cur_word != 1) strcat(TA4, fe_tail(t->word[c - 1], 1));
    k = fe_strpos(W, "*");
    if (k == 0) goto underscore;
    if (k <= 1) {
        if (digit(TA4[0])) {
            C1[0] = 0;
            if (strlen(W) == 1) {
                strncat(C1, t->text + pos + 1, 1);
                if (C1[0] >= '1' && C1[0] <= '9') goto underscore;    /* "3 * 4" */
            }
            strncat(C1, W + 1, 1);
            if (C1[0] >= '1' && C1[0] <= '9') goto underscore;
        }
        for (bx = 1; (short)strlen(W) - 1 >= bx; bx++) {
            C1[0] = 0;
            strncat(C1, W + bx, 1);
            fe_strlwr(C1);
            if (lower(C1[0])) goto star_word;
            if (bx == 1 && C1[0] == '.') goto underscore;
        }
        goto underscore;
    star_word:
        bx = 1;
        for (si = 2; (short)strlen(W) >= si; si++) {
            if (W[si - 1] != '*') {
                bx = 0;
                break;
            }
        }
        if (bx) {
            if (fe_emph_state) {
                strcpy(W, "");
                fe_emph_state = 0;
            } else {
                strcpy(W, "~~");
                fe_emph_state = 1;
            }
            goto done;
        }
        for (bx = 1; (short)strlen(W) >= bx && W[0] == '*'; bx++) {
            T68[0] = 0;
            strcat(T68, W + 1);
            strcpy(W, T68);
        }
        if (!strcmp(W, ".") || !strcmp(W, "..")) {
            fe_emph_state = 0;
        } else {
            strcpy(T68, "~");
            strcat(T68, W);
            strcpy(W, T68);
            fe_emph_state = 1;
        }
    }
    if (fe_emph_state == 0) goto underscore;
    bx = fe_strpos(W, "*");
    if ((short)strlen(W) == bx) {
        T68[0] = 0;
        strncat(T68, W, strlen(W) - 1);
        if (atoi(T68) != 0) {
            if (fe_fixes) C1[0] = 0;                               /* (the original does not clear it) */
            strncat(C1, t->text + pos + strlen(W), 1);
            if (C1[0] >= '1' && C1[0] <= '9') goto underscore;      /* "3* 4" */
        }
    }
    if (bx <= 0) goto underscore;
    bp = 1;
    for (bx = bx + 1; (short)strlen(W) >= bx; bx++) {
        C1[0] = 0;
        strncat(C1, W + bx - 1, 1);
        fe_strlwr(C1);
        if (lower(C1[0]) || digit(C1[0])) {
            bp = 0;
            break;
        }
    }
    if (bp == 0) goto underscore;
    for (bx = (short)strlen(W); bx >= 1; bx--) {
        if (W[bx - 1] == '*') {
            T68[0] = 0;
            strncat(T68, W, bx - 1);
            if ((short)strlen(W) > bx) strcat(T68, W + bx);
            strcpy(W, T68);
        } else {
            C1[0] = 0;
            strncat(C1, W + bx - 1, 1);
            fe_strlwr(C1);
            if (lower(C1[0]) || digit(C1[0])) break;
        }
    }
    fe_emph_state = 0;
underscore:
    /* _emphasis_ */
    if (fe_strpos(W, "_") == 1 &&
        (!strcmp(fe_tail(W, 1), "_") ||
         (!strncmp(fe_tail(W, 2), "_", 1) && (!strcmp(LAST, ".") || !strcmp(LAST, ","))))) {
        fe_str_replace_all(W, "_", "", W);
        strcpy(TA4, "~");
        strcat(TA4, W);
        strcpy(W, TA4);
        fe_strlwr(W);
    }

dollar:
    /* "$5", "$ 5" (joined), "$5K" -> "$5,000", "$s" -> "dollars" */
    if (fe_strpos(W, "$") == 1) {
        for (si = 2; (short)strlen(W) >= si; si++) {
            T68[0] = 0;
            strncat(T68, W + si - 1, 1);
            if ((si == 2 && T68[0] == 's') || digit(T68[0])) goto dollar_k;
        }
        T68[0] = 0;
        strncat(T68, t->text + pos + strlen(W), 1);
        if (!digit(T68[0])) goto pound;
        k = fe_strpos(t->text + pos + strlen(W), " ");
        T68[0] = 0;
        strncat(T68, t->text + pos + strlen(W), k - 1);
        strcat(W, T68);
        strcpy(WL, W);
        fe_strlwr(WL);
        *end = pos + (short)strlen(W);
    dollar_k:
        bx = fe_strpos(W, "K");
        if (bx) {
            int ok = (short)strlen(W) == bx;
            if (!ok && (short)strlen(W) - 2 <= bx) {
                if (strcmp(LAST, "!") >= 0 && strcmp(LAST, "/") <= 0) ok = 1;
                else if (strcmp(LAST, ":") >= 0 && strcmp(LAST, "@") <= 0) ok = 1;
            }
            if (ok) {
                T68[0] = 0;
                strncat(T68, W, bx - 1);
                strcat(T68, ",000");
                if ((short)strlen(W) > bx) strcat(T68, W + bx);
                strcpy(W, T68);
                goto pound;
            }
        }
        if (strlen(W) == 2 && !strcmp(WL + 1, "s")) strcpy(W, "dollars");
    }
pound:
    /* "#" after a number: pound(s); "5#" -> "5 pounds"; "#s" -> "numbers" */
    k = fe_strpos(WL, "#");
    if (k) {
        if (t->cur_word > 1 && t->num[c - 1]) {
            if (!strcmp(WL, "#s") || !strcmp(WL, "#'s")) {
                strcpy(W, "pounds");
                goto done;
            }
            if (!strcmp(WL, "#")) {
                strcpy(W, "pound");
                if (atoi(t->word[c - 1]) > 1) strcat(W, "s");
                goto done;
            }
        } else {
            T68[0] = 0;
            strncat(T68, WL, k - 1);
            if (atoi(T68) != 0) {
                strcpy(W, T68);
                strcat(W, " pound");
                if (atoi(T68) > 1) strcat(W, "s");
                goto done;
            }
        }
    }
    s28 = fe_strpos(WL, "#");
    if (s28) {
        strcpy(T68, WL);
        TA4[0] = 0;
        si = fe_strpos(WL, "s");
        if (si) {
            T11C[0] = 0;
            if (s28 > 1) strncat(T11C, WL, s28 - 1);
            T68[0] = 0;
            strncat(T68, WL + s28 - 1, si - s28 + 1);
            if ((short)strlen(WL) > si) strcat(TA4, WL + si);
        }
        if (!strcmp(T68, "#s") || !strcmp(T68, "#'s")) {
            strcpy(W, "numbers");
            strcat(W, TA4);
            goto x_by;
        }
        if (!strcmp(WL, "#")) goto x_by;
        s18 = 3;
        if (strlen(W) < 5) s18 = (short)strlen(W) - 2;
        if (s18 < 1 || s28 <= 1) goto x_by;
        si = 1;
        for (k = 1; k <= s18; k++) {
            char *p = WL + s28 - k - 1;
            if (strncmp(p, "a", 1) < 0 || strncmp(p, "z", 1) > 0) {
                si = 0;
                break;
            }
        }
        if (si == 1) {
            /* "abc#5": a space before the "#" */
            W[0] = 0;
            strncat(W, t->word[c], s28 - 1);
            buf[0] = 0;
            strncat(buf, t->text, pos + s28 - 2);
            strcat(buf, " ");
            strcat(buf, t->text + pos + s28 - 2);
            strcpy(t->text, buf);
            *end = pos + s28 - 1;
        }
    }
x_by:
    /* "xb..." -> "x b..."; "3x5" -> "3 by 5" (not 80x86) */
    bp = fe_strpos(WL, "x");
    if (bp && strcmp(WL, "xmas")) {
        if (bp == 1) {
            strcpy(TA4, "#");
            strncat(TA4, WL + 1, 1);
            strcat(TA4, "#");
            if (fe_strpos(CONS, TA4)) {
                strcpy(TA4, "rom");
                for (si = 2; (short)strlen(WL) >= si; si++) {
                    char ch = WL[si - 1];
                    C1[0] = ch;
                    C1[1] = 0;
                    if (ch == 'i' || ch == 'v' || ch == 'x' || ch == 'l' || ch == 'c' || ch == 'd' || ch == 'm' ||
                        (ch >= '!' && ch <= '.') || (ch >= ':' && ch <= '?'))
                        continue;
                    strcpy(TA4, "###");
                    break;
                }
                if (strcmp(TA4, "rom")) {
                    strcpy(W, "x ");
                    strcat(W, t->word[c] + 1);
                }
            }
        } else {
            s28 = strncmp(WL + bp - 1, W + bp - 1, 1) != 0;
            T68[0] = 0;
            strncat(T68, W, bp - 1);
            TA4[0] = 0;
            if (T68[0] == '(' || T68[0] == '[') {
                strncat(TA4, T68, 1);
                T68[0] = 0;
                strncat(T68, W + 1, bp - 2);
            }
            T11C[0] = 0;
            strcat(T11C, W + bp);
            if (atoi(T68) && atoi(T11C) && s28 == 0 && !(atoi(T68) == 80 && atoi(T11C) == 86)) {
                strcpy(W, TA4);
                strcat(W, T68);
                buf[0] = 0;
                strncat(buf, t->text, pos + bp - 2);
                strcat(buf, " by ");
                strcat(buf, t->text + pos + bp - 1);
                strcpy(t->text, buf);
                strcpy(t->text, buf);
                *end = pos + bp - 1;
            }
        }
    }
    /* "q" + consonant, "z" + consonant other than "w": "q" and the rest */
    if (fe_strpos(WL, "q") == 1 || fe_strpos(WL, "z") == 1) {
        strcpy(TA4, "#");
        strncat(TA4, WL + 1, 1);
        strcat(TA4, "#");
        if (fe_strpos(CONS, TA4) && (fe_strpos(WL, "q") == 1 || strcmp(TA4, "#w#"))) {
            W[0] = 0;
            strncat(W, WL, 1);
            strcat(W, " ");
            strcat(W, t->word[c] + 1);
        }
    }
    /* ">" quoting mark before a word (not before a number) */
    if (fe_strpos(W, ">") == 1) {
        TA4[0] = 0;
        strncat(TA4, strlen(W) > 1 ? W + 1 : t->text + pos + strlen(W), 1);
        if (atoi(TA4) == 0) {
            T68[0] = 0;
            strcat(T68, W + 1);
            strcpy(W, T68);
        }
    }
    /* a number with a letter unit: "5kg" -> "5 kg" when the unit is an abbreviation (not "5s", "1st", "4th") */
    fe_str_replace_all(W, "(", "", W);
    fA = WL[0] == '(';
    fe_str_replace_all(WL, "(", "", WL);
    fe_str_replace_all(WL, ")", "", WL);
    fB = 0;
    TA4[0] = 0;
    strncat(TA4, W, 1);
    if (strcmp(TA4, "0") >= 0 && strcmp(TA4, "9") <= 0) {
        for (bp = 2; (short)strlen(W) >= bp; bp++) {
            TA4[0] = 0;
            strncat(TA4, WL + bp - 1, 1);
            if (strcmp(TA4, "a") >= 0 && strcmp(TA4, "z") <= 0) {
                fB = 1;
                break;
            }
            if (!strcmp(TA4, "[") || !strcmp(TA4, ";")) break;
        }
        if (fB == 1) {
            fB = 0;
            t->word_lc[0] = 0;
            strcat(t->word_lc, WL + bp - 1);
            TA4[0] = 0;
            strncat(TA4, WL + bp - 2, 1);
            fe_Tokenize_Dots(t);
            if (strcmp(t->word_lc, "s") && !(!strcmp(TA4, "1") && !strcmp(t->word_lc, "st")) &&
                !(!strcmp(TA4, "2") && !strcmp(t->word_lc, "nd")) && !(!strcmp(TA4, "3") && !strcmp(t->word_lc, "rd")) &&
                strcmp(t->word_lc, "th")) {
                fe_Abbrev_LetterRange(t, &s1a, &s1c);
                for (bx = s1a; bx <= s1c; bx++) {
                    strcpy(T68, fe_abbrev[bx].key);
                    fe_strlwr(T68);
                    strcpy(T11C, T68);
                    strcpy(T1A8, t->word_lc);
                    fe_str_replace_all(T1A8, ".", " ", T1A8);
                    if (!strcmp(t->word_lc, T68) || !strcmp(T1A8, T11C)) {
                        bp += fA;
                        W[0] = 0;
                        strncat(W, t->word[c], bp - 1);
                        buf[0] = 0;
                        strncat(buf, t->text, pos + bp - 2);
                        strcat(buf, " ");
                        strcat(buf, t->text + pos + bp - 2);
                        strcpy(t->text, buf);
                        fB = 1;
                        *end = bp + pos - 1;
                        break;
                    }
                }
            }
        }
    }
    /* "ext12", "rte66", "apt3", "jan5": the prefix and the number */
    fA = 0;
    for (bp = 1; (short)(strlen(EXT) / 4) >= bp; bp++) {
        T11C[0] = 0;
        strncat(T11C, EXT + (bp - 1) * 4, 4);
        strcpy(T11C, fe_str_trim_spaces(T11C));
        if (!strncmp(T11C, WL, strlen(T11C))) {
            s18 = (short)strlen(T11C);
            TA4[0] = 0;
            strncat(TA4, WL + s18, 1);
            if (strcmp(TA4, ".") && !(strcmp(TA4, "0") >= 0 && strcmp(TA4, "9") <= 0)) break;
            fA = 1;
            if (!strcmp(TA4, ".")) s18++;
            if ((short)strlen(WL) < s18 + 1) goto done;
            for (si = s18 + 1; (short)strlen(WL) >= si; si++) {
                TA4[0] = 0;
                strncat(TA4, WL + si - 1, 1);
                if (strcmp(TA4, "0") >= 0 && strcmp(TA4, "9") <= 0) continue;
                if ((short)strlen(WL) == si && fe_strpos(PUNCT, TA4)) continue;
                fA = 0;
                break;
            }
            break;
        }
    }
    if (fA) {
        if (!strncmp(t->word[c], "(", 1)) s18++;
        W[0] = 0;
        strncat(W, t->word[c], s18);
        buf[0] = 0;
        strncat(buf, t->text, pos + s18 - 1);
        strcat(buf, " ");
        strcat(buf, t->text + pos + s18 - 1);
        strcpy(t->text, buf);
        fA = 1;
        *end = pos + s18;
    }

done:
    fe_free(buf);
    /* the word after "~~" (an opening "**") carries the "~" */
    if (t->cur_word > 1 && !strcmp(t->word[c - 1], "~~")) {
        strcpy(t->word[c - 1], "");
        strcpy(T68, "~");
        strcat(T68, W);
        strcpy(W, T68);
        fe_strlwr(W);
    }
    t->prepunc[c] = fe_strdup("    ");
    if (!strcmp(W, t->word[c])) return 0;
    if (!strncmp(t->word[c], "(", 1) && W[0] != '(') strcpy(t->prepunc[c], "(");
    if (strlen(W) < 30) {
        strcpy(t->word[c], W);
    } else {
        fe_free(t->word[c]);
        t->word[c] = fe_strdup(W);
    }
    if (fB || fA) strcpy(t->sepr[c], "");
    return 0;
}

/* ---- Tokenize_SplitSeparators ------------------------------------------------------------------------------------- */
/* The first step for each word of TextExpander_Paragraph: where does the word that starts at pos (1-based) end?
   *end becomes the 1-based position of the first blank, '-', '/' or '+' from pos on, and sep that character ("- "
   when the '-' is followed by a blank, as in "well- known"); when there is none, *end and sep stay as they are.
   Before that, a group "(12)" to "(1234)" of digits at pos is freed of its parentheses in the text ("(415) 555" ->
   " 415 555"), unless nothing follows the ')', and every double blank of the text becomes one. Separators inside
   the word (555-1212, 1/16/95) thus end it here; Tokenize_FirstPass and the number stage put such words together
   again through SEPR. @0x1002e410 */
#ifndef B_Tokenize_SplitSeparators   /* harness: the bridge runs the original instead (num_bridge.c) */
void fe_Tokenize_SplitSeparators(TextProc *t, short pos, short *end, char *sep)
{
    /* The original rebuilds the text in a 984-byte stack buffer ([F+0x20]); a longer paragraph (the control
       procedure lets it reach about 1000 characters, and the tokenizer lengthens it) runs over the saved registers
       of the driver. The port has room for the whole text in both modes. */
    char buf[sizeof t->text + 8];
    char *text = t->text, *p = text + pos - 1;
    short n = 0, k, sp, dash, slash, plus;
    int digits = 0;

    if (*p == '(' && (short)strlen(text) >= pos + 4) {
        for (n = 2; n <= 4 && text[pos + n] != ')'; n++)
            ;
        if (n <= 4) {
            digits = 1;
            for (k = 1; k <= n; k++)
                if (text[pos + k - 1] < '0' || text[pos + k - 1] > '9') { digits = 0; break; }
        }
    }
    if (digits) {
        buf[0] = 0;
        strncat(buf, text, pos - 1);
        strcat(buf, " ");
        strncat(buf, text + pos, n);
        strcat(buf, " ");
        if ((short)strlen(text) >= pos + n + 2) {
            strcat(buf, text + pos + n + 1);
            strcpy(text, buf);
        }
        fe_str_replace_all(text, "  ", " ", text);
    }

    sp = fe_strpos(p, " ");
    dash = fe_strpos(p, "-");
    slash = fe_strpos(p, "/");
    plus = fe_strpos(p, "+");
    if (!sp && !dash && !slash && !plus) return;
    sp = sp ? (short)(pos + sp - 1) : 9999;
    dash = dash ? (short)(pos + dash - 1) : 9999;
    slash = slash ? (short)(pos + slash - 1) : 9999;
    plus = plus ? (short)(pos + plus - 1) : 9999;
    strcpy(sep, "");
    if (dash < sp) {
        sp = dash;
        strcpy(sep, "-");
        if (fe_strpos(p, "-") - fe_strpos(p, " ") == -1) strcpy(sep, "- ");
    }
    if (slash < sp) {
        sp = slash;
        strcpy(sep, "/");
    }
    if (sp > plus) {
        sp = plus;
        strcpy(sep, "+");
    }
    *end = sp;
}
#endif
