/* The compass / latitude-longitude rules (REFERENCE 3.4a). */
#include <string.h>
#include "frontend.h"

static int is_nsew(const char *w) { return !strcmp(w, "N") || !strcmp(w, "S") || !strcmp(w, "E") || !strcmp(w, "W"); }
/* the last character of a word (the original reads word[-1] for an empty one, as here unless fe_fixes) */
static int ends_with(const char *w, const char *c)
{
    return strcmp(fe_tail(w, 1), c) == 0;
}

/* direction: N/S/E/W and Lat/Long inside a coordinate, "40 deg 26' 46\" N", "40 deg 26 min N", "Lat 40 deg".
   @0x10057c30 */
void fe_Rule_Direction(TextProc *t, short i, short *res)
{
    short ok = *res, off = 0, s;
    char buf[60];

    /* how far back the coordinate may start */
    if (t->auc[i] && (!strcmp(t->word_lc, "n") || !strcmp(t->word_lc, "s") || !strcmp(t->word_lc, "e") ||
                      !strcmp(t->word_lc, "w")))
        off = -6;
    if (i - 1 > 0 && t->iuc[i] && (!strcmp(t->word_lc, "lat") || !strcmp(t->word_lc, "long"))) {
        strcpy(buf, "");
        strncat(buf, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
        if (is_nsew(buf)) off = -7;
    }

    /* backwards: "n deg" at i+s, followed by n' n", or n min [n sec] as far as reaches this word */
    for (s = off; s <= off + 4; s++) {
        int j = i + s;
        if (!(j > 0 && j + 1 <= t->cur_word && t->num[j] && strncmp(t->word[j + 1], "deg", 3) == 0)) continue;
        if (s == -3 || s == -2) goto found;
        if (!(t->cur_word < j + 2 || t->num[j + 2] == 0) && ends_with(t->word[j + 2], "'")) {
            if (s == -4 || s == -3) goto found;
            if (j + 3 <= t->cur_word && t->num[j + 3] && ends_with(t->word[j + 3], "\"") && (s == -5 || s == -4))
                goto found;
        }
        if (t->cur_word >= j + 3 && t->num[j + 2] && strncmp(t->word[j + 3], "min", 3) == 0 &&
            (s == -5 || s == -4 ||
             (t->cur_word >= j + 5 && t->num[j + 4] && strncmp(t->word[j + 5], "sec", 3) == 0 && (s == -7 || s == -6))))
            goto found;
    }

    /* forwards: "Lat 40 deg ... N" */
    if (t->iuc[i] == 0 || (strcmp(t->word_lc, "lat") != 0 && strcmp(t->word_lc, "long") != 0)) goto done;
    if ((t->num[i + 1] & (i + 2 <= t->cur_word)) == 0 || strncmp(t->word[i + 2], "deg", 3) != 0) goto done;
    if (!(t->cur_word < i + 3 || t->num[i + 3] == 0) && ends_with(t->word[i + 3], "'")) {
        s = (t->cur_word < i + 4 || t->num[i + 4] == 0 || !ends_with(t->word[i + 4], "\"")) ? 4 : 5;
    } else if (t->cur_word < i + 4 || t->num[i + 3] == 0 || strncmp(t->word[i + 4], "min", 3) != 0) {
        s = 3;
    } else if (t->cur_word < i + 6 || t->num[i + 5] == 0 || strncmp(t->word[i + 6], "sec", 3) != 0) {
        s = 5;
    } else {
        s = 7;
    }
    if (i + s < 1 || t->cur_word < i + s || !is_nsew(t->word[i + s])) goto done;
    goto accept;

found:
    /* (the original also has a branch for off == 1, which never happens) */
    if (off != -6 && off != -7) goto done;
accept:
    ok = 1;
done:
    *res = ok;
}
