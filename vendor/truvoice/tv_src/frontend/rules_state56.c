/* state5-6: Canadian provinces (REFERENCE 3.4a); State_Helper_A is shared with state1-3. */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

static void bare(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
}
static void hash_tag(char *out, const char *w) { strcpy(out, "#"); strcat(out, w); strcat(out, "#"); }
static int is_conj3(const char *w) { return !strcmp(w, "and") || !strcmp(w, "&") || !strcmp(w, "or"); }
static int cap(const TextProc *t, int j) { return t->iuc[j] || t->auc[j]; }

/* The word before is a city of the state in rec_expansion ("Dallas TX"), also a two-word city. @0x10069bd0 */
void fe_State_Helper_A(TextProc *t, short i, short *res)
{
    char pw[64], pw2[140], st[64], buf[64];
    short start, end;

    if (i - 1 > 0) {
        bare(t, i - 1, pw);
        fe_strlwr(pw);
        if (!strcmp(fe_tail(pw, 1), ",")) {
            strcpy(buf, "");
            strncat(buf, pw, strlen(pw) - 1);
            strcpy(pw, buf);
        }
        fe_str_replace_all(pw, ".", " ", pw);
        fe_str_replace_all(pw, "  ", " ", pw);
        strcpy(pw, fe_str_trim_spaces(pw));
        strcpy(t->u_2bd5, t->rec_expansion);
        fe_CityState_LetterRange(t, &start, &end);
        strcpy(st, t->rec_expansion);
        fe_strlwr(st);
        strcpy(pw2, "");
        if (i - 2 > 0) {
            bare(t, i - 2, pw2);
            fe_strlwr(pw2);
            fe_str_replace_all(pw2, ".", "", pw2);
            strcat(pw2, " ");
            strcat(pw2, pw);
        }
        for (short k = start; k <= end; k++) {
            if (strcmp(fe_city_state[k].a, st) != 0) continue;
            if (!strcmp(pw, fe_city_state[k].b)) goto yes;
            if (fe_strpos(fe_city_state[k].b, " ") && i > 2 && !strcmp(pw2, fe_city_state[k].b)) goto yes;
        }
    }
    *res = 0;
    return;
yes:
    *res = 1;
}

/* the last n characters of w when it is that long (the original reads stack bytes in front of shorter words,
   which do not match in practice) */
static int ends(const char *w, const char *s)
{
    size_t n = strlen(w), m = strlen(s);
    return n >= m && !strcmp(w + n - m, s);
}

/* Several state/province words in a row are a list of places only if they are all different and not all in
   capitals, and the text does not contain two of the common-word abbreviations. */
static int list_ok(const char *s)
{
    static const char *const common[] = {"al", "cal", "ida", "del", "mo", "tex", "miss", "ms", "penn", "pa", "ma", "col", "md"};
    int cnt = 0;
    if (fe_strpos(s, "ok") || fe_strpos(s, "oh") || fe_strpos(s, "hi") || fe_strpos(s, "me")) return 0;
    for (int k = 0; k < 13; k++)
        if (fe_strpos(s, common[k])) cnt++;
    return cnt < 2;
}

/* @0x1006a090 */
void fe_Rule_State56(TextProc *t, short i, short *res)
{
    static const char saints[] = "#st#saint#ste#sainte#sault#mont#val#grande#north#south#east#west#new#";
    static const char places1[] =
        "#valley#hts#heights#falls#city#village#town#township#beach#pk#park#bay#harbor#ridge#peak#summit#shore#spring#"
        "springs#hill#hills#bend#bluff#bluffs#";
    static const char places2[] = "#island#jct#junction#plain#plains#pt#point#rapids#ctr#center#cyn#canyon#";
    static const char places3[] = "#mt#mount#mtn#mountain#ft#fort#lake#lk#port#";
    static const char provinces[] = "#AB#Alta#B.C#Man#MB#N.B#NB#Nfld#N.S#NS#Ont#ON#P.E.I#PEI#PE#Que#P.Q#PQ#Sask#SK#N.W.T#NWT#Y.T#YT#";
    short ok = *res, r;
    const char *w = t->word_lc;
    char tag[160], s[300];
    char n1[64], n2[64], n3[64], p1[64], p2[64], p3[64];
    char *slot[7] = {p3, p2, p1, NULL, n1, n2, n3};

    strcpy(tag, "#");
    strncat(tag, t->word[i], strlen(t->word[i]) - strlen(t->punc[i]));
    strcat(tag, "#");
    if (!fe_strpos(provinces, tag)) goto done;
    if (t->auc[i] && (!strcmp(w, "n.b") || !strcmp(w, "nb"))) {
        /* NB: "nota bene" at the start of a sentence */
        if (i == 1) goto done;
        if (i - 1 > 0 && (!strcmp(t->punc[i - 1], ".") || !strcmp(t->punc[i - 1], "!") || !strcmp(t->punc[i - 1], "?")))
            goto done;
    }
    for (short k = -3; k < 4; k++) {
        char tmp[64];
        int j = i + k;
        if (j < 1 || t->cur_word < j) {
            strcpy(tmp, "Z!@Z");
        } else {
            bare(t, j, tmp);
        }
        fe_strlwr(tmp);
        if (slot[k + 3]) strcpy(slot[k + 3], tmp);
    }

    /* "Ont prov", "Que Canada" */
    if (!(t->cur_word < i + 1)) {
        if (!strcmp(n1, "prov")) goto accept;
        if (t->iuc[i + 1] && (!strcmp(n1, "can") || !strcmp(n1, "canada"))) goto accept;
        if (t->auc[i + 1] && (!strcmp(n1, "canada") || !strcmp(n1, "cdn") || !strcmp(n1, "cda"))) goto accept;
    }
    /* a postal code "K1A 0B1" */
    if (i + 2 <= t->cur_word && strlen(n1) == 3 && strlen(n2) == 3 && t->auc[i + 1] && t->auc[i + 2] &&
        strncmp(n1, "a", 1) >= 0 && strncmp(n1, "z", 1) <= 0 && strncmp(n1 + 1, "0", 1) >= 0 &&
        strncmp(n1 + 1, "9", 1) <= 0 && strncmp(n1 + 2, "a", 1) >= 0 && strncmp(n1 + 2, "z", 1) <= 0 &&
        strncmp(n2, "0", 1) >= 0 && strncmp(n2, "9", 1) <= 0 && strncmp(n2 + 1, "a", 1) >= 0 &&
        strncmp(n2 + 1, "z", 1) <= 0 && strncmp(n2 + 2, "0", 1) >= 0 && strncmp(n2 + 2, "9", 1) <= 0)
        goto accept;
    r = 0;
    fe_State_Helper_A(t, i, &r);
    if (r) goto accept;

    if (!(t->cur_word < i + 2 || t->stprov[i + 1] == 0)) {
        /* this and the next words are all states: "Ont Que Man" */
        if (t->stprov[i + 2] == 0) {
            if (!(i + 3 <= t->cur_word && is_conj3(n2) && t->stprov[i + 3])) goto cef;
        }
        strcpy(s, w); strcat(s, " "); strcat(s, n1); strcat(s, " "); strcat(s, n2); strcat(s, " "); strcat(s, n3);
        if (!strcmp(w, n1) || !strcmp(w, n2) || !strcmp(w, n3) || !strcmp(n1, n2) || !strcmp(n1, n3) || !strcmp(n2, n3))
            goto cef;
        if (t->auc[i] && t->auc[i + 1] && (t->stprov[i + 2] == 0 || t->auc[i + 2]) &&
            (t->cur_word < i + 3 || t->stprov[i + 3] == 0 || t->auc[i + 3]))
            goto cef;
        goto b4cd;
    }
cef:
    if (i - 1 > 0 && i + 2 <= t->cur_word && t->stprov[i - 1]) {
        if (t->stprov[i + 1] == 0 && !(is_conj3(n1) && t->stprov[i + 2])) goto b0d0;
        strcpy(s, p1); strcat(s, " "); strcat(s, w); strcat(s, " "); strcat(s, n1); strcat(s, " "); strcat(s, n2);
        if (strcmp(p1, w) && strcmp(p1, n1) && strcmp(p1, n2) && strcmp(w, n1) && strcmp(w, n2) && strcmp(n1, n2) &&
            (t->auc[i] == 0 || t->auc[i - 1] == 0 || (t->stprov[i + 1] && t->auc[i + 1] == 0) ||
             (t->stprov[i + 2] && t->auc[i + 2] == 0)))
            goto b4cd;
    }
b0d0:
    if (i - 2 > 0 && t->stprov[i - 2]) {
        if (t->stprov[i - 1] == 0) {
            if (!(i - 3 > 0 && is_conj3(p1) && t->stprov[i - 3])) goto b55a;
        }
        strcpy(s, w); strcat(s, " "); strcat(s, p1); strcat(s, " "); strcat(s, p2); strcat(s, " "); strcat(s, p3);
        if (strcmp(w, p1) && strcmp(w, p2) && strcmp(w, p3) && strcmp(p1, p2) && strcmp(p1, p3) && strcmp(p2, p3) &&
            (t->auc[i] == 0 || t->auc[i - 2] == 0 || (t->stprov[i - 1] && t->auc[i - 1] == 0) ||
             (i - 3 > 0 && t->stprov[i - 3] && t->auc[i - 3] == 0)))
            goto b4cd;
    }
    goto b55a;
b4cd:
    if (list_ok(s)) goto accept;

b55a:
    if (strlen(w) == 3 && t->auc[i] &&
        (!strcmp(w, "b.c") || !strcmp(w, "y.t") || !strcmp(w, "n.s") || !strcmp(w, "p.q") || !strcmp(w, "n.b")))
        goto done;
    if (i - 1 > 0) {
        /* "Springfield Ont", "Hamilton, Ont" */
        if (cap(t, i - 1) && (!strcmp(t->punc[i - 1], "") || !strcmp(t->punc[i - 1], ","))) {
            if (ends(p1, "burg") || ends(p1, "boro") || ends(p1, "bury") || ends(p1, "town") || ends(p1, "port") ||
                ends(p1, "wood") || ends(p1, "ville") || ends(p1, "burgh") || ends(p1, "field") || ends(p1, "meadow") ||
                ends(p1, "ington") || ends(p1, "ingham") || ends(p1, "borough"))
                goto accept;
        }
        if (t->iuc[i - 1]) {
            hash_tag(tag, p1);
            if (fe_strpos(places1, tag) || fe_strpos(places2, tag) || fe_strpos(places3, tag)) goto accept;
        }
    }
    /* "St Jean Que", "Sault Ste Marie Ont" */
    hash_tag(tag, p1);
    if (i - 1 >= 1 && cap(t, i - 1) && fe_strpos(saints, tag)) goto accept;
    hash_tag(tag, p2);
    if (i - 2 >= 1 && cap(t, i - 1) && cap(t, i - 2) && fe_strpos(saints, tag)) goto accept;
    hash_tag(tag, p3);
    if (i - 3 >= 1 && cap(t, i - 1) && cap(t, i - 2) && cap(t, i - 3) && fe_strpos(saints, tag)) goto accept;
    goto done;

accept:
    ok = 1;
done:
    *res = ok;
}
