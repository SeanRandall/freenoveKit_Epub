/* Number stage: numbers read digit by digit or as words (patent 142; REFERENCE 3.8, 8.4). */
#include <string.h>
#include <stdlib.h>
#include "frontend.h"
#ifdef _WIN32
#include <windows.h>
#endif

/* Work buffers: the original's are 60 bytes on the stack and overflow for longer words (the DLL then crashes); the
   port's are large enough for any word. */
#define WBUF 2048

/* A stale word slot past cur_word can hold any value (the DLL faults on it, e.g. NULL at 0x1003c52d); the port stops
   there instead. */
static int readable(const char *p)
{
#ifdef _WIN32
    return p && !IsBadReadPtr(p, 1);
#else
    return p != NULL;
#endif
}

/* 911, 411, 707-767, 80286/80386/80486 and "L-1011" read as words, into out_text (+ the word's punctuation).
   The number is looked up with strstr in one run-together list and the position picks the reading. @0x1003c6b0 */
void fe_Num_SpecialNumbers(TextProc *t, short i, short *res)
{
    static const char list[] = "8028680386804869114117077277377477577671011";
    static const struct { short pos; const char *num, *say; } sp[] = {
        {1, "80286", "eighty, two eighty six"}, {6, "80386", "eighty, three eighty six"},
        {11, "80486", "eighty, four eighty six"}, {16, "911", "nine one one"}, {19, "411", "four one one"},
        {22, "707", "seven oh seven"}, {25, "727", "seven twenty seven"}, {28, "737", "seven thirty seven"},
        {31, "747", "seven forty seven"}, {34, "757", "seven fifty seven"}, {37, "767", "seven sixty seven"},
    };
    char buf[WBUF];
    int n = i, k;
    short r = *res, p;
    size_t bl = strlen(t->word[n]) - strlen(t->punc[n]), wl = strlen(t->word[n]);

    if ((bl < wl ? bl : wl) >= sizeof buf) return;   /* longer than 5 anyway */
    buf[0] = 0;
    strncat(buf, t->word[n], bl);
    if (strlen(buf) < 3 || strlen(buf) > 5) return;
    p = fe_strpos(list, buf);
    if (p) {
        r = 1;
        for (k = 0; k < (int)(sizeof sp / sizeof sp[0]) && sp[k].pos != p; k++)
            ;
        if (k < (int)(sizeof sp / sizeof sp[0])) {
            /* A prefix of an entry ("802", "7077") stays accepted with out_text unchanged [original] */
            if (!strcmp(buf, sp[k].num)) strcpy(t->out_text, sp[k].say);
            else if (fe_fixes) r = 0;
        } else if (p == 40) {
            if (strcmp(buf, "1011")) {
                if (fe_fixes) r = 0;   /* "101", "1011"'s prefix: same as above */
            } else if (n - 1 >= 1 && !strcmp(t->word[n - 1], "L") && !strcmp(t->sepr[n - 1], "-")) {
                strcpy(t->out_text, "ten eleven");
            } else {
                r = 0;
            }
        } else {
            r = 0;
        }
    }
    if (r) strcat(t->out_text, t->punc[n]);
    *res = r;
}

/* A phone number without area code: 7 digits after tel/ph/fax/call (or "number"/"#" after one of them), or
   nnn.nnn.nnnn; read digit by digit into out_text, with a comma pause after the exchange, for each '.', and at the
   end. Trailing .,?!;: are dropped first. @0x1003aec0 */
void fe_Num_PhoneContext(TextProc *t, short i, short *res)
{
    char a[WBUF], b[WBUF], prev[WBUF], prev2[WBUF];
    int n = i;
    short r = *res, ok, k;

    strcpy(a, t->word[n]);
    strcpy(b, t->word[n]);
    k = 1;
    /* strips at most as many marks as are left after the count: "x.." -> "x" but "5..." -> "5." */
    while ((short)strlen(b) >= k && strchr(".,?!;:", b[strlen(b) - 1])) {
        a[0] = 0;
        strncat(a, b, strlen(b) - 1);
        k++;
        strcpy(b, a);
    }
    ok = 1;
    prev[0] = prev2[0] = 0;
    if (n - 1 >= 1) {
        strcpy(prev, t->word[n - 1]);
        if (n - 2 >= 1) strcpy(prev2, t->word[n - 2]);
    }
    fe_strlwr(prev);
    fe_strlwr(prev2);
    if (strlen(a) == 7 &&
        (!strncmp(prev, "tel", 3) || !strncmp(prev, "ph", 2) || !strcmp(prev, "fax") || !strcmp(prev, "call") ||
         ((!strncmp(prev2, "tel", 3) || !strncmp(prev2, "ph", 2) || !strcmp(prev2, "fax") || !strcmp(prev2, "call")) &&
          (!strcmp(prev, "number") || !strcmp(prev, "#"))))) {
        for (k = 1; k <= 7; k++)
            if (a[k - 1] < '0' || a[k - 1] > '9') { ok = 0; break; }
        if (ok) goto say;
    }
    ok = 0;
    if (strlen(a) != 12) goto done;
    for (k = 1; k <= 12; k++) {
        if (k == 4 || k == 8) { if (a[k - 1] != '.') goto done; }
        else if (a[k - 1] < '0' || a[k - 1] > '9') goto done;
    }
say:
    t->out_text[0] = 0;
    r = 1;
    for (k = 1; (short)strlen(a) >= k; k++) {
        if (a[k - 1] == 0) strcat(t->out_text, "oh");   /* never: meant for '0'? (the engine reads 0 anyway) */
        else if (a[k - 1] == '.') strcat(t->out_text, ",");
        else strncat(t->out_text, &a[k - 1], 1);
        if (ok && k == 3) strcat(t->out_text, ",");
        strcat(t->out_text, (short)strlen(a) == k ? "," : " ");
    }
done:
    *res = r;
}

/* Groups of digits read digit by digit ("oh" for 0), with ", " after each group: nnn-nn-nnnn (SSN), nnnn-nnn-nnn-nnn
   and nnnn-nnnn-nnnn-nnnn (4 groups), and 0n.. followed by up to 5 more number words (6 groups). The groups after the
   first are emptied and their NUM cleared. @0x1003c130 */
void fe_Num_DigitString(TextProc *t, short i, short *res)
{
    char tail[WBUF], g[WBUF];
    int n = i, k, c;
    short r, lead0 = 0, groups;

    if (t->cur_word < n + 2 || t->num[n + 2] == 0) return;
    r = *res;
    if (!strncmp(t->word[n], "0", 1) && strlen(t->word[n]) >= 2 && strlen(t->word[n]) <= 4 && strlen(t->word[n + 1]) >= 2) {
        lead0 = 1;
        groups = 6;
    } else {
        if (t->num[n + 2] == 0) goto done;
        if (n + 3 <= t->cur_word && t->num[n + 3]) {
            tail[0] = 0;
            strncat(tail, t->word[n + 3], strlen(t->word[n + 3]) - strlen(t->punc[n + 3]));
            if (strlen(t->word[n]) == 4 && strlen(t->word[n + 1]) == 3 && strlen(t->word[n + 2]) == 3 && strlen(tail) == 3) {
                groups = 4;
                goto say;
            }
            if (strlen(t->word[n]) == 4 && strlen(t->word[n + 1]) == 4 && strlen(t->word[n + 2]) == 4 && strlen(tail) == 4) {
                groups = 4;
                goto say;
            }
        }
        tail[0] = 0;
        strncat(tail, t->word[n + 2], strlen(t->word[n + 2]) - strlen(t->punc[n + 2]));
        if (strlen(t->word[n]) != 3 || strlen(t->word[n + 1]) != 2 || strlen(tail) != 4) goto done;
        groups = 3;
    }
say:
    r = 1;
    t->out_text[0] = 0;
    for (k = 1; groups >= k; k++) {
        int j = n + k - 1;
        /* The 6-group case tests NUM of words past cur_word and reads their stale word slots; the DLL crashes when
           that slot is NULL (0x1003c52d). */
        if (t->num[j] == 0 || (fe_fixes && j > t->cur_word)) goto done;
        if (j > t->cur_word && !readable(t->word[j])) goto done;   /* the original faults here */
        strcpy(g, t->word[j]);
        for (c = 1; (short)strlen(g) >= c; c++) {
            if (g[c - 1] == '0') strcat(t->out_text, "oh");
            else strncat(t->out_text, &g[c - 1], 1);
            /* 0nn nnnn: ", " also inside the second group after its second digit when it is longer than 3 */
            if ((short)strlen(g) == c || (lead0 && k == 2 && c == 2 && strlen(g) > 3)) strcat(t->out_text, ", ");
            else strcat(t->out_text, " ");
        }
        /* The stale slots past cur_word are shared with the DLL in the harness: they are left alone. */
        if (k > 1 && j <= t->cur_word) strcpy(t->word[j], "");
        t->num[j] = 0;
    }
done:
    *res = r;
}
