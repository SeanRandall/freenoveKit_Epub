/* Acronym / initialism expander (patent US 5,634,084 item 150; REFERENCE 3.9, 8.4): "IBM" -> "eye-B-M",
   "YMCA" -> "Y-M-C-Aye", "LabVIEW" -> "Lab-VIEW", "MSDOS" -> "m-s-dos", "NETX" -> "NET-X", "DO NOT" unchanged. */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

#pragma GCC diagnostic ignored "-Wrestrict"   /* the work buffers share one frame array */

/* ---- Acronym_CountUpper ------------------------------------------------------------------------------------- */
/* *n = the number of A-Z in s. @0x1001cf30 */
void fe_Acronym_CountUpper(TextProc *t, short *n, char *s)
{
    short k, cnt = 0;

    (void)t;
    for (k = 1; (short)strlen(s) >= k; k++)
        if ((unsigned char)s[k - 1] >= 'A' && (unsigned char)s[k - 1] <= 'Z') cnt++;
    *n = cnt;
}

/* ---- string helpers ------------------------------------------------------------------------------------------ */
/* The original's inlined strcpy/strcat measure the source first and copy it forward a dword at a time; its strncat
   (10078400) copies up to n characters (n unsigned: a negative count copies everything; 0 writes nothing). The
   copies into the object's 60-byte fields and the frame's buffers run on into their neighbours in the original, as
   here. With fe_fixes every copy is cut to the size of its buffer (cap). */
static int g_fix;

static void fwd(char *d, const char *s, size_t n)
{
    size_t k;
    for (k = 0; k + 4 <= n; k += 4) {
        char b[4];
        memcpy(b, s + k, 4);
        memcpy(d + k, b, 4);
    }
    for (; k < n; k++) d[k] = s[k];
}

static void cpy(char *d, const char *s, int cap)
{
    size_t n = strlen(s) + 1;
    if (g_fix && n > (size_t)cap) {
        memmove(d, s, cap - 1);
        d[cap - 1] = 0;
        return;
    }
    fwd(d, s, n);
}

static void cat(char *d, const char *s, int cap)
{
    size_t n = strlen(s) + 1, l = strlen(d);
    if (g_fix) {
        if (l >= (size_t)cap - 1) return;
        if (l + n > (size_t)cap) {
            memmove(d + l, s, cap - 1 - l);
            d[cap - 1] = 0;
            return;
        }
    }
    fwd(d + l, s, n);
}

static void ncat(char *d, const char *s, int cnt, int cap)
{
    unsigned n = (unsigned)cnt;
    char *e = d + strlen(d);
    if (!n) return;
    while (n--) {
        if (g_fix && e - d >= cap - 1) { *e = 0; return; }
        if (!(*e++ = *s++)) return;
    }
    *e = 0;
}

/* MSVC atoi (10078370 -> 100782c0): C-locale blanks, a sign, digits; wraps. */
static int acr_atoi(const char *p)
{
    const unsigned char *s = (const unsigned char *)p;
    unsigned v = 0;
    int neg;
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    neg = *s == '-';
    if (*s == '-' || *s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return (int)(neg ? 0u - v : v);
}

/* str_replace_all(s, find, repl, s); with fe_fixes the result is cut to the buffer */
static void repl(char *s, const char *find, const char *rep, int cap)
{
    if (g_fix) {
        char tmp[1100];
        fe_str_replace_all(s, find, rep, tmp);
        cpy(s, tmp, cap);
    } else
        fe_str_replace_all(s, find, rep, s);
}

#define SL(s) ((short)strlen(s))

/* ---- AcronymInitialismExpander ----------------------------------------------------------------------------- */
/* The locals keep their frame offsets [F+x] (tools/asm_frame.py): long words run on from one buffer into the next as
   in the original, and the flags S18/S1a are read without being set on the US/UK path. */
#define FRAME 0x1d8
#define SLACK 0x800
typedef short __attribute__((may_alias)) ashort;
#define S(x) (*(ashort *)(F + (x)))
#define S10 S(0x10)    /* loop index; first '-' position */
#define S12 S(0x12)    /* start of the lower-case part; offset into out_text */
#define S14 S(0x14)    /* word i-1 is no upper-case neighbour */
#define S16 S(0x16)    /* word i+1 is no upper-case neighbour */
#define S18 S(0x18)    /* the first part is a morph: no syllable parse */
#define S1A S(0x1a)    /* the second part is a morph */
#define S1C S(0x1c)    /* the result */
#define S22 S(0x22)    /* upper-case letters (Acronym_CountUpper) */
#define S24 S(0x24)    /* the word is split into F_AC + F_E8 */
#define S30 S(0x30)    /* split found by case or digits */
#define F1E (F + 0x1e)     /* 2: a leading character */
#define F20 (F + 0x20)     /* 2 */
#define F26 (F + 0x26)     /* 2 */
#define F28 (F + 0x28)     /* 4: the suffix taken off ("'s", "d") */
#define F34 (F + 0x34)     /* 60 */
#define F70 (F + 0x70)     /* 60 */
#define FAC (F + 0xac)     /* 60: first part */
#define FE8 (F + 0xe8)     /* 60: second part */
#define F124 (F + 0x124)   /* 60: a word to count */
#define F160 (F + 0x160)   /* 60: leading digits and quotes */
#define F19C (F + 0x19c)   /* 60: trailing digits and quotes */

/* two-letter words that are said as words ("#yo" lacks its closing '#' [original]) */
static const char s_two[] = "#ad#ah#an#as#at#aw#be#by#do#ed#eh#el#fa#go#ha#he#hi#ho#hy#if#in#is#it#jo#lo#me#my#no#of#"
                            "oh#on#or#ow#oy#so#to#uh#up#we#yo";
static const char s_two_fixed[] = "#ad#ah#an#as#at#aw#be#by#do#ed#eh#el#fa#go#ha#he#hi#ho#hy#if#in#is#it#jo#lo#me#my#"
                                  "no#of#oh#on#or#ow#oy#so#to#uh#up#we#yo#";
/* a letter plus "ed", 5 characters each ("OKed" -> "O-Kayed"), @0x100a20b8 */
static const char s_letter_ed[] = "Aide Beed Ceed Deed Eed  Efd  Geed AchedEyed JayedKayedEld  Emd  End  Owed Peed Qude "
                                  "Ared Est  Teed Yood Veed WWed Exed Wide Zeed ";

/* out_text = the letters of w joined by '-' ("NYSE" -> "N-Y-S-E") */
static void spell(char *F, char *out, const char *w)
{
    out[0] = 0;
    for (S10 = 1; SL(w) - 1 >= S10; S10++) {
        ncat(out, w + S10 - 1, 1, 60);
        cat(out, "-", 60);
    }
    cat(out, w + strlen(w) - 1, 60);
}

/* Is the word at j no upper-case neighbour: absent, fewer than 2 capitals, or punctuation / a separator (after
   word g) between it and the word i? */
static short not_upper(TextProc *t, char *F, int j, int g, int ok)
{
    if (!ok) return 1;
    cpy(F124, t->word[j], 60);
    fe_Acronym_CountUpper(t, &S22, F124);
    return S22 < 2 || SL(t->punc[g]) > 0 || SL(t->sepr[g]) > 0;
}

/* W keeps its first n characters (n unsigned in strncat); the suffix mark sfx ("'s", "d") goes to F28. With gone,
   the text taken off is kept there. */
static void cut_suffix(char *F, char *W, int n, const char *sfx, char *gone)
{
    if (gone && (unsigned)n < strlen(W)) strcpy(gone, W + n);
    F34[0] = 0;
    ncat(F34, W, n, 60);
    cpy(W, F34, 60);
    cat(F28, sfx, 4);
}

/* @0x10013460 */
void fe_AcronymInitialismExpander(TextProc *t, short i, short *res)
{
    unsigned char Fb[FRAME + SLACK] __attribute__((aligned(4)));
    char *F = (char *)Fb;
    char *T = (char *)t;
    char *W = T + 0x2aa9;      /* word_as_typed */
    char *OUT = T + 0x2a54;    /* out_text */
    char *LC = T + 0x2a18;     /* word_lc */
    const char *two = fe_fixes ? s_two_fixed : s_two;
    const unsigned char *fr = fe_frame_of(0x10013460);
    const char *src;
    char gone[64] = "";   /* fe_fixes: the "ed" taken off */
    short si, pos;

    g_fix = fe_fixes;
    if (fr) memcpy(Fb, fr, FRAME); else memset(Fb, 0, FRAME);
    memset(Fb + FRAME, 0, SLACK);
    S1C = 0;
    F28[0] = 0;
    F160[0] = 0;

    /* leading digits and quotes go to F160: "'IBM" */
    for (;;) {
        F1E[0] = 0;
        ncat(F1E, W, 1, 2);
        if (!acr_atoi(F1E) && F1E[0] != '"' && F1E[0] != '\'' && F1E[0] != '~') break;
        cat(F160, F1E, 60);
        F34[0] = 0;
        cat(F34, W + 1, 60);
        cpy(W, F34, 60);
    }
    /* from the first later digit (atoi), quote or '*' on, the rest goes to F19C: "IBM's*", "PC2" */
    F19C[0] = 0;
    for (si = 1; SL(W) - 1 >= si; si++) {
        char *p = W + si;
        if (acr_atoi(p) || *p == '"' || *p == '\'' || *p == '*') {
            cat(F19C, p, 60);
            F34[0] = 0;
            ncat(F34, W, si, 60);
            cpy(W, F34, 60);
            break;
        }
    }

    /* plural and past: "IBMs", "IBM's" -> "IBM" + "'s"; "OKed", "OK'ed" -> "OK" + "d" (gone: "ed",
       "'ed", for fe_fixes) */
    cpy(F124, W, 60);
    fe_Acronym_CountUpper(t, &S22, F124);
    if (!strcmp(fe_tail(W, 1), "s") && S22 > 1
        && (SL(W) - S22 == 1 || (SL(W) - S22 == 2 && !strcmp(fe_tail(W, 2), "'s"))))
        cut_suffix(F, W, S22, "'s", NULL);
    else if (t->ilc[i] && !strcmp(fe_tail(W, 2), "'s"))
        cut_suffix(F, W, (int)strlen(W) - 2, "'s", NULL);
    if (!strcmp(fe_tail(W, 2), "ed") && S22 >= 2
        && (SL(W) - S22 == 2 || (SL(W) - S22 == 3 && !strcmp(fe_tail(W, 3), "'ed"))))
        cut_suffix(F, W, S22, "d", gone);
    else if (t->ilc[i] && !strcmp(fe_tail(W, 3), "'ed"))
        cut_suffix(F, W, (int)strlen(W) - 3, "d", gone);

    cpy(OUT, W, 60);
    fe_Acronym_KnownLists(t, i, &S1C);
    if (S1C) {
        /* a known spelling; a leading '~' is dropped when the word had one in front */
        if (OUT[0] != '~' || !fe_strpos(F160, "~")) goto post;
        F34[0] = 0;
        cat(F34, OUT + 1, 60);
        goto set_f34;
    }

    if (S22 < 2) goto out;
    if (S22 == 2 && (fe_strpos(W, "Mc") == 1 || fe_strpos(W, "Mac") == 1 || fe_strpos(W, "O'") == 1)) goto out;
    if (S22 == 2) {
        if (!strcmp(W, "MS")) { repl(OUT, "MS", "M-S", 60); goto out; }
        if (!strcmp(W, "OS")) { repl(OUT, "OS", "O-S", 60); goto out; }
        if (!strcmp(W, "IP") && i - 1 >= 1 && !strcmp(t->word[i - 1], "TCP")) { repl(OUT, "IP", "I-P", 60); goto out; }
    }
    if (!strcmp(LC, "msdos")) {
        cpy(OUT, "M-S-", 60);
        cat(OUT, W + 2, 60);
        goto post;
    }

    /* NY..., ...NY */
    if (!strncmp(W, "NY", 2) || !strcmp(fe_tail(W, 2), "NY")) {
        F34[0] = 0;
        cpy(F34, "#", 60);
        cat(F34, W, 60);
        cat(F34, "#", 60);
        if (fe_strpos("#NYSE#", F34)) {
            spell(F, OUT, W);
            goto post;
        }
    }
    if (!strncmp(W, "NY", 2)) {
        if (!strcmp(W, "NY")) { src = "New York"; goto set_src; }
        if (!strcmp(W, "NYC")) { src = "New York City"; goto set_src; }
        if (strlen(W) == 3) {
            cpy(OUT, "N-Y-", 60);
            cat(OUT, W + 2, 60);
            goto post;
        }
        cpy(F34, OUT, 60);
        fe_strlwr(F34);
        for (si = 3; SL(OUT) >= si; si++) {
            F26[0] = 0;
            ncat(F26, F34 + si - 1, 1, 2);
            if (fe_strpos("aeiouy", F26)) {
                cpy(OUT, "NY-", 60);
                cat(OUT, W + 2, 60);
                goto post;
            }
        }
        cpy(OUT, "N-Y", 60);
        S10 = 3;
        if (SL(W) >= 3)
            do {
                cat(OUT, "-", 60);
                ncat(OUT, W + S10 - 1, 1, 60);
                S10++;
            } while (SL(W) >= S10);
        goto post;
    }
    if (!strcmp(fe_tail(W, 2), "NY")) goto post;

    /* LA..., ...LA */
    if (!strncmp(W, "LA", 2) || !strcmp(fe_tail(W, 2), "LA")) {
        cpy(F34, "#", 60);
        cat(F34, W, 60);
        cat(F34, "#", 60);
        if (fe_strpos("#UCLA#", F34)) {
            spell(F, OUT, W);
            goto post;
        }
    }
    if (!strncmp(W, "LA", 2)) {
        if (strlen(W) == 2) { src = "L-A"; goto set_src; }
        if (strlen(W) == 3) goto post;
        cpy(F34, OUT, 60);
        fe_strlwr(F34);
        for (si = 3; SL(OUT) >= si; si++) {
            F26[0] = 0;
            ncat(F26, F34 + si - 1, 1, 2);
            if (fe_strpos("aeiouy", F26)) goto post;
        }
        cpy(OUT, "L-A", 60);
        S10 = 3;
        if (SL(W) >= 3)
            do {
                cat(OUT, "-", 60);
                ncat(OUT, W + S10 - 1, 1, 60);
                S10++;
            } while (SL(W) >= S10);
        goto post;
    }
    if (!strcmp(fe_tail(W, 2), "LA")) goto post;

    /* US..., UK..., UC.. */
    F34[0] = 0;
    if (SL(W) >= 3) ncat(F34, W + 2, 1, 60);
    if (!strncmp(W, "US", 2) && F34[0] != 'E' && F34[0] != 'H' && F34[0] != 'U' && strcmp(W, "USING")) {
        if (strlen(W) <= 4) {
            spell(F, OUT, W);
            goto post;
        }
        cpy(FAC, "U-S", 60);
        cpy(FE8, "", 60);
        cat(FE8, W + 2, 60);
        S24 = 1;
        /* The original leaves S18/S1A unset here: next to "dot" or "@" whatever the stack holds decides whether the
           parts are parsed (DLL: "USPTO dot gov" -> "U-S-PTO"). fe_fixes: "U-S" is spelled already, the rest is
           parsed ("U-S-P-T-O"). */
        if (fe_fixes) S18 = 1, S1A = 0;
        goto morphs;
    }
    if (!strcmp(fe_tail(W, 2), "US")) goto post;
    if (!strncmp(W, "UK", 2)) {
        if (strlen(W) <= 4) {
            spell(F, OUT, W);
            goto post;
        }
        cpy(FAC, "U-K", 60);
        cpy(FE8, "", 60);
        cat(FE8, W + 2, 60);
        S24 = 1;
        if (fe_fixes) S18 = 1, S1A = 0;
        goto morphs;
    }
    if (!strncmp(W, "UC", 2) && strlen(W) <= 4) {
        spell(F, OUT, W);
        goto post;
    }

    /* an all-capitals word: _EMPHASIS_ and *EMPHASIS* stay; so does one among upper-case neighbours ("DO NOT") */
    cpy(F34, W, 60);
    fe_strupr(F34);
    if (!strcmp(W, F34)) {
        if (!strncmp(LC, "_", 1) && !strcmp(fe_tail(LC, 1), "_")) goto out;
        if (!strncmp(LC, "*", 1) && !strcmp(fe_tail(LC, 1), "*")) goto out;
        S10 = S14 = S16 = S12 = 0;
        S10 = not_upper(t, F, i - 2, i - 2, i - 2 >= 1) ? 1 : S10;
        S16 = not_upper(t, F, i + 1, i, t->cur_word >= i + 1) ? 1 : S16;
        if (!S10 || !S16) {
            S14 = not_upper(t, F, i - 1, i - 1, i - 1 >= 1) ? 1 : S14;
            S12 = not_upper(t, F, i + 2, i + 1, t->cur_word >= i + 2) ? 1 : S12;
            if (!S14) goto out;
            if (!S12 && !S16) goto out;
        }
    }

    /* two letters ("AB", "AB's"): said as a word when it is one, else spelled */
    if (strlen(W) == 2 || (strlen(W) == 3 && !strcmp(W + 2, "s")) || (strlen(W) == 4 && !strcmp(W + 2, "'s"))) {
        cpy(F34, W, 60);
        fe_strlwr(F34);
        F70[0] = 0;
        cat(F70, "#", 60);
        cat(F70, F34, 60);
        cat(F70, "#", 60);
        if (fe_strpos(two, F70)) goto out;
        cpy(F34, "", 60);
        ncat(F34, W, 1, 60);
        cat(F34, "-", 60);
        cat(F34, W + strlen(W) - 1, 60);
        if (strlen(W) > 2) cat(F34, "'s", 60);
        goto set_f34;
    }
    /* three letters ending in S ("ABS", not "dos"): spelled */
    if (strlen(LC) == 3 && !strcmp(fe_tail(W, 1), "S") && strcmp(LC, "dos") && !fe_strpos(LC, ".")
        && !fe_strpos(LC, "'")) {
        cpy(OUT, "", 60);
        ncat(OUT, W, 1, 60);
        cat(OUT, "-", 60);
        ncat(OUT, W + 1, 1, 60);
        cat(OUT, "-", 60);
        cat(OUT, W + 2, 60);
        goto post;
    }

    /* split the word: a morph (StripMorphs), mixed case ("LabVIEW", "MSdos"), or digits ("MP3", "X25A") */
    cpy(FAC, W, 60);
    cpy(FE8, "", 60);
    S24 = S18 = S1A = 0;
morphs:
    if (!S24) {
        fe_StripMorphs_A(t, &S24, &S18, &S1A, FAC, FE8);
        if (!S24) fe_StripMorphs_B(t, &S24, &S18, &S1A, FAC, FE8);
    }
    F26[0] = 0;
    ncat(F26, W, 1, 2);
    fe_strupr(F26);
    F20[0] = 0;
    ncat(F20, W + 1, 1, 2);
    fe_strlwr(F20);
    S30 = 0;
    if (!S24 && !strncmp(W, F26, 1) && !strncmp(W + 1, F20, 1) && (signed char)F20[0] >= 'a'
        && (signed char)F20[0] <= 'z') {
        /* Capital + lower case, up to the next capital: "LabVIEW" -> "Lab" + "VIEW" */
        S10 = 3;
        if (SL(W) >= 3)
            do {
                char *p = W + S10 - 1;
                F26[0] = 0;
                ncat(F26, p, 1, 2);
                fe_strupr(F26);
                if (!strncmp(p, F26, 1) && strcmp(F26, ".") && strcmp(F26, "'")) {
                    S30 = 1;
                    break;
                }
                S10++;
            } while (SL(W) >= S10);
        if (S30) {
            cpy(FAC, "", 60);
            ncat(FAC, W, S10 - 1, 60);
            cpy(FE8, W + S10 - 1, 60);
            S24 = 1;
        }
    }
    F26[0] = 0;
    ncat(F26, W, 1, 2);
    fe_strupr(F26);
    F20[0] = 0;
    ncat(F20, W + strlen(W) - 1, 1, 2);
    fe_strlwr(F20);
    S30 = 0;
    if (S24) goto parts;
    if (!strncmp(W, F26, 1) && !strncmp(W + strlen(W) - 1, F20, 1)) {
        /* capitals, then lower case to the end: "MSdos" -> "MS" + "dos" */
        S10 = 2;
        if (SL(W) >= 2) {
            do {
                char *p = W + S10 - 1;
                F26[0] = 0;
                ncat(F26, p, 1, 2);
                fe_strupr(F26);
                if (strncmp(p, F26, 1) || (signed char)F26[0] < 'A' || (signed char)F26[0] > 'Z') {
                    S12 = S10;
                    break;
                }
                S10++;
            } while (SL(W) >= S10);
        }
        S10 = S12;
        if (SL(W) >= S12)
            do {
                char *p = W + S10 - 1;
                F26[0] = 0;
                ncat(F26, p, 1, 2);
                fe_strlwr(F26);
                if (strncmp(p, F26, 1) || (signed char)F26[0] < 'a' || (signed char)F26[0] > 'z') goto digits;
                S10++;
            } while (SL(W) >= S10);
        cpy(FAC, "", 60);
        S30 = 1;
        ncat(FAC, W, S12 - 1, 60);
        cpy(FE8, W + S12 - 1, 60);
        S24 = 1;
    }
digits:
    if (S24 || S30) goto parts;
    cpy(F26, W + strlen(W) - 1, 2);
    if ((signed char)F26[0] >= '0' && (signed char)F26[0] <= '9') {
        /* trailing digits: "X25" -> "X" + "25" */
        S30 = 1;
        FAC[0] = 0;
        S24 = 1;
        si = (short)(strlen(W) - 1);
        while (si >= 2) {
            const char *p = W + si - 1;
            if (strncmp(p, "0", 1) < 0 || strncmp(p, "9", 1) > 0) break;
            si--;
        }
        ncat(FAC, W, si, 60);
        cpy(FE8, W + si, 60);
        goto parts;
    }
    /* the first digit run: "A0B" -> "A0" + "B" */
    for (S10 = 1; SL(W) - 1 >= S10; S10++) {
        const char *p = W + S10 - 1;
        if (strncmp(p, "0", 1) >= 0 && strncmp(p, "9", 1) <= 0) {
            S30 = 1;
            S24 = 1;
            S12 = S10;
            break;
        }
    }
    if (!S30) goto parts;
    for (si = S12 + 1; SL(W) - 1 >= si; si++) {
        const char *p = W + si - 1;
        if (strncmp(p, "0", 1) < 0 || strncmp(p, "9", 1) > 0) {
            FAC[0] = 0;
            ncat(FAC, W, si - 1, 60);
            cpy(FE8, W + si - 1, 60);
            break;
        }
    }

parts:
    /* a long word, not in an address ("dot", "@" next to it): the two parts as they are */
    if (strlen(OUT) > 4 && !(i > 1 && !strcmp(t->word[i - 1], "dot"))
        && !(t->cur_word > i && (!strcmp(t->word[i + 1], "dot") || !strcmp(t->word[i + 1], "@")))) {
        cpy(OUT, FAC, 60);
        if (strcmp(FE8, "")) {
            cat(OUT, "-", 60);
            cat(OUT, FE8, 60);
        }
        if (strcmp(OUT, W)) goto post;
        goto out;
    }
    if (S30 && S24) {
        /* a part that is a morph ("net", "dos") is not parsed; cad, cam / io, use are */
        cpy(F34, FAC, 60);
        fe_strlwr(F34);
        cpy(F70, FE8, 60);
        fe_strlwr(F70);
        for (pos = 0; fe_acr_morphs_count - 1 >= pos; pos++) {
            const char *e = fe_acr_morphs[pos];
            if (!strcmp(F34, e) && strcmp(e, "cad") && strcmp(e, "cam")) S18 = 1;
            if (!strcmp(F70, e) && strcmp(e, "io") && strcmp(e, "use")) S1A = 1;
        }
    }
    cpy(F34, FAC, 60);
    if (!S18) fe_Acronym_SyllableParse(t, i, F34);
    repl(F34, "-.", ".", 60);
    cpy(OUT, F34, 60);
    if (SL(FE8) > 0) {
        cpy(F34, FE8, 60);
        if (!S1A) fe_Acronym_SyllableParse(t, i, F34);
        cat(OUT, "-", 60);
        repl(F34, "-.", ".", 60);
        cat(OUT, F34, 60);
    }
    if (!strcmp(OUT, W)) goto out;
    /* a two-letter piece "XY-" that is no word is spelled "X-Y-" */
    F34[0] = 0;
    S12 = 0;
    for (;;) {
        char *p = OUT + S12;
        S10 = fe_strpos(p, "-");
        if (!S10) break;
        if (S10 == 3) {
            F70[0] = 0;
            cat(F70, "#", 60);
            ncat(F70, p, 2, 60);
            fe_strlwr(F70);
            cat(F70, "#", 60);
            if (!fe_strpos(two, F70)) {
                ncat(F34, p, 1, 60);
                cat(F34, "-", 60);
                ncat(F34, p + 1, 2, 60);
                S12 += S10;
                continue;
            }
        }
        ncat(F34, p, S10, 60);
        S12 += S10;
    }
    cat(F34, OUT + S12, 60);
    repl(F34, "-.", ".", 60);
    goto set_f34;

set_src:
    cpy(OUT, src, 60);
    goto post;
set_f34:
    cpy(OUT, F34, 60);
post:
    /* "Y-M-C-A" -> "Y-M-C-Aye"; "IO-", "EDU-", "I-" said as words; the suffix back on */
    if (!strcmp(fe_tail(OUT, 2), "-A") || !strcmp(fe_tail(OUT, 2), "-a"))
        if (strcmp(F28, "d")) cat(OUT, "ye", 60);
    if (!strncmp(OUT, "IO-", 3)) {
        cpy(F34, "eye-oh-", 60);
        cat(F34, OUT + 3, 60);
        cpy(OUT, F34, 60);
    }
    if (!strncmp(OUT, "EDU-", 4)) {
        cpy(F34, "ed-yoo-", 60);
        cat(F34, OUT + 4, 60);
        cpy(OUT, F34, 60);
    }
    if (!strncmp(OUT, "I-", 2)) {
        cpy(F34, "eye-", 60);
        cat(F34, OUT + 2, 60);
        cpy(OUT, F34, 60);
    }
    cat(W, F28, 60);
    if (strcmp(F28, "'s") && F28[0] == 'd' && !strncmp(fe_tail(OUT, 2), "-", 1)) {
        /* "O-K" + "d" -> "O-Kayed" */
        F34[0] = 0;
        ncat(F34, OUT, (int)strlen(OUT) - 1, 60);
        F26[0] = 0;
        cat(F26, OUT + strlen(OUT) - 1, 2);
        fe_strlwr(F26);
        pos = fe_strpos("abcdefghijklmnopqrstuvwxyz", F26);
        if (pos > 0) {
            F70[0] = 0;
            ncat(F70, s_letter_ed + 5 * (pos - 1), 5, 60);
            cpy(F70, fe_str_trim_spaces(F70), 60);
            if (!strcmp(F70, "WWed")) cpy(F70, "double Yood", 60);
            cat(F34, F70, 60);
            cpy(OUT, F34, 60);
        }
    } else
        cat(OUT, F28, 60);
    F28[0] = 0;
    S1C = 1;
out:
    /* A word left as it is gets the mark as its suffix: "ANed" -> "ANd", "EDUed" -> "EDUd" [original]. With fe_fixes
       it gets back the "ed" that was taken off ("'s" for a plural "s" stays: "NASAs" -> "NASA's"). */
    {
        char mark[80];
        strcpy(mark, F28);
        if (fe_fixes && gone[0] && mark[0] && mark[strlen(mark) - 1] == 'd') {
            mark[strlen(mark) - 1] = 0;
            strcat(gone, mark);
            strcpy(mark, gone);
        }
        cat(OUT, mark, 60);
        cat(W, mark, 60);
    }
    cat(OUT, F19C, 60);
    cat(W, F19C, 60);
    cpy(F34, F160, 60);
    cat(F34, OUT, 60);
    cpy(OUT, F34, 60);
    cat(F160, W, 60);
    cpy(W, F160, 60);
    fe_str_replace_all(t->punc[i], "..", ". ", t->punc[i]);
    cat(OUT, t->punc[i], 60);
    *res = S1C;
}
