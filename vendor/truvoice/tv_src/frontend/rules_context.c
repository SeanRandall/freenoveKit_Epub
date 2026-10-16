/* What happens after a rule accepted an expansion (REFERENCE 3.4): the punctuation after the abbreviation. */
#include <string.h>
#include "frontend.h"

/* Carry the abbreviation's punctuation into out_text. Its own period is dropped in mid-sentence ("Dr. Smith" ->
   "Doctor Smith"); at the end of a sentence the expansion gets ". " (or " " after a real period). The sentence
   is taken to go on when lc_period is set and the next word is capitalised, except after titles other than
   street/drive/foot, directions other than lat/long, and cities. @0x10047410 */
void fe_Rule_Context_StreetTitleCity(TextProc *t, short i, short accepted)
{
    char buf[64], lw[60];
    char *p;
    short d;
    int end;

    /* the punctuation may sit on empty words after this one */
    if (i + 1 <= t->cur_word && strlen(t->punc[i]) == 0) {
        for (short j = i + 1; j <= t->cur_word; j++, i++)
            if (strcmp(t->word[j], "") != 0 || strlen(t->punc[j]) == 0) break;
    }
    end = t->cur_word <= i;
    if (t->lc_period == 0) goto punct;
    if (i < t->cur_word) {
        if (t->iuc[i + 1] == 0 && t->auc[i + 1] == 0) goto punct;
        if (strncmp(t->rule_type, "title", 5) == 0) {
            strcpy(lw, t->rec_expansion);
            fe_strlwr(lw);
            if (strcmp(lw, "street") != 0 && strcmp(lw, "drive") != 0 && strcmp(lw, "foot") != 0) goto punct;
        }
        if (strncmp(t->rule_type, "direction", 9) == 0 && strcmp(t->word_lc, "lat") != 0 &&
            strcmp(t->word_lc, "long") != 0)
            goto punct;
        if (i + 1 <= t->cur_word && strncmp(t->qual[i + 1], "direction", 9) == 0 && strcmp(t->word_lc, "lat") != 0 &&
            strcmp(t->word_lc, "long") != 0)
            goto punct;
        if (strcmp(t->rule_type, "city") == 0) goto punct;
    }
    end = 1;

punct:
    p = t->punc[i];
    if (strcmp(p, "") != 0) {
        d = fe_strpos(p, ".");
        if (d && accepted && !end) {
            /* drop the abbreviation's period (the original builds this in a 4-byte buffer) */
            strcpy(buf, "");
            strncat(buf, p, d - 1);
            if (d < (short)strlen(p)) strcat(buf, p + d);
            strcpy(p, buf);
        }
        strcat(t->out_text, p);
    }
    if (end) {
        if (!fe_strpos(p, ".")) strcat(t->out_text, ". ");
        if (fe_strpos(p, ".")) strcat(t->out_text, " ");
    }
    if (fe_strpos(p, "?")) strcat(t->out_text, " ");
    if (fe_strpos(p, "!")) strcat(t->out_text, " ");
}
