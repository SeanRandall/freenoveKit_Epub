/* Acronym stage: the known-word lists and respellings, morph stripping, military ratings (patent 150; REFERENCE 3.9,
   8.4). Acronym_KnownLists and StripMorphs_A/B are called by AcronymInitialismExpander 10013460; the paragraph
   driver TextExpander_Paragraph 1002b8d0 calls Acronym_MilitaryRating (with *res = 0) before the expander. */
#include <stdlib.h>
#include <string.h>
#include "fe_data.h"
#include "frontend.h"

/* ---- Acronym_KnownLists --------------------------------------------------------------------------------------- */

/* "abc" -> "a-b-c" */
static void spell(char *d, const char *s)
{
    short k, n = (short)strlen(s);

    for (k = 1; n > k; k++) {
        strncat(d, s + k - 1, 1);
        strcat(d, "-");
    }
    strcat(d, s + strlen(s) - 1);
}

/* Known acronyms and initialisms. out_text holds the word as the expander has it; the lists are searched with
   "#word#" (lower case). The case of the word as typed decides which lists apply: ilc[i] (lists 0, 3, 5), all upper
   case as typed (list 1), either (lists 2, 4, 6):
     lists 0-2   spelled letter by letter           "cia" -> "c-i-a", "AT&T" -> "A-T-&-T"
     lists 3-4   the first letter split off         "JPEG" -> "J-PEG"
     lists 5-6   the last letter split off          "NETX" -> "NET-X", "POSSLQ" -> "POSSL-Q"
     list 7      (upper case) kept as a word        "SCSI", "AIDS"
     list 8      (upper case) prefixed with '~' ("ALL", "YES", ...) unless a word up to two before or after is all
                 upper case and not "I" or "A" (then kept as it is: emphasis)
   Otherwise the word, padded to 8 characters, is looked up in the fixed-width respelling table (ilc or upper case):
   "asap" -> "aye-sap", "msdos" -> "m-s-dos". *res = 1 when the word was found (out_text holds the result).
   @0x10016ae0 */
void fe_Acronym_KnownLists(TextProc *t, short i, short *res)
{
    char up[2100], key[2100], buf[2100];
    char *out = t->out_text;
    int upper, ilc = t->ilc[i] != 0;
    short p, k;
    int j;

    *res = 0;
    strcpy(up, t->word_as_typed);
    fe_strupr(up);
    upper = strcmp(t->word_as_typed, up) == 0;
    strcpy(key, "#");
    strcat(key, out);
    fe_strlwr(key);
    strcat(key, "#");
    buf[0] = 0;

    if ((ilc && fe_strpos(fe_acr_known[0], key)) || (upper && fe_strpos(fe_acr_known[1], key)) ||
        ((ilc || upper) && fe_strpos(fe_acr_known[2], key))) {
        spell(buf, out);
    } else if ((ilc && fe_strpos(fe_acr_known[3], key)) || ((ilc || upper) && fe_strpos(fe_acr_known[4], key))) {
        strncat(buf, out, 1);
        strcat(buf, "-");
        strcat(buf, out + 1);
    } else if ((ilc && fe_strpos(fe_acr_known[5], key)) || ((ilc || upper) && fe_strpos(fe_acr_known[6], key))) {
        strncat(buf, out, strlen(out) - 1);
        strcat(buf, "-");
        strcat(buf, out + strlen(out) - 1);
    } else if (upper && fe_strpos(fe_acr_known[7], key)) {
        *res = 1;
        return;
    } else if (upper && fe_strpos(fe_acr_known[8], key)) {
        for (k = 1; k <= 2; k++) {
            j = i - k;
            if (j >= 1 && t->auc[j] && strcmp(t->word[j], "I") && strcmp(t->word[j], "A")) {
                *res = 1;
                return;
            }
        }
        for (k = 1; k <= 2; k++) {
            j = i + k;
            if (t->cur_word >= j && t->auc[j] && strcmp(t->word[j], "I") && strcmp(t->word[j], "A")) {
                *res = 1;
                return;
            }
        }
        strcat(buf, "~");
        strcat(buf, out);
    } else {
        /* strncat with a negative count (a word longer than 8) appends all 8 blanks */
        int pad = 8 - (int)strlen(out);
        strcpy(buf, out);
        strncat(buf, "        ", pad < 0 ? 8 : pad);
        fe_strlwr(buf);
        p = fe_strpos(fe_acr_respell_keys, buf);
        /* The key is found anywhere, not only at the start of an 8-character record: an 8-letter word inside
           "calleridmsdos" ("ridmsdos") takes the record the match starts in. Fixed: records only. */
        if (fe_fixes && p && (p - 1) % 8) p = 0;
        if (!(ilc || upper) || p == 0) return;
        buf[0] = 0;
        strncat(buf, fe_acr_respell + (p / 8) * 10, 10);
        strcpy(out, fe_str_trim_spaces(buf));
        *res = 1;
        return;
    }
    strcpy(out, buf);
    *res = 1;
}

/* ---- StripMorphs_A / StripMorphs_B ---------------------------------------------------------------------------- */

/* Morph stripping from the front: when word_as_typed (compared in lower case) starts with a morph of the table
   ("dos", "net", "pc", ...) and does not start with an exception ("dose", "inform", ...) or "cad"/"cam", head =
   the morph as typed, tail = the rest, *split = 1, *front = 1, and *tail_morph = 1 when the rest is itself a
   morph. A word starting with "os" or "io" is then (also) split after 2 characters, which sets *split and *front.
   Used for "MSDOS" -> "m-s-dos" style splits by AcronymInitialismExpander. @0x1001d030 */
void fe_StripMorphs_A(TextProc *t, short *split, short *front, short *tail_morph, char *head, char *tail)
{
    char w[2100], lc[2100];
    short a = *split, b = *front, c = *tail_morph, k, ml;

    strcpy(w, t->word_as_typed);
    fe_strlwr(w);
    if (strncmp(w, "cad", 3) == 0 || strncmp(w, "cam", 3) == 0) goto done;
    for (k = 0; k <= fe_acr_morph_exc_count - 1; k++)
        if (strlen(fe_acr_morph_exc[k]) <= strlen(w) &&
            strncmp(w, fe_acr_morph_exc[k], strlen(fe_acr_morph_exc[k])) == 0)
            goto done;
    for (k = 0; k <= fe_acr_morphs_count - 1; k++) {
        ml = (short)strlen(fe_acr_morphs[k]);
        if ((short)strlen(w) >= ml && strncmp(w, fe_acr_morphs[k], ml) == 0) {
            head[0] = 0;
            strncat(head, t->word_as_typed, ml);
            strcpy(tail, t->word_as_typed + ml);
            a = 1;
            b = 1;
            for (k = 0; k <= fe_acr_morphs_count - 1; k++) {
                strcpy(lc, tail);
                fe_strlwr(lc);
                if (strcmp(lc, fe_acr_morphs[k]) == 0) {
                    c = 1;
                    break;
                }
            }
            break;
        }
    }
    if (strncmp(w, "os", 2) == 0 || strncmp(w, "io", 2) == 0) {
        head[0] = 0;
        strncat(head, t->word_as_typed, 2);
        strcpy(tail, t->word_as_typed + 2);
        a = 1;
        b = 1;
    }
done:
    *split = a;
    *front = b;
    *tail_morph = c;
}

/* Morph stripping from the back: when word_as_typed (compared in lower case) is longer than a morph of the table and
   ends in it, and does not end in an exception ("magnet", "cabinet", "business", ...), "io" or "use", head = the
   word before the morph, tail = the morph (as typed), *split = 1 and *tail_morph = 1 ("PCNET" -> "PC" + "NET").
   *front is left as it is. @0x1001d410 */
void fe_StripMorphs_B(TextProc *t, short *split, short *front, short *tail_morph, char *head, char *tail)
{
    /* the frame: [F+0x1c] holds `this`, the lower-case copy is at [F+0x20]; the "io"/"use" tests of a shorter word
       read the bytes in front of it (fe_fixes: they fail) */
    unsigned char fr[4 + 2100];
    char *w = (char *)fr + 4;
    const void *self = fe_frame_this ? fe_frame_this : (const void *)t;
    short a = *split, c = *tail_morph, k, ml;
    size_t n, wl = strlen(t->word_as_typed);

    (void)front;
    memcpy(fr, &self, 4);
    strcpy(w, t->word_as_typed);
    fe_strlwr(w);
    n = strlen(w);
    if (fe_fixes ? n >= 2 && strncmp(w + n - 2, "io", 2) == 0 : strncmp(w + n - 2, "io", 2) == 0) goto done;
    if (fe_fixes ? n >= 3 && strncmp(w + n - 3, "use", 3) == 0 : strncmp(w + n - 3, "use", 3) == 0) goto done;
    for (k = 0; k <= fe_acr_morph_exc_count - 1; k++)
        if (strlen(fe_acr_morph_exc[k]) <= n && strcmp(w + n - strlen(fe_acr_morph_exc[k]), fe_acr_morph_exc[k]) == 0)
            goto done;
    for (k = 0; k <= fe_acr_morphs_count - 1; k++) {
        ml = (short)strlen(fe_acr_morphs[k]);
        if ((short)n > ml && strcmp(w + n - ml, fe_acr_morphs[k]) == 0) {
            head[0] = 0;
            strncat(head, t->word_as_typed, wl - ml);
            strcpy(tail, t->word_as_typed + wl - ml);
            a = 1;
            c = 1;
            break;
        }
    }
done:
    *split = a;
    *tail_morph = c;
}

/* ---- Acronym_MilitaryRating ----------------------------------------------------------------------------------- */

/* Frame of the original (0xa0 + 4 pushes): [F+0x10] result, [F+0x12] the grade digit (6 bytes), [F+0x18] the class
   word (12), [F+0x24] the rating (12), [F+0x30] the word (60), [F+0x6c] "asert", [F+0x74] a work buffer (60). The
   array is larger than the frame so that long words do not run out of it (the original overwrites its return
   address when the word, "/" and the next word pass 0x80 characters). */
#define MIL_FRAME 0x2000

/* the inlined strcat: the length of s is taken first, then it is copied forwards (s may lie after d's end) */
static void cat_fwd(char *d, const char *s)
{
    memmove(d + strlen(d), s, strlen(s) + 1);
}

/* strcmp(s, "1") < 0 || strcmp(s, "9") > 0 */
static int not_digit(const char *s) { return strcmp(s, "1") < 0 || strcmp(s, "9") > 0; }

/* the last character is c, or the last two are "cl" */
static int class_tail(const char *w)
{
    size_t n = strlen(w);
    if (fe_fixes && n < 2) return n == 1 && strcmp(w, "c") == 0;
    return strcmp(w + n - 1, "c") == 0 || strcmp(w + n - 2, "cl") == 0;
}

/* Military ratings, for a word typed with an initial capital and followed by a capitalised word (a name), directly
   or after "/" and one more word:
     A<d>c, A<d>cl, A/<d>c         -> "Airman <d>th Class"        (the digit 1-9 after the letter, or after '/')
     Sp<d>, Spec<d>, S<d>, Sp/<d>  -> "Specialist <d>",  "... <d>th Class" if it ends in c / cl
     En<d>c                        -> "Engineman <d>th Class"
     Rdm<d>c                       -> "Radarman <d>th Class"
     T<d>g                         -> "Technician <d>th Grade"
   ("A1C Smith" -> "Airman 1st Class", "SP/4 Jones" -> "Specialist 4" with the "4" word emptied.) The result goes to
   out_text and accepted_text, *res = 1. The lower-case word is word_lc; with "/" as the separator, the next word
   (lower case, without its punctuation) is appended after a "/", and it is emptied when the rating is accepted.
   @0x100502f0 */
void fe_Acronym_MilitaryRating(TextProc *t, short i, short *res)
{
    const unsigned char *fr = fe_frame_of(0x100502f0);
    static unsigned char F[MIL_FRAME];
    char *d12 = (char *)F + 0x12, *cls = (char *)F + 0x18, *rate = (char *)F + 0x24, *w = (char *)F + 0x30,
         *b74 = (char *)F + 0x74;
    short r = *res, bp, off;
    int hit = 0, ii = i;

    memset(F, 0, sizeof F);
    if (fr) memcpy(F, fr, 0xb0);
    memcpy(F + 0x6c, "asert", 6);

    b74[0] = 0;
    strncat(b74, t->word_lc, 1);
    bp = fe_strpos((char *)F + 0x6c, b74);
    if (fe_fixes && b74[0] == 0) bp = 0;     /* an empty word_lc is "found" at 1 and taken for 'a' */
    if (bp == 0) goto out;
    if (strncmp(t->word_as_typed, t->word_lc, 1) == 0) goto out;
    if (!(t->cur_word >= ii + 1 && t->iuc[ii + 1])) {
        if (strcmp(t->sepr[ii], "/") || !(t->cur_word >= ii + 2 && t->iuc[ii + 2])) goto out;
    }
    strcpy(w, t->word_lc);
    if (strcmp(t->sepr[ii], "/") == 0) {
        strcat(w, "/");
        b74[0] = 0;
        strncat(b74, t->word[ii + 1], strlen(t->word[ii + 1]) - strlen(t->punc[ii + 1]));
        fe_strlwr(b74);
        cat_fwd(w, b74);
    }
    if (fe_fixes) {
        /* the original reads the digit and the tail out of whatever is on its stack past a short word */
        size_t n = strlen(w);
        memset(w + n, 0, 8);
    }

    switch (bp) {
    case 1:     /* Airman */
        off = strncmp(w + 1, "/", 1) == 0 ? 2 : 1;
        d12[0] = 0;
        strncat(d12, w + off, 1);
        if (not_digit(d12) || !class_tail(w)) break;
        r = hit = 1;
        strcpy(rate, "Airman");
        strcpy(cls, "Class");
        break;
    case 2:     /* Specialist */
        off = strncmp(w, "spec", 4) == 0 ? 4 : strncmp(w, "sp", 2) == 0 ? 2 : 1;
        if (strncmp(w + off, "/", 1) == 0) off++;
        d12[0] = 0;
        strncat(d12, w + off, 1);
        if (not_digit(d12)) break;
        r = hit = 1;
        strcpy(rate, "Specialist");
        strcpy(cls, class_tail(w) ? "Class" : "");
        break;
    case 3:     /* Engineman */
        if (strncmp(w, "en", 2)) break;
        off = strncmp(w + 2, "/", 1) == 0 ? 3 : 2;
        d12[0] = 0;
        strncat(d12, w + off, 1);
        if (not_digit(d12) || !class_tail(w)) break;
        r = hit = 1;
        strcpy(rate, "Engineman");
        strcpy(cls, "Class");
        break;
    case 4:     /* Radarman */
        if (strncmp(w, "rdm", 3)) break;
        off = strncmp(w + 3, "/", 1) == 0 ? 4 : 3;
        d12[0] = 0;
        strncat(d12, w + off, 1);
        if (not_digit(d12) || !class_tail(w)) break;
        r = hit = 1;
        strcpy(rate, "Radarman");
        strcpy(cls, "Class");
        break;
    case 5:     /* Technician */
        off = strncmp(w + 1, "/", 1) == 0 ? 2 : 1;
        d12[0] = 0;
        strncat(d12, w + off, 1);
        if (not_digit(d12)) break;
        if (fe_fixes ? strlen(w) < 1 || strcmp(w + strlen(w) - 1, "g") : strcmp(w + strlen(w) - 1, "g")) break;
        r = hit = 1;
        strcpy(rate, "Technician");
        strcpy(cls, "Grade");
        break;
    }
    /* With *res already set on entry, the original builds the text from the stack buffers even when no rating was
       recognised (the paragraph driver never calls it so). Fixed: only for a recognised rating. */
    if (r == 0 || (fe_fixes && !hit)) goto out;
    strcpy(t->out_text, rate);
    strcat(t->out_text, " ");
    if (strcmp(cls, "")) {
        int n = atoi(d12);
        if (fe_fixes || (n >= 0 && n <= 9))
            strncat(t->out_text, (const char *)fe_mil_ordinals + 3 * n, 3);
        strcat(t->out_text, " ");
        strcat(t->out_text, cls);
    } else
        strcat(t->out_text, d12);
    strcpy(t->accepted_text, t->out_text);
    if (strcmp(t->sepr[ii], "/") == 0) strcpy(t->word[ii + 1], "");
out:
    *res = r;
}
