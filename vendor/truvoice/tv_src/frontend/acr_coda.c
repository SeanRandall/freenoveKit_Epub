/* Acronym_CodaSplit: the right-to-left coda check of the acronym stage (patent US 5,634,084 item 150; REFERENCE 3.9,
   8.4). Acronym_SyllableParse asks it whether the end of an all-capitals word is a pronounceable syllable coda; when
   it is not, the caller spells the last letter apart: NETX -> NET-X, HUMV -> HUM-V, FASB -> FAS-B.
   Its one caller (10018d52) passes what is left of the word after the onset split, and on every corpus that is 3 or 4
   letters; the five-letter test ("ngth"), the doubled consonant before the last letter and the buffer overruns of
   long words are reached only by calling it directly. Those were checked against the DLL that way (a scratch driver:
   52,508 words of 1 to 200 letters, all equal except 5 on which the DLL itself crashes or hangs). */
#include <string.h>
#include "frontend.h"

#define FRAME 0x284          /* [F+0] .. the return address: 0x274 bytes of locals and 4 saved registers */
#define WMAX 0x7f0           /* longest word the port copies (the original's own buffer holds 59 characters) */
#define XBUF (WMAX + 16)
static unsigned char coda_frame[FRAME + 3 * XBUF + 8] __attribute__((aligned(16)));

static const char VOW[] = "aeiouy", CONS[] = "bcdfghjklmnpqrstvwxyz", STOP[] = "ptkcbdg";

/* The word lists the original copies into its frame at entry, at their frame offsets [F+x]. Its text buffers run on
   into them when the word is long. */
static const struct { unsigned short off; const char *s; } coda_lists[] = {
    {0x20, "hw"},            /* copied but never read */
    {0x24, "lr"},
    {0xc0, "mbghgngm"},      /* the silent-letter endings, looked up by position: mb=1 gh=3 gn=5 gm=7 */
    {0x130, "#bomb#climb#comb#crumb#dumb#lamb#limb#numb#thumb#tomb#womb#"},
    {0x1e8, "#sleigh#neigh#weigh#high#thigh#bligh#nigh#sigh#hugh#burgh#ugh#laugh#cough#dough#although#though#enough#rough#through#borough#thorough#trough#tough#pugh#"},
    {0x16c, "#arraign#deign#feigh#sovereign#reign#foreign#malign#aligh#benign#sign#design#resign#ensign#consign#assign#impugn#repugn#"},
    {0x114, "#diaphragm#phlegm#paradigm#"},
    {0xcc, "#ow#oy#ay#"},
};

static int in(const char *set, const char *s) { return fe_strpos(set, s) != 0; }

/* The string copies of the original, with its overlap behaviour (a long word runs its buffers into each other):
   the inlined strcpy / strcat measure the source first and copy it with REP MOVSD / MOVSB; tv_strncat 10078400
   copies bytes until the source is dword-aligned, then dwords. No write goes past the end of the frame model. */
#define FEND ((char *)coda_frame + sizeof coda_frame - 8)

static void movs(char *d, const char *s, size_t n)
{
    size_t k;
    char v[4];
    for (k = 0; k + 4 <= n; k += 4) {
        if (d + k + 4 > FEND) return;
        memcpy(v, s + k, 4);
        memcpy(d + k, v, 4);
    }
    for (; k < n && d + k < FEND; k++) d[k] = s[k];
}

static void cpy(char *d, const char *s) { movs(d, s, strlen(s) + 1); }

static void cat(char *d, const char *s)
{
    size_t n = strlen(s) + 1;
    movs(d + strlen(d), s, n);
}

static void ncat(char *d, const char *s, size_t count)     /* 10078400 */
{
    size_t n4, rem;
    if (!count) return;
    d += strlen(d);
    while ((unsigned)(size_t)s & 3) {
        char c = *s++;
        if (d >= FEND) return;
        if (!c) { *d = 0; return; }
        *d++ = c;
        if (!--count) { *d = 0; return; }
    }
    rem = count;
    for (n4 = count >> 2; n4; n4--) {
        unsigned char v[4];
        if (d + 4 > FEND || s >= FEND) return;
        memcpy(v, s, 4);
        s += 4;
        if (!v[0]) { d[0] = 0; return; }
        if (!v[1]) { d[0] = v[0]; d[1] = 0; return; }
        if (!v[2]) { d[0] = v[0]; d[1] = v[1]; d[2] = 0; return; }
        memcpy(d, v, 4);
        if (!v[3]) return;
        d += 4;
    }
    for (rem &= 3; rem; rem--) {
        if (d >= FEND) return;
        if (!(*d++ = *s++)) return;
    }
    if (d < FEND) *d = 0;
}

/* dst = the one character at src ("" at the terminator): strcpy(dst, ""); strncat(dst, src, 1) */
static void ch(char *dst, const char *src)
{
    dst[0] = src[0];
    dst[1] = 0;
}

/* w loses its last `drop` characters (2 for a final "'s" when poss is set), through the scratch buffer x. As in the
   original, x is emptied before w is measured: a long word runs into x, and emptying it shortens the word. Fewer than
   `drop` characters leave the whole word (a negative strncat count). */
static void cut(char *w, char *x, int drop, int poss)
{
    int n;
    x[0] = 0;
    if (poss && !strncmp(fe_tail(w, 2), "'s", 2)) drop = 2;
    n = (int)strlen(w) - drop;
    ncat(x, w, n < 0 ? (size_t)-1 : (size_t)n);
    cpy(w, x);
}

/* Is the coda at the end of word acceptable (*ok = 1; otherwise *ok is left as the caller set it)? The word is
   lower-cased, a plural or possessive s is stripped ("'s", "s" after a consonant; not after ss/zs/chs/chz/zzs), then
   a final e, a doubled last letter and trailing vowels. The rest must hold a vowel before a consonant ("CV" in the
   right-to-left C/V pattern); then the last one to five letters are tested against the English coda rules: a vowel
   before a single final consonant, liquid/nasal + stop or fricative clusters (lp, rt, mp, nt, ls, ns, ft, rl...),
   th/ph/sh/ch/ck/ght/tch after a vowel, y/h/w/q/j/x contexts, and the silent-letter words (bomb, sigh, sign,
   phlegm). *stripped is the number of characters removed from the end. @0x1001ac00 */
void fe_Acronym_CodaSplit(TextProc *t, short *ok, char *word, short *stripped)
{
    const unsigned char *fr = fe_frame_of(0x1001ac00);
    unsigned char *F = coda_frame;
    char *W, *T, *P;                   /* the word [F+0x38], a scratch buffer [F+0x84], the C/V pattern [F+0xd8] */
    char *B12 = (char *)F + 0x12, *B18 = (char *)F + 0x18, *B1A = (char *)F + 0x1a, *B1C = (char *)F + 0x1c;
    char *B1E = (char *)F + 0x1e, *B28 = (char *)F + 0x28, *B30 = (char *)F + 0x30;
    char *B74 = (char *)F + 0x74, *B7C = (char *)F + 0x7c;
    size_t n = strlen(word), q;
    short res = *ok, k, x;
    (void)t;

    /* Without fe_fixes the frame is modelled byte for byte: the 60-byte buffers overflow into each other and into the
       word lists (a word of 60 letters or more is cut at [F+0x74]). With fe_fixes each buffer has room for WMAX. */
    memset(F, 0, sizeof coda_frame);
    if (fr) memcpy(F, fr, FRAME);
    for (q = 0; q < sizeof coda_lists / sizeof coda_lists[0]; q++)
        memcpy(F + coda_lists[q].off, coda_lists[q].s, strlen(coda_lists[q].s) + 1);
    if (fe_fixes) {
        W = (char *)F + FRAME; T = W + XBUF; P = T + XBUF;
    } else {
        W = (char *)F + 0x38; T = (char *)F + 0x84; P = (char *)F + 0xd8;
    }
    if (n > WMAX) n = WMAX;            /* (the original would have run over its return address long before) */
    memcpy(W, word, n);
    W[n] = 0;
    fe_strlwr(W);
#define LEN ((int)strlen(W))
#define TL(k) fe_tail(W, k)            /* the last k characters */

    ch(B1E, TL(1));
    ch(B28, TL(2));

    /* a final s or z: the plural / possessive s goes, unless a vowel other than e precedes it */
    if (!strcmp(B1E, "s") || !strcmp(B1E, "z")) {
        if (!in(VOW, B28) || !strcmp(B28, "e") || !strcmp(B28, "'")) {
            if (!strcmp(TL(2), "zs") || !strcmp(TL(3), "chs") || !strcmp(TL(3), "chz") || !strcmp(TL(3), "zzs"))
                goto done;
            if (!strcmp(TL(2), "ss") || !strcmp(TL(2), "zs")) {
                ch(B1A, TL(3));
                if (in(CONS, B1A)) goto done;
            }
            cut(W, T, 1, 1);
        }
    }
    if (!strcmp(TL(1), "e")) cut(W, T, 1, 0);                 /* final e */
    if (!strncmp(TL(1), TL(2), 1)) cut(W, T, 1, 0);          /* doubled last letter */
    for (;;) {                                                   /* trailing vowels */
        cpy(B1E, TL(1));
        if (!in(VOW, B1E)) break;
        cut(W, T, 1, 0);
        if (strlen(W) < 2) goto accept;
    }
    if (strlen(W) < 2) goto accept;
    if (strlen(W) >= 3) {
        /* a doubled consonant before the last letter loses one of its pair */
        ch(B28, TL(2));
        if (!in(VOW, B28) && !strncmp(TL(2), TL(3), 1)) {
            T[0] = 0;                                            /* (before W is measured) */
            ncat(T, W, (size_t)(LEN - 2));
            cat(T, TL(1));
            cpy(W, T);
        }
    }

    /* the C/V pattern, right to left, runs collapsed ("netx" -> "CVC"); no vowel before a consonant: accept */
    P[0] = 0;
    for (k = (short)strlen(W); k >= 1; k--) {
        const char *cv;
        ch(B1E, W + k - 1);
        cv = in(VOW, B1E) ? "V" : "C";
        if (!P[0] || strcmp(P + strlen(P) - 1, cv)) cat(P, cv);
    }
    if (!fe_strpos(P, "CV")) goto accept;

    cpy(B30, TL(2));
    if (!strcmp(B30, "mb") || !strcmp(B30, "gh") || !strcmp(B30, "gn") || !strcmp(B30, "gm"))
        goto silent;
    cpy(B1E, TL(1));                                          /* the last letter */
    cpy(B30, "#");                                            /* "#" + the last two + "#" */
    cat(B30, TL(2));
    cat(B30, "#");
    ch(B28, TL(2));                                              /* the letter before it */
    cpy(B1A, "");
    cpy(B74, "");
    if (strlen(W) >= 3) {
        ch(B1A, TL(3));                                          /* the third from the end */
        cpy(B74, "#");
        ncat(B74, TL(3), 2);
        cat(B74, "#");
    }
    cpy(B18, "");
    cpy(B7C, "");
    if (strlen(W) >= 4) {
        ch(B18, TL(4));                                          /* the fourth from the end */
        cpy(B7C, "#");
        ncat(B7C, TL(4), 2);
        cat(B7C, "#");
    }

    /* vowel + one final consonant */
    if ((in(STOP, B1E) || !strcmp("q", B1E)) && in(VOW, B28)) goto accept;
    if (in("fvsz", B1E) && in(VOW, B28)) goto accept;
    if (in("mn", B1E) && in(VOW, B28)) goto accept;
    if (in((char *)F + 0x24, B1E) && in(VOW, B28)) goto accept;
    if (in((char *)F + 0xcc, B30)) goto accept;                  /* ow oy ay */

    /* the letters around y, h, w (their first occurrence) */
    x = fe_strpos(W, "yy");
    if (x) {
        cpy(B1C, "");
        if ((short)strlen(W) - 1 >= x + 1) ncat(B1C, W + x + 1, 1);
        if (in(CONS, B1C)) goto done;                            /* (also at the end of the word: "" matches) */
    }
    x = fe_strpos(W, "y");
    if (x > 1) {
        ch(B1C, W + x - 2);
        if (in(VOW, B1C)) goto accept;
    }
    x = fe_strpos(W, "h");
    if (x > 1) {
        ch(B1C, W + x - 2);
        if (in(VOW, B1C)) goto accept;
    }
    x = fe_strpos(W, "w");
    if (x > 1) {
        cpy(B1C, "");
        cpy(B12, "");
        ncat(B1C, W + x - 2, 1);
        if (in("aeo", B1C)) goto accept;
        if ((short)strlen(W) > x) ncat(B12, W + x, 1);
        if (in("iu", B1C) && in(CONS, B12)) goto done;
    }
    if (!strcmp("y", B1E) && in(CONS, B28)) goto accept;
    if (!strcmp("q", B28) && !strcmp("s", B1E) && in(VOW, B1A)) goto accept;

    /* q, j, x: not acceptable when a consonant follows (or, for j and x, precedes) */
    x = fe_strpos(W, "q");
    if (x) {
        if ((short)strlen(W) <= x) goto accept;
        ch(B1C, W + x);
        if (in(CONS, B1C)) goto done;
        goto accept;
    }
    x = fe_strpos(W, "j");
    if (!x) {
        if ((fe_strpos(W, "nx") || fe_strpos(W, "xt") || fe_strpos(W, "rx")) && in(VOW, B1A)) goto accept;
        x = fe_strpos(W, "x");
    }
    if (x) {
        if ((short)strlen(W) <= x) goto accept;
        ch(B1C, W + x);
        if (in(CONS, B1C)) goto done;
        if (x <= 1) goto accept;
        ch(B1C, W + x - 2);
        if (in(CONS, B1C)) goto done;
        goto accept;
    }
    if (strlen(W) < 3) goto done;

    /* two-letter codas after a vowel */
    if (in("#ch#ck#", B30) && in(VOW, B1A)) goto accept;
    if (in("#th#ph#sh#", B30) && in(VOW, B1A)) goto accept;
    /* The original has the arguments of this test swapped (strpos("ng", "#ng#") never matches); vowel + "ng" is
       accepted anyway by the n + stop test below. */
    if ((fe_fixes ? in(B30, "ng") : in("ng", B30)) && in(VOW, B1A)) goto accept;
    if (in((char *)F + 0x24, B28) && in("mn", B1E) && in(VOW, B1A)) {
        if (strcmp("l", B28)) goto accept;                       /* rm rn */
        if (strcmp("n", B1E)) goto accept;                       /* lm; "ln" goes on */
    }
    if (in((char *)F + 0x24, B28) && in(STOP, B1E) && in(VOW, B1A)) {
        if (strcmp("l", B28)) goto accept;                       /* r + stop */
        if (strcmp("b", B1E) && strcmp("g", B1E)) goto accept;   /* l + stop except lb, lg */
    }
    if (!strcmp("m", B28) && in("pb", B1E) && in(VOW, B1A)) goto accept;
    if (!strcmp("n", B28) && in("tkdgc", B1E) && in(VOW, B1A)) goto accept;
    if (in((char *)F + 0x24, B28) && in("fvsz", B1E) && in(VOW, B1A)) goto accept;
    if (in("mn", B28) && in("sz", B1E) && in(VOW, B1A)) goto accept;
    if (!strcmp("m", B28) && !strcmp("f", B1E) && in(VOW, B1A)) goto accept;
    if (in("#ptkc#", B28) && !strcmp(B1E, "t") && in(VOW, B1A)) goto accept;
    if (in("s", B28) && in("#ptkc#", B1E) && in(VOW, B1A)) goto accept;
    if (in(STOP, B28) && (!strcmp("s", B1E) || !strcmp("z", B1E)) && in(VOW, B1A)) goto accept;
    /* never true: the consonant set holds no "#" (the intent is unknown, so it stays in both modes) */
    if (!strcmp("y", B1E) && in(CONS, B74)) goto accept;
    if ((!strcmp("#ft#", B30) || !strcmp("#rl#", B30)) && in(VOW, B1A)) goto accept;

    /* three-letter codas after a vowel */
    if (strlen(W) < 4) goto done;
    if (in((char *)F + 0x24, B1A) && in("#th#ph#sh#", B30) && in(VOW, B18)) goto accept;
    if (in("mn", B1A) && !strcmp("#th#", B30) && in(VOW, B18)) goto accept;
    if (!strcmp("m", B1A) && !strcmp("#ph#", B30) && in(VOW, B18)) goto accept;
    /* vowel + "ng" + s/z: swapped arguments again, so it never matches. A final s/z after "ng" is normally stripped
       first; it reaches this test only after a doubled one ("BANGZZ" -> "bangz"), which fe_fixes then accepts. */
    if ((fe_fixes ? in(B74, "ng") : in("ng", B74)) && in("sz", B1E) && in(VOW, B18)) goto accept;
    if (in(STOP, B1A) && !strcmp("#th#", B30) && in(VOW, B18)) goto accept;
    if (in((char *)F + 0x24, B1A) && in("#ch#ck#", B30) && in(VOW, B18)) goto accept;
    if (!strcmp("n", B1A) && in("#ch#ck#", B30) && in(VOW, B18)) goto accept;
    if ((!strncmp("ght", TL(3), 3) || !strncmp("tch", TL(3), 3)) && in(VOW, B18)) goto accept;

    /* vowel + "ngth" (length, strength). In the original the test can never pass: strpos("ng", "#ng#") has its
       arguments swapped, and the fifth letter from the end is appended to whatever [F+0x1c] held (a letter left by
       the y/h/w tests, or stale stack), not put in an empty buffer. */
    if (strlen(W) < 5) goto done;
    if (fe_fixes) cpy(B1C, "");
    ncat(B1C, TL(5), 1);
    if (!(fe_fixes ? in(B7C, "ng") : in("ng", B7C))) goto done;
    if (strcmp("#th#", B30)) goto done;
    if (!in(VOW, B1C)) goto done;
    goto accept;

silent:
    /* mb, gh, gn, gm: accepted when a prefix of three letters or more is one of the words of its list */
    for (k = 3; (short)strlen(W) >= k; k++) {
        static const unsigned short list_at[8] = {0, 0x130, 0, 0x1e8, 0, 0x16c, 0, 0x114};
        short at;
        cpy(T, "#");
        ncat(T, W, (size_t)k);
        cat(T, "#");
        at = fe_strpos((char *)F + 0xc0, B30);
        if (at >= 1 && at <= 7 && list_at[at] && in((char *)F + list_at[at], T)) goto accept;
    }
    goto done;

accept:
    res = 1;
done:
    *ok = res;
    *stripped = (short)(strlen(word) - strlen(W));
#undef LEN
#undef TL
}
