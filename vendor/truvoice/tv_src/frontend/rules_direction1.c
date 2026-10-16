/* direction1: the single letters N, S, E, W as compass points (REFERENCE 3.4a). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

static void bare_lc(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
    fe_strlwr(out);
}
static void hash_tag(char *out, const char *w) { strcpy(out, "#"); strcat(out, w); strcat(out, "#"); }
static int is_state_name(const char *tag)
{
    return fe_strpos(fe_state_names[0], tag) || fe_strpos(fe_state_names[1], tag) || fe_strpos(fe_state_names[2], tag);
}
static int ends_with(const char *w, const char *c) { return strcmp(fe_tail(w, 1), c) == 0; }

/* Accept N/S/E/W as a compass point:
   - with a house number before and a street word after ("12 N Main St"), or before an ordinal street;
   - after a coordinate ("40 deg 26' N");
   - after a house number at the start of a sentence, before a name ending in -ville, -burg, ... or a common
     street name;
   - before a state name, a capitalised state/province word, a city followed by a state or zip code, a place word
     (Valley, Heights, ...), after "by"/"x"/"to"/"or" between compass points;
   - before region names ("N America", "S Africa"); "N Territory" becomes "Northern", "E Europe" "Eastern",
     "W Australia" "Western".
   Written label for label after the original. @0x1006d490 */
void fe_Rule_Direction1(TextProc *t, short i, short *res)
{
    static const char streets[] = "#street#st#drive#dr#";
    static const char suffixes[] = "ville  burg   burgh  boro   boroughbury   town   port   field  meadow wood   ington ingham ";
    static const char street_names[] = "#central#main#washington#lincoln#jefferson#birch#maple#elm#oak#pine#palm#hill#";
    static const char place_words[] =
        "#valley#heights#mount#mountain#falls#city#village#town#township#fort#bay#harbor#ridge#peak#summit#spring#"
        "springs#bend#bluff#bluffs#island#junction#plain#plains#point#rapids#center#";
    static const char compass_all[] = "#n#e#s#w#ne#nw#se#sw#nne#nnw#sse#ssw#wnw#ene#wsw#ese#";
    static const char compass_2[] = "#nw#se#sw#nne#nnw#sse#ssw#wnw#ene#wsw#ese#";
    static const char regions_n[] = "#america#american#atlantic#carolina#dakota#korea#pacific#pole#vietnam#sea#";
    static const char regions_s[] =
        "#africa#african#america#american#carolina#china#dakota#georgia#korea#pacific#pole#vietnam#yemen#";
    static const char regions_e[] = "#china#german#germany#";
    static const char regions_w[] = "#german#germany#indies#irian#virginia#";
    short ok = *res;
    char n1[60] = "", n2[60] = "", n3[60] = "", t2[64], t3[64], num1[64], ord[8], tag[140], suf[16];
    const char *region_name;

    if (i + 1 <= t->cur_word) {
        bare_lc(t, i + 1, n1);
        if (i + 2 <= t->cur_word) {
            bare_lc(t, i + 2, n2);
            if (i + 3 <= t->cur_word) bare_lc(t, i + 3, n3);
        }
    }
    hash_tag(t2, n2);
    hash_tag(t3, n3);

    /* "12 N Main St" */
    if (i - 1 > 0 && t->num[i - 1]) {
        if (!(t->cur_word < i + 2)) {
            if (!strcmp(t->qual[i + 2], "street") || fe_strpos(streets, t2)) goto accept;
        }
        if (!(t->cur_word < i + 3)) {
            if (!strcmp(t->qual[i + 3], "street") || fe_strpos(streets, t3)) goto accept;
        }
    }
    /* "N 5th St" */
    if (i + 1 <= t->cur_word) {
        int bare = (int)strlen(t->word[i + 1]) - (int)strlen(t->punc[i + 1]);
        strcpy(ord, "");
        strncat(ord, t->word[i + 1] + bare - 2, 2);
        strcpy(num1, "");
        strncat(num1, t->word[i + 1], (size_t)(bare - 2));
        fe_strlwr(ord);
        if (strcmp(t->qual[i + 1], "ordnl") != 0) {
            if (!(strlen(t->word[i + 1]) > 2 && atoi(num1) != 0 &&
                  (!strcmp(ord, "st") || !strcmp(ord, "nd") || !strcmp(ord, "rd") || !strcmp(ord, "th"))))
                goto coord;
        }
        if (i - 1 < 1 || t->num[i - 1] == 0) {
            if (t->cur_word < i + 2 || (strcmp(t->qual[i + 2], "street") && !fe_strpos(streets, t2))) {
                if (strcmp(t->punc[i + 1], "") == 0 && t->cur_word - i != 1) goto coord;
            }
        }
        goto accept;
    }

coord:
    /* "40 deg 26' N" (no bounds checks in the original: it reads past cur_word) */
    for (short s = -6; s < -1; s++) {
        int j = i + s;
        if (!(j > 0 && t->num[j] && strncmp(t->word[j + 1], "deg", 3) == 0)) continue;
        if (s == -3 || s == -2) goto accept;
        if (t->num[j + 2]) {
            if (ends_with(t->word[j + 2], "'")) {
                if (s == -4 || s == -3) goto accept;
                if (t->num[j + 3] && ends_with(t->word[j + 3], "\"") && (s == -5 || s == -4)) goto accept;
            }
            if (t->num[j + 2] && strncmp(t->word[j + 3], "min", 3) == 0) {
                if (s == -5 || s == -4) goto accept;
                if (t->num[j + 4] && strncmp(t->word[j + 5], "sec", 3) == 0 && (s == -7 || s == -6)) goto accept;
            }
        }
    }

    if (t->fper[i] == 0) goto after_places;
    /* "12 N Springville", "12 N Main" at the start of a sentence */
    if (i - 1 > 0 && i + 1 <= t->cur_word && t->num[i - 1] && (t->iuc[i + 1] || t->auc[i + 1])) {
        if (t->stprov[i + 1] == 0) {
            for (int k = 1; k < 14; k++) {
                strcpy(suf, "");
                strncat(suf, suffixes + (k - 1) * 7, 7);
                strcpy(suf, fe_str_trim_spaces(suf));
                if (strlen(suf) < strlen(n1) && strcmp(n1 + (int)strlen(n1) - (int)strlen(suf), suf) == 0)
                    goto accept;
            }
            hash_tag(tag, n1);
            if (!fe_strpos(street_names, tag)) goto states;
        }
        goto accept;
    }

states:
    if (t->cur_word < i + 1) goto city;
    /* "N Dakota" style: before a state name */
    hash_tag(tag, n1);
    if (is_state_name(tag)) goto accept;
    if ((t->iuc[i + 1] || t->auc[i + 1]) && t->stprov[i + 1] && strlen(n1) > 2) goto accept;
    if (!(t->cur_word < i + 2)) {
        strcpy(tag, "#");
        strcat(tag, n1);
        strcat(tag, " ");
        strcat(tag, n2);
        strcat(tag, "#");
        if (is_state_name(tag)) goto accept;
    }

city:
    /* "N Dallas, TX", "N Dallas 75201" */
    if (i > 1 && i + 2 <= t->cur_word && strcmp(t->qual[i + 1], "city") == 0) {
        if (t->stprov[i + 2]) goto accept;
        if (t->num[i + 2] && (strlen(n2) == 5 || strlen(n2) == 9)) goto accept;
        if (strcmp(t->qual[i + 2], "city") == 0) goto accept;
    }
    if (t->auc[i] || t->iuc[i]) {
        /* "N Valley", "N Walnut Heights" */
        if (i + 1 <= t->cur_word) {
            hash_tag(tag, n1);
            if (fe_strpos(place_words, tag)) goto accept;
            if (i + 2 <= t->cur_word && (t->iuc[i + 1] || t->auc[i + 1]) && strcmp(t->punc[i + 1], "") == 0) {
                hash_tag(tag, n2);
                if (fe_strpos(place_words, tag)) goto accept;
            }
        }
after_places:
        /* "N by NE", "N to NW" */
        hash_tag(tag, n2);
        if ((!strcmp(n1, "by") || !strcmp(n1, "x")) && fe_strpos(compass_all, tag)) goto accept;
        if ((!strcmp(n1, "to") || !strcmp(n1, "or")) && fe_strpos(compass_2, tag)) goto accept;
    }

    /* regions */
    hash_tag(tag, n1);
    if (strcmp(t->word_lc, "n") == 0) {
        if (fe_strpos(regions_n, tag)) goto accept;
        if (!strcmp(n1, "terr") || !strcmp(n1, "territory")) { region_name = "Northern"; goto rename; }
    }
    if (strcmp(t->word_lc, "s") == 0 && fe_strpos(regions_s, tag)) goto accept;
    if (strcmp(t->word_lc, "e") == 0) {
        if (fe_strpos(regions_e, tag)) goto accept;
        if (!strcmp(n1, "europe") || !strcmp(n1, "european")) { region_name = "Eastern"; goto rename; }
    }
    if (strcmp(t->word_lc, "w") != 0) goto done;
    if (fe_strpos(regions_w, tag)) goto accept;
    if (!strcmp(n1, "australia") || !strcmp(n1, "europe") || !strcmp(n1, "european") || !strcmp(n1, "samoa")) {
        region_name = "Western";
        goto rename;
    }
    goto done;

rename:
    strcpy(t->rec_expansion, region_name);
accept:
    ok = 1;
done:
    *res = ok;
}
