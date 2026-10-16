/* Acronym stage: the left-to-right onset check (patent 150: "V-RAM", "M-PEG"; REFERENCE 3.9, 8.4). */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

#pragma GCC diagnostic ignored "-Wrestrict"   /* the work buffers share one frame array */

/* The work buffers keep their frame offsets (tools/asm_frame.py of 10019380): the word is copied into a 60-byte
   buffer and runs on into the next ones when it is longer, and a short argument is read past its end. Without
   fe_fixes the frame is modelled byte for byte; with fe_fixes the word, the scratch buffer and the (unused) CV
   pattern get their own space after it. */
#define FRAME 0x28c        /* locals, up to the return address */
#define C5 (F + 0x14)      /* 5th letter (2) */
#define C4 (F + 0x16)      /* 4th letter (2) */
#define C3 (F + 0x18)      /* 3rd letter (2) */
#define C1 (F + 0x1a)      /* 1st letter (2) */
#define LR (F + 0x1c)      /* "lr" */
#define P2 (F + 0x20)      /* the first 2 letters, then "#xy#" (8) */
#define X1 (F + 0x28)      /* 2nd letter (8) */
#define HWY (F + 0x30)     /* "hwy" */
#define P23 (F + 0x34)     /* "#" letters 2-3 "#" (8) */
#define P34 (F + 0x3c)     /* "#" letters 3-4 "#" (8) */
#define P3 (F + 0x44)      /* the first 3 letters (8) */
#define ONS (F + 0xc4)     /* "gnptpsrhghkn": selects the word list below */

/* MSVC's strcpy / strcat as inlined (source length first, then a forward copy) and the CRT strncat 10078400. The
   buffers overlap when the word is long. */
static void cpy(char *d, const char *s)
{
    size_t n = strlen(s) + 1;
    for (size_t k = 0; k < n; k++) d[k] = s[k];
}
static void cat(char *d, const char *s) { cpy(d + strlen(d), s); }
static void ncat(char *d, const char *s, int n)
{
    d += strlen(d);
    while (n-- > 0)
        if (!(*d++ = *s++)) return;
    *d = 0;
}
/* d = the character at s ("" at its end) */
static void ch1(char *d, const char *s) { d[0] = 0; ncat(d, s, 1); }

/* Can the upper-case word s start a spoken syllable? The initial vowels are stripped first (AMEN -> MEN, ETON -> TON),
   then the first consonants are checked against the English onsets: C + vowel, C + y, qu, j/x + vowel, wh wr sph, th
   ph sh ch ck + vowel, stop + l/r (not tl dl), stop + h/w, ps, fl fr sl, sw sh, sp st sk sc sm sn, spl spr str scr
   (not stl), chr, chw, sch + ... + vowel, and the silent-letter words from fe_acr_100a27b0 (knack, gnome, psalm,
   rhythm, ...). *ok = 1 when it can; otherwise *ok is left as it is (the caller sets 0, and then spells the word).
   *skip = the number of vowels stripped. 16-bit results. @0x10019380 */
void fe_Acronym_OnsetSplit(TextProc *t, short *ok, char *s, short *skip)
{
    static const char *const V6 = "aeiouy", *const CONS = "bcdfghjklmnpqrstvwxyz";
    const unsigned char *fr = fe_frame_of(0x10019380);
    int n = (int)strlen(s);
    size_t size = FRAME + 3 * (size_t)n + 0x200;
    char *F = fe_malloc(size), *W, *V, *P;
    const char *list;
    short r = *ok, bx, q;
    (void)t;

    memset(F, 0, size);
    if (fr) memcpy(F, fr, FRAME);
    if (fe_fixes) {
        W = F + FRAME + 16; V = W + n + 16; P = V + n + 16;
    } else {
        W = F + 0x4c; V = F + 0x88; P = F + 0xd4;
    }
    cpy(HWY, "hwy");
    cpy(LR, "lr");
    cpy(ONS, "gnptpsrhghkn");
    cpy(F + 0x154, fe_acr_100a27b0[5]);   /* gn */
    cpy(F + 0x130, fe_acr_100a27b0[4]);   /* pt */
    cpy(F + 0x110, fe_acr_100a27b0[3]);   /* ps */
    cpy(F + 0x1b4, fe_acr_100a27b0[2]);   /* rh */
    cpy(F + 0x180, fe_acr_100a27b0[1]);   /* gh */
    cpy(F + 0x210, fe_acr_100a27b0[0]);   /* kn */
    cpy(W, s);
    fe_strlwr(W);

    /* The consonant/vowel pattern ("CVCV", y a vowel after the first letter) is built and never used. */
    cpy(P, "");
    cpy(V, "aeiou");
    for (bx = 1; (short)strlen(W) >= bx; bx++) {
        ch1(C1, W + bx - 1);
        const char *cv = fe_strpos(V, C1) ? "V" : "C";
        if (!*P || strcmp(P + strlen(P) - 1, cv)) cat(P, cv);
        cpy(V, "aeiouy");
    }

    /* initial vowels: A-MEN, E-TON */
    for (;;) {
        ch1(C1, W);
        if (!fe_strpos("aeiou", C1)) break;
        cpy(V, W + 1);
        cpy(W, V);
        if (strlen(W) < 2) goto accept;
    }

    ch1(C1, W);
    if (fe_strpos(CONS, C1) && !strncmp(W + 1, "y", 1)) goto accept;

    /* a q anywhere: qu, and a final q or qs. The test of the letter before the first q, for q, never fails. */
    q = fe_strpos(W, "q");
    if (q) {
        if ((short)strlen(W) > q) {
            const char *after = W + q;
            if (strncmp(after, "s", 1) || (short)((short)strlen(W) - q) != 1)
                if (strncmp(after, "u", 1)) goto done;
        }
        if (q <= 1 || strncmp(W + q - 2, "q", 1)) goto accept;
        goto done;
    }

    P2[0] = 0;
    ncat(P2, W, 2);
    if (!strcmp(P2, "gn") || !strcmp(P2, "pt") || !strcmp(P2, "ps") || !strcmp(P2, "rh") || !strcmp(P2, "gh") ||
        !strcmp(P2, "kn"))
        goto silent;

    ch1(C1, W);
    cpy(P2, "#");
    ncat(P2, W, 2);
    cat(P2, "#");
    ch1(X1, W + 1);
    cpy(P23, "#");
    ncat(P23, W + 1, 2);
    cat(P23, "#");
    ch1(C3, W + 2);

    /* one consonant and a vowel */
    if (fe_strpos(LR, C1) && fe_strpos(V6, X1)) goto accept;
    if (fe_strpos("fvsz", C1) && fe_strpos(V6, X1)) goto accept;
    if (fe_strpos("ptkcbdg", C1) && fe_strpos(V6, X1)) goto accept;
    if (fe_strpos("mn", C1) && fe_strpos(V6, X1)) goto accept;
    if (fe_strpos(HWY, C1) && fe_strpos(V6, X1)) goto accept;
    if (!strcmp(C1, "j") || !strcmp(C1, "x")) {
        if (fe_strpos(CONS, X1)) goto done;
        goto accept;
    }
    if (!strncmp(W, "wh", 2) || !strncmp(W, "wr", 2) || !strncmp(W, "sph", 3)) goto accept;

    /* The length tests use the argument, not the word left after the vowels: past its end the letters are "", which
       every set below contains (ASPR -> A-SPR). Fixed: the word's length. */
#define LEN ((size_t)(fe_fixes ? strlen(W) : strlen(s)))
    if (LEN < 3) goto done;
    /* two consonants and a vowel */
    if (fe_strpos("#th#ph#sh#", P2) && fe_strpos(V6, C3)) goto accept;
    if (fe_strpos("#ch#ck#", P2) && fe_strpos(V6, C3)) goto accept;
    if (fe_strpos("ptkcbdg", C1) && fe_strpos(LR, X1) && fe_strpos(V6, C3)) {
        if (strcmp(X1, "l")) goto accept;
        if (strcmp(C1, "t") && strcmp(C1, "d")) goto accept;
    }
    if (fe_strpos("ptkcbdg", C1) && fe_strpos(HWY, X1) && fe_strpos(V6, C3)) {
        if (!strcmp(X1, "w") && (!strcmp(C1, "p") || !strcmp(C1, "b"))) goto ps;
        if (strcmp(X1, "h")) goto accept;
        if (strcmp(C1, "b") && strcmp(C1, "d")) goto accept;
    }
ps:
    if (!strcmp(C1, "p") && !strcmp(X1, "s") && fe_strpos(V6, C3)) goto accept;
    if (fe_strpos("fs", C1) && fe_strpos(LR, X1) && fe_strpos(V6, C3)) {
        if (strcmp(X1, "r")) goto accept;
        if (strcmp(C1, "s")) goto accept;
    }
    if (fe_strpos("fvsz", C1) && fe_strpos(HWY, X1) && fe_strpos(V6, C3)) {
        if (!strcmp(X1, "w") && (!strcmp(C1, "f") || !strcmp(C1, "v"))) goto s2;
        if (strcmp(X1, "h")) goto accept;
        if (!strcmp(C1, "s")) goto accept;
    }
s2:
    if (!strcmp(C1, "s") && fe_strpos("#ptkc#", X1) && fe_strpos(V6, C3)) goto accept;
    if (!strcmp(C1, "s") && fe_strpos("mn", X1) && fe_strpos(V6, C3)) goto accept;

    /* three consonants and a vowel */
    if (LEN < 4) goto done;
    ch1(C4, W + 3);
    if (!strcmp(C1, "s") && fe_strpos("#ptkc#", X1) && fe_strpos(LR, C3) && fe_strpos(V6, C4)) {
        if (strcmp(C3, "l")) goto accept;
        if (strcmp(X1, "t")) goto accept;
    }
    if (fe_strpos("#ch#ck#", P2) && fe_strpos(LR, C3) && fe_strpos(V6, C4)) goto accept;
    if (fe_strpos("#ch#ck#", P2) && fe_strpos(HWY, C3) && fe_strpos(V6, C4)) {
        if (strcmp(P2, "#ch#")) goto accept;
        if (strcmp(C3, "w") && strcmp(C3, "h")) goto accept;
    }
    if (!strcmp(C1, "s") && fe_strpos("#ch#ck#", P23) && fe_strpos(V6, C4)) goto accept;
    /* thl and phw are meant to be refused, but the original compares "#th#" / "#ph#" with "th" / "ph", so they never
       are. Fixed: compared with "#th#" / "#ph#". */
    if (fe_strpos("#th#ph#sh#", P2) && fe_strpos(LR, C3) && fe_strpos(V6, C4)) {
        if (strcmp(C3, "l")) goto accept;
        if (strcmp(P2, fe_fixes ? "#th#" : "th")) goto accept;
    }
    if (fe_strpos("#th#ph#sh#", P2) && fe_strpos(HWY, C3) && fe_strpos(V6, C4)) {
        if (strcmp(C3, "w")) goto accept;
        if (strcmp(P2, fe_fixes ? "#ph#" : "ph")) goto accept;
    }
    if (fe_strpos("#sh#", P2) && fe_strpos("#ptkc#", C3) && fe_strpos(V6, C4)) goto accept;
    if (fe_strpos("#sh#", P2) && fe_strpos("mn", C3) && fe_strpos(V6, C4)) goto accept;

    /* four consonants and a vowel */
    if (LEN < 5) goto done;
#undef LEN
    ch1(C5, W + 4);
    if (!strcmp(C1, "s") && fe_strpos("#ch#ck#", P23) && fe_strpos(LR, C4) && fe_strpos(V6, C5)) goto accept;
    cpy(P34, "#");
    ncat(P34, W + 2, 2);
    cat(P34, "#");
    if (fe_strpos("#sh#", P2) && fe_strpos("#ch#ck#", P34) && fe_strpos(V6, C5)) goto accept;
    cpy(P3, "");
    ncat(P3, W, 3);
    if (fe_strpos("sch", P3) && fe_strpos("#ptkc#", C4) && fe_strpos(V6, C5)) goto accept;
    if (fe_strpos("sch", P3) && fe_strpos("mn", C4) && fe_strpos(V6, C5)) goto accept;
    goto done;

    /* gn pt ps rh gh kn: a prefix of 3 or more letters must be in that onset's word list (gnome, psych, rhythm).
       A word of 58 or more letters runs on into the scratch buffer that receives its prefix, so it never ends and the
       original writes up the stack until it faults; the port stops at the end of its frame array. */
silent:
    for (bx = 3; (short)strlen(W) >= bx; bx++) {
        if (V + bx + 3 > F + size) goto done;
        cpy(V, "#");
        ncat(V, W, bx);
        cat(V, "#");
        switch (fe_strpos(ONS, P2)) {
        case 1: list = F + 0x154; break;
        case 3: list = F + 0x130; break;
        case 5: list = F + 0x110; break;
        case 7: list = F + 0x1b4; break;
        case 9: list = F + 0x180; break;
        case 11: list = F + 0x210; break;
        default: list = NULL;
        }
        if (list && fe_strpos(list, V)) goto accept;
    }
    goto done;

accept:
    r = 1;
done:
    *ok = r;
    *skip = (short)((short)strlen(s) - (short)strlen(W));
    fe_free(F);
}
