/* Acronym stage: the syllable parser that decides how an upper-case word that is not a known acronym is read, and
   hyphenates it into spelled letters and pronounceable parts (patent 150; REFERENCE 3.9, 8.4). "IBM" -> "I-B-M",
   "AAAS" -> "triple-A-S", "SAE" -> "S-A-E", "AMEN" -> "A-MEN", "CID" -> "C-I-D" (but "CIDER"), "NETX" -> "NET-X". */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

#pragma GCC diagnostic ignored "-Wrestrict"   /* the work buffers share one frame array */

typedef short __attribute__((may_alias)) fshort;

/* The work buffers keep their frame offsets [F+x] (tools/asm_frame.py): the original's 60-byte buffers run on into
   each other when the word is long, and the short locals are passed to the callees by address. SLACK keeps the
   port's own overruns inside the array. */
#define FRAME 0x190
#define SLACK 0x1000
#define FBIG 0x400        /* fe_fixes: size of each text buffer */
#define W10 (*(fshort *)(F + 0x10))   /* the word has a vowel; later: the word ends in a vowel */
#define W12 ((fshort *)(F + 0x12))    /* OnsetSplit / CodaSplit: length of the part split off, minus 1 */
#define W14 (*(fshort *)(F + 0x14))   /* no soft-c / hard-g list word matched */
#define W16 ((fshort *)(F + 0x16))    /* OnsetSplit / CodaSplit: stop */
#define C18 (F + 0x18)    /* one character (2 bytes) */
#define C1A (F + 0x1a)    /* one character (2 bytes) */

static const char VOWELS[] = "aeiouy";                 /* @0x100a1cd8 */
static const char VOWELS5[] = "aeiou";                 /* @0x100a1ce0 */
static const char TRIPLES[] = "aaaeeeiiiooouuu";       /* @0x100a1ce8 */
static const char PAIRS_INIT[] = "aaaeaoeieoiiiuuauiuuiaieiooeueuoyy";   /* @0x100a1cf8: spelled at the start */
static const char PAIRS_FINAL[] = "aaaeaoeieoiiiuuauiuueuyy";             /* @0x100a1d20: spelled at the end */
static const char CONSONANTS[] = "bcdfghjklmnpqrstvwxyz";               /* @0x100a1da8 */

/* buf spelled letter by letter: "A-B-C" */
static void spell(char *dst, const char *buf)
{
    short k;
    dst[0] = 0;
    strncat(dst, buf, 1);
    for (k = 2; k <= (short)strlen(buf); k++) {
        strcat(dst, "-");
        strncat(dst, buf + k - 1, 1);
    }
}

/* Is "#" + the first n characters of lw + "#" in one of the two lists, for some n from n0 to strlen(buf)? */
static int prefix_listed(char *tmp, const char *lw, const char *buf, short n0, const char *l1, const char *l2)
{
    short n;
    for (n = n0; n <= (short)strlen(buf); n++) {
        strcpy(tmp, "#");
        strncat(tmp, lw, n);
        strcat(tmp, "#");
        if (fe_strpos(l1, tmp) || fe_strpos(l2, tmp)) return 1;
    }
    return 0;
}

/* The upper-case word in buf (without morphs; i is not used) is rewritten in place as its reading: letters to
   spell are separated by "-", pronounceable parts are kept whole. Called twice by AcronymInitialismExpander (the
   part before and after a morph). @0x10017630 */
void fe_Acronym_SyllableParse(TextProc *t, short i, char *buf)
{
    unsigned char Fb[FRAME + SLACK], big[7 * FBIG];
    char *F = (char *)Fb;
    const unsigned char *fr = fe_frame_of(0x10017630);
    char *F20, *F5C, *F98, *FD4, *FDC, *F118, *F154;
    short k, n, p;

    (void)i;
    if (fr) memcpy(Fb, fr, FRAME); else memset(Fb, 0, FRAME);
    memset(Fb + FRAME, 0, SLACK);
    if (fe_fixes) {
        /* The original's 60-byte buffers overrun each other (and the return address) on long words; the fixed port
           gives each its own. */
        memset(big, 0, sizeof big);
        F20 = (char *)big;       F5C = F20 + FBIG;  F98 = F5C + FBIG;  FD4 = F98 + FBIG;
        FDC = FD4 + FBIG;        F118 = FDC + FBIG; F154 = F118 + FBIG;
    } else {
        F20 = F + 0x20;  F5C = F + 0x5c;  F98 = F + 0x98;  FD4 = F + 0xd4;   /* FD4 is F98's last 8 bytes */
        FDC = F + 0xdc;  F118 = F + 0x118;  F154 = F + 0x154;
    }

    if (strlen(buf) <= 2) return;

    /* no vowel (y counts): spell it */
    W10 = 0;
    strcpy(F154, "");
    for (k = 1; k <= (short)strlen(buf); k++) {
        strcpy(C1A, "");
        strncat(C1A, buf + k - 1, 1);
        strcpy(C18, C1A);
        fe_strlwr(C18);
        if (fe_strpos(VOWELS, C18)) { W10 = 1; break; }
        strcat(F154, C1A);
        strcat(F154, "-");
    }
    if (W10 == 0) {
        strcpy(buf, "");
        strncat(buf, F154, strlen(F154) - 1);
        return;
    }

    /* a vowel three times at the start or the end: "AAAS" -> "triple-A-S", "XIII" -> "X-triple-I" */
    if (strlen(buf) > 3) {
        strcpy(F5C, "");
        strncat(F5C, buf, 3);
        fe_strlwr(F5C);
        strcpy(F20, buf + strlen(buf) - 3);
        fe_strlwr(F20);
        for (k = 1; k <= (short)(strlen(TRIPLES) / 3); k++) {
            const char *tr = TRIPLES + 3 * k - 3;
            if (!strncmp(F5C, tr, 3)) {
                strcpy(FDC, "triple-");
                strncat(FDC, buf, 1);
                strcat(FDC, "-");
                strcat(FDC, buf + 3);
                strcpy(buf, FDC);
                return;
            }
            if (!strncmp(F20, tr, 3)) {
                strcpy(FDC, "");
                strncat(FDC, buf, strlen(buf) - 3);
                strcat(FDC, "-triple-");
                strcat(FDC, buf + strlen(buf) - 1);
                strcpy(buf, FDC);
                return;
            }
        }
    }

    /* F98 collects the front part, F118 the back part */
    strcpy(F98, "");
    strcpy(F118, "");
    if (strlen(buf) < 2) goto finish;

    /* initial vowels: an unusual pair is spelled ("AE..." -> "A-E-..."); a vowel before a consonant and a vowel is
       split off in a word of up to 4 letters ("AMEN" -> "A-MEN") */
    strcpy(F20, buf);
    fe_strlwr(F20);
    strcpy(C1A, "");
    strncat(C1A, F20, 1);
    if (fe_strpos(VOWELS5, C1A)) {
        strcpy(C18, "");
        strncat(C18, F20 + 1, 1);
        if (fe_strpos(VOWELS5, C18)) {
            for (k = 1; k <= (short)strlen(PAIRS_INIT) / 2; k++)
                if (!strncmp(F20, PAIRS_INIT + 2 * k - 2, 2)) {
                    strncat(F98, buf, 1);
                    strcat(F98, "-");
                    strncat(F98, buf + 1, 1);
                    strcat(F98, "-");
                    strcpy(F5C, "");
                    strcat(F5C, buf + 2);
                    strcpy(buf, F5C);
                    break;
                }
        } else {
            strcpy(C1A, "");
            strncat(C1A, F20 + 2, 1);
            if (strlen(buf) <= 4 && fe_strpos(VOWELS, C1A)) {
                strncat(F98, buf, 1);
                strcat(F98, "-");
                strcpy(F5C, buf + 1);
                strcpy(buf, F5C);
            }
        }
    }

    /* a final unusual vowel pair is spelled ("...UI" -> "...-U-I"); any final vowel sets W10, which keeps the coda
       pass off after the onset pass */
    if (strlen(buf) < 2) goto finish;
    W10 = 0;
    strcpy(F20, buf);
    fe_strlwr(F20);
    strcpy(C1A, F20 + strlen(F20) - 1);
    strcpy(C18, "");
    strncat(C18, F20 + strlen(F20) - 2, 1);
    if (fe_strpos(VOWELS, C1A)) {
        if (fe_strpos(VOWELS, C18))
            for (k = 1; k <= (short)strlen(PAIRS_FINAL) / 2; k++)
                if (!strncmp(F20 + strlen(F20) - 2, PAIRS_FINAL + 2 * k - 2, 2)) {
                    strcpy(F5C, "");
                    strncat(F5C, buf + strlen(buf) - 2, 1);
                    strcat(F5C, "-");
                    strcat(F5C, buf + strlen(buf) - 1);
                    strcat(F5C, "-");
                    strcat(F5C, F118);
                    strcpy(F118, F5C);
                    strcpy(F5C, "");
                    strncat(F5C, buf, strlen(buf) - 2);
                    strcpy(buf, F5C);
                    break;
                }
        W10 = 1;
    }

    if (strlen(buf) < 2) goto finish;
    if (!strcmp(F98, "")) {
        /* initial x: "XO..."/"XU..." -> "X-O...", "XE..."/"XI..."/"XY..."/"XA..." go to the coda check at once */
        strcpy(F20, buf);
        fe_strlwr(F20);
        strcpy(C1A, "");
        strncat(C1A, F20 + 1, 1);
        if (!strncmp(F20, "x", 1)) {
            if (fe_strpos("ou", C1A)) {
                strncat(F98, buf, 1);
                strcat(F98, "-");
                strcpy(F5C, buf + 1);
                strcpy(buf, F5C);
            } else if (fe_strpos("eiya", C1A))
                goto coda;
        }
    }
    if (!strcmp(F98, "")) {
        /* initial ce / ci: spelled unless the word starts with a soft-c word ("CID" -> "C-I-D", "CIDER") */
        strcpy(F20, buf);
        fe_strlwr(F20);
        if (!strncmp(F20, "ce", 2) || !strncmp(F20, "ci", 2)) {
            W14 = !prefix_listed(FDC, F20, buf, 4, fe_acr_100a1de0[0], fe_acr_100a1de0[1]);
            if (W14) {
                spell(F20, buf);
                strcpy(buf, F20);
                return;
            }
        }
        /* initial ge / gi: spelled if the g would be read soft ("GEM", "GIF" -> "G-I-F"), unless the word starts with a
           hard-g word, or gee/gei/gie, or gel/gem/gig/gin/gene before a digit or at the end */
        strcpy(F20, buf);
        fe_strlwr(F20);
        if (!strncmp(F20, "ge", 2) || !strncmp(F20, "gi", 2)) {
            if (!strncmp(F20, "gee", 3) || !strncmp(F20, "gei", 3) || !strncmp(F20, "gie", 3)) goto split;
            if (!strncmp(F20, "gel", 3) || !strncmp(F20, "gem", 3) || !strncmp(F20, "gig", 3)
                || !strncmp(F20, "gin", 3)) {
                if (strlen(F20) <= 3) goto split;
                if (strncmp(F20 + 3, "0", 1) >= 0 && strncmp(F20 + 3, "9", 1) <= 0) goto split;
            }
            if (!strncmp(F20, "gene", 4)) {
                if (strlen(F20) <= 4) goto split;
                if (strncmp(F20 + 4, "0", 1) >= 0 && strncmp(F20 + 4, "9", 1) <= 0) goto split;
            }
            W14 = !prefix_listed(FDC, F20, buf, 3, fe_acr_100a1de0[2], fe_acr_100a1de0[3]);
            if (W14) {
                spell(F20, buf);
                strcpy(buf, F20);
                return;
            }
        }
    }

split:
    /* left to right: OnsetSplit decides whether to stop; else the leading vowels, one consonant (two when doubled)
       and "-" go to F98 */
    fe_Acronym_JoinHyphen(t, buf, F98);
    if (strlen(buf) >= 2) {
        *W16 = 0;
        do {
            fe_Acronym_OnsetSplit(t, W16, buf, W12);
            if (*W16) goto onset_done;
            strcpy(FD4, VOWELS5);
            while (strlen(buf) > 2) {
                strcpy(C1A, "");
                strncat(C1A, buf, 1);
                fe_strlwr(C1A);
                if (fe_strpos(FD4, C1A)) {
                    strncat(F98, buf, 1);
                    strcpy(F5C, buf + 1);
                    strcpy(buf, F5C);
                    strcpy(FD4, VOWELS);
                    continue;
                }
                strncat(F98, buf, 1);
                strcpy(F5C, buf + 1);
                strcpy(buf, F5C);
                if (!strncmp(buf, F98 + strlen(F98) - 1, 1) && (short)strlen(F98) > 1) {
                    strncat(F98, buf, 1);
                    strcpy(F5C, buf + 1);
                    strcpy(buf, F5C);
                }
                strcat(F98, "-");
                break;
            }
            fe_Acronym_JoinHyphen(t, buf, F98);
        } while (strlen(buf) >= 2);
    }
    goto finish;
onset_done:
    if (strlen(buf) < 2 || W10 != 0) goto finish;

coda:
    /* right to left: CodaSplit gives the length of the final part, which goes to the front of F118 */
    *W16 = 0;
    do {
        fe_Acronym_CodaSplit(t, W16, buf, W12);
        if (*W16) break;
        n = *W12;
        strcpy(F5C, buf + strlen(buf) - n - 1);
        strcat(F5C, "-");
        strcat(F5C, F118);
        strcpy(F118, F5C);
        strcpy(F5C, "");
        strncat(F5C, buf, strlen(buf) - n - 1);
        strcpy(buf, F5C);
        fe_Acronym_JoinHyphen(t, buf, F98);
    } while (strlen(buf) >= 2);

finish:
    /* F98 + what is left + F118 */
    if (strcmp(F118, "") && !strcmp(F118 + strlen(F118) - 1, "-")) {
        strcpy(F5C, "");
        strncat(F5C, F118, strlen(F118) - 1);
        strcpy(F118, F5C);
    }
    if (strcmp(buf, "")) {
        strcat(F98, buf);
        if (strcmp(F118, "")) {
            strcat(F98, "-");
            strcat(F98, F118);
        }
    } else if (strcmp(F118, ""))
        strcat(F98, F118);
    else {
        strcpy(F5C, "");
        strncat(F5C, F98, strlen(F98) - 1);
        strcpy(F98, F5C);
    }
    strcpy(buf, F98);

    /* "qu" before a consonant is read "KEW" */
    strcpy(F5C, buf);
    fe_strlwr(F5C);
    p = fe_strpos(F5C, "qu");
    if (!p || (short)strlen(F5C) < p + 2) return;
    strcpy(C1A, "");
    strncat(C1A, F5C + p + 1, 1);
    if (!fe_strpos(CONSONANTS, C1A)) return;
    strcpy(F98, "");
    strncat(F98, buf, p - 1);
    strcat(F98, "KEW");
    strcat(F98, buf + p + 1);
    strcpy(buf, F98);
}

/* When exactly two letters are left in rest, they are spelled onto the end of out: out + "X-Y-", rest "". The
   original builds this in a 60-byte buffer. @0x1001cdf0 */
void fe_Acronym_JoinHyphen(TextProc *t, char *rest, char *out)
{
    char tmp[60 + SLACK];
    (void)t;
    if (strlen(rest) != 2) return;
    strcpy(tmp, out);
    strncat(tmp, rest, 1);
    strcat(tmp, "-");
    strcat(tmp, rest + 1);
    strcat(tmp, "-");
    strcpy(out, tmp);
    strcpy(rest, "");
}
