/* Roman numerals, and the sentence-end spacing the paragraph driver applies after every word (REFERENCE 3.8, 8.4). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

/* MSVC _itoa(v, s, 10) @0x10080c10 */
static char *itoa10(int v, char *s)
{
    sprintf(s, "%d", v);
    return s;
}

/* Replace word[i] with s + punc[i]. The original strcpy's into the old block, which is only strlen(word)+1 bytes
   ("X" -> "10", "VIII" -> "the 8th" run past it); with fe_fixes a longer result gets a new block. */
static void put_word(TextProc *t, short i, const char *s)
{
    if (fe_fixes && strlen(s) + strlen(t->punc[i]) > strlen(t->word[i])) {
        char *n = fe_malloc(strlen(s) + strlen(t->punc[i]) + 1);
        strcpy(n, s);
        strcat(n, t->punc[i]);
        fe_free(t->word[i]);
        t->word[i] = n;
        return;
    }
    strcpy(t->word[i], s);
    strcat(t->word[i], t->punc[i]);
}

/* "Chapter IV" -> "4", "Henry VIII" -> "the 8th": a word of the letters i, v and x (one case) is read as a Roman
   numeral. After a first name (or Clare/Claire) it becomes "the <ordinal>", after a word of the chapter/volume list
   (or in first position with ".", ")" or "/" after it) a cardinal with QUAL "roman". @0x10040760 */
void fe_Num_Roman(TextProc *t, short i)
{
    /* The locals mirror the original frame ([F+x] of the listing), so that its overruns hit the same neighbours: a
       word of 60+ characters runs from W into P and the word lists. */
    const unsigned char *fr = fe_frame_of(0x10040760);
    unsigned char F[0x3f0];
    char *W = (char *)F + 0x20, *P = (char *)F + 0x5c, *C0 = (char *)F + 0xc0, *roman = (char *)F + 0x98;
    char *an = (char *)F + 0xa4, *clare = (char *)F + 0xb0, *names1 = (char *)F + 0x11c,
         *names2 = (char *)F + 0x1fc, *chapter = (char *)F + 0x2dc;
    char ch[2];
    short prev = 0, bx = 0, si, v;
    int im1 = i - 1;
    const char *sfx;

    if (fr) memcpy(F, fr, sizeof F);
    else memset(F, 0, sizeof F);
    strcpy(roman, "i###v####x");
    strcpy(an, "#an#the#no#");
    strcpy(clare, "#clare#claire#");
    strcpy((char *)F + 0xfc, "#i#v#x#iv#xi#vi#xx#xxx#xl#il#");     /* unused */
    strcpy(names2, "#john#james#michael#peter#paul#paula#joseph#josephine#andrew#charles#anthony#francis#chris"
                   "#christopher#gabriel#luke#jerome#thomas#george#jon#jonathan#daniel#david#dominic#augustine"
                   "#lawrence#lou#louis#mark#patrick#ed#edward#");
    strcpy(names1, "#vincent#william#martin#nick#nicholas#matthew#henry#stephen#steven#valentine#helen#helena#mary"
                   "#catherine#ann#anne#laura#marie#maria#joan#sophia#monica#lucy#patricia#teresa#theresa#veronica"
                   "#barbara#elizabeth#beth#frances#");
    strcpy(chapter, "#ver#version#versions#system#chap#chapter#chapters#div#division#divisions#vol#volume#volumes"
                    "#fig#figure#figures#art#article#articles#sec#section#iss#issue#grp#group#chapt#num#number#art"
                    "#article#par#paragraph#lev#level#gr#grade#phr#phrase#seg#segment#pg#page#amend#amendment#");

    W[0] = 0;
    strncat(W, t->word[i], strlen(t->word[i]) - strlen(t->punc[i]));
    if (strlen(W) > 1) {
        /* all letters of one case: all >= 'a' or all below it */
        int lower = (unsigned char)W[0] >= 'a';
        for (si = 1; (short)strlen(W) - 1 >= si; si++)
            if (((unsigned char)W[si] >= 'a') != lower) return;
    }
    fe_strlwr(W);
    strcpy(C0, "#");                                                   /* built but never used */
    strcat(C0, W);
    strcat(C0, "#");

    /* value, right to left; a letter smaller than the one after it is subtracted. '#' counts 2 (its position in
       "i###v####x"). */
    for (si = (short)strlen(W); si >= 1; si--) {
        ch[0] = W[si - 1];
        ch[1] = 0;
        v = fe_strpos(roman, ch);
        if (v == 0) {
            bx = 0;
            break;
        }
        if (prev > v) bx -= v;
        else bx += v;
        prev = v;
    }
    if (bx == 0) return;

    if (im1 >= 1) {
        strcpy(P, "#");
        strncat(P, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
        fe_strlwr(P);
        strcat(P, "#");
    } else {
        P[0] = 0;
    }

    if (im1 >= 1) {
        if (bx <= 1) goto small;
        if (fe_strpos(names2, P) || fe_strpos(names1, P) || fe_strpos(clare, P)) {
            /* "Henry V" / "John X" are left alone before a capitalised word or at a sentence start */
            if (bx == 5 || bx == 10) {
                if (t->cur_word >= i + 1 && (t->iuc[i + 1] || t->auc[i + 1])) goto other;
                if (i < t->cur_word && t->fper[i]) goto other;
            }
            if (bx == 6 && !strcmp(t->punc[i - 1], ",")) return;
            itoa10(bx, W);
            if (bx >= 10 && bx <= 19) sfx = "th";
            else if (!strcmp(fe_tail(W, 1), "1")) sfx = "st";
            else if (!strcmp(fe_tail(W, 1), "2")) sfx = "nd";
            else if (!strcmp(fe_tail(W, 1), "3")) sfx = "rd";
            else sfx = "th";
            strcat(W, sfx);
            strcpy(P, "the ");
            strcat(P, W);
            put_word(t, i, P);
            strcpy(t->out_text, P);
            strcat(t->out_text, t->punc[i]);
            return;
        }
    }
other:
    if (bx <= 1 || bx == 10 || bx == 5 || bx == 30) goto small;
    if (bx == 4 && im1 >= 1 && fe_strpos(an, P)) return;              /* "an IV", "the IV", "no IV" */
    if (bx == 39) return;
    if ((bx == 20 || bx == 30) && i == 1 && t->cur_word == i) return;
    if (im1 >= 1) {
        /* the original lower-cases word[i-1] in place */
        if (fe_fixes) {
            char b[256] = "";
            strncat(b, t->word[i - 1], 255);
            if (!strncmp(fe_strlwr(b), "sigma", 5)) return;
        } else if (!strncmp(fe_strlwr(t->word[i - 1]), "sigma", 5)) {
            return;
        }
    }
    goto convert;

small:  /* 1, 5, 10, 30 */
    if (im1 >= 1) {
        if (bx == 1) return;
        if (!fe_strpos(chapter, P)) return;
        goto convert;
    }
    if (bx == 30) return;
    if (bx != 5) {
        if (!fe_strpos(t->punc[i], ".") && !fe_strpos(t->punc[i], ")") && !fe_strpos(t->sepr[i], "/")) {
            if (t->cur_word < i + 1) return;
            if (strcmp(t->word[i + 1], ")") && strcmp(t->word[i + 1], "/")) return;
        }
    }
convert:
    itoa10(bx, W);
    put_word(t, i, W);
    strcpy(t->out_text, W);
    strcat(t->out_text, t->punc[i]);
    if (strcmp(t->qual[i], "roman")) {
        strcpy(P, "roman");
        fe_free(t->qual[i]);
        t->qual[i] = fe_strdup(P);
    }
}

/* Called by the paragraph driver after every word (unless the word already set its own output): spacing for the
   sentence end. A word that ended in '.' (lc_period) before a capitalised word gets a space after its expansion,
   unless the period is part of "..", or the word was a title / city abbreviation, a direction abbreviation other
   than lat/long (also when the next word is one), or a single capital ("J."). A '?' or '!' after the word adds a
   space too, and the last word of the paragraph without punctuation gets ". " unless the expansion ends in "." or
   ". " already. @0x100403a0 */
void fe_Classify_PeriodContext(TextProc *t, short i)
{
    if (t->lc_period && t->cur_word >= i + 1 && t->iuc[i + 1]) {
        if (!fe_strpos(t->punc[i], "..")) {
            if (!strncmp(t->rule_type, "title", 5)) goto end;
            if (!strncmp(t->rule_type, "direction", 9) && strcmp(t->word_lc, "lat") && strcmp(t->word_lc, "long"))
                goto end;
            if (t->cur_word >= i + 1 && !strncmp(t->qual[i + 1], "direction", 9) && strcmp(t->word_lc, "lat") &&
                strcmp(t->word_lc, "long"))
                goto end;
            if (!strcmp(t->rule_type, "city")) goto end;
            if (t->auc[i] && strlen(t->word[i]) - strlen(t->punc[i]) == 1) goto end;
        }
        strcat(t->out_text, " ");
    }
end:
    if (fe_strpos(t->punc[i], "?")) strcat(t->out_text, " ");
    if (fe_strpos(t->punc[i], "!")) strcat(t->out_text, " ");
    /* an empty out_text makes the original look at the last bytes of word_lc */
    if (t->cur_word <= i && !strcmp(t->punc[i], "") && strcmp(fe_tail(t->out_text, 1), ".") &&
        strncmp(fe_tail(t->out_text, 2), ".", 1))
        strcat(t->out_text, ". ");
}
