/* title5: the ambiguous St / Dr / Ft (Saint or Street, Doctor or Drive, Fort or foot) and Sts / Drs / Fts
   (REFERENCE 3.4a, 3.4b). The record's expansion is the title (Saint, Doctor, Fort); "fix" switches it to the
   street / measure reading and relabels the word's QUAL. */
#include <string.h>
#include "frontend.h"
#include "fe_data.h"

static void bare(const TextProc *t, int j, char *out)
{
    strcpy(out, "");
    strncat(out, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
}
static void bare_lc(const TextProc *t, int j, char *out) { bare(t, j, out); fe_strlwr(out); }
static void hash_tag(char *out, const char *w) { strcpy(out, "#"); strcat(out, w); strcat(out, "#"); }
static int starts_and(const char *w) { return !strncmp(w, "and", 3) || !strncmp(w, "or", 2) || !strncmp(w, "&", 1); }
static int is_sdf(const char *w) { return !strcmp(w, "st") || !strcmp(w, "dr") || !strcmp(w, "ft"); }
/* Is s a city name? The original compares with the state column of the city table, from the second row, so it never
   matches; fe_fixes compares with the city column. */
static int is_city(const char *s)
{
    for (short k = 0; k <= fe_city_state_count - 2; k++)
        if (!strcmp(fe_fixes ? fe_city_state[k].b : fe_city_state[k + 1].a, s)) return 1;
    return fe_fixes && !strcmp(fe_city_state[fe_city_state_count - 1].b, s);
}

/* the last n characters (the original reads before the string when it is shorter; fe_fixes gives "" then) */
static const char *last(const char *w, int n) { return fe_tail(w, n); }
static int street_or_measure(const char *q) { return !strcmp(q, "street") || !strncmp(q, "measure", 7); }
static int any_stopword(const char *tag)
{
    for (int l = 0; l < (int)(sizeof fe_mod_stopwords / sizeof *fe_mod_stopwords); l++)
        if (fe_strpos(fe_mod_stopwords[l], tag)) return 1;
    return 0;
}

/* @0x1004d880 */
void fe_Rule_Title5(TextProc *t, short i, short *res)
{
    static const char puncs[] = "#.#,#!#?#:#;#-#&#";
    static const char dots[] = "#. #..#";
    static const char suffixes[] = "ville  burg   burgh  boro   boroughbury   town   port   field  meadow wood   ington ingham ";
    static const char street_names[] =
        "#central#main#washington#lincoln#jefferson#birch#maple#elm#oak#pine#palm#hill#wall#state#spring#broad#baker#"
        "bourbon#downing#high#park#ash#beech#forest#river#riverside#lake#lakeshore#";
    static const char saints_m[] =
        "#john#james#michael#peter#paul#paula#joseph#josephine#andrew#charles#anthony#francis#chris#christopher#gabriel#"
        "luke#jerome#thomas#george#jon#jonathan#daniel#david#dominic#augustine#lawrence#lou#louis#mark#patrick#ed#edward#";
    static const char saints_f[] =
        "#vincent#william#martin#nick#nicholas#matthew#henry#stephen#steven#valentine#helen#helena#mary#catherine#ann#"
        "anne#laura#marie#maria#joan#sophia#monica#lucy#patricia#teresa#theresa#veronica#barbara#elizabeth#beth#frances#";
    static const char saints_clare[] = "#clare#claire#";
    static const char surnames[] = "#smith#johnson#williams#jones#brown#miller#davis#anderson#wilson#thompson#";
    static const char apartment[] = "#apartment#apt#building#bldg#suite#ste#room#rm#condominium#condo#townhouse#th###box#drop#";
    static const char saint_places[] =
        "#louis#paul#augustine#charles#joseph#thomas#bernard#lucia#helena#helens#andrews#alban's#clair#petersburg#cloud#"
        "johns#peters#peter's#croix#";
    static const char forts[] =
        "#benning#bragg#collins#dix#dodge#hood#knox#laramie#lauderdale#leavenworth#lee#meade#myers#pierce#riley#sumter#"
        "wayne#worth#ticonderoga#apache#bend#madison#smith#thomas#mchenry#";
    static const char doctors[] = "#jekyll#king#kevorkian#pepper#salk#seuss#spock#";
    short ok = *res;
    const char *w = t->word_lc;
    char tag[140], buf[140], buf2[140];
    int j = i + 1, p = 0;

    /* famous ones: "Ft Worth", "St Louis", "Dr Seuss" */
    if (!(t->cur_word < j)) {
        strcpy(tag, "#");
        strncat(tag, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        strcat(tag, "#");
        if (!strcmp(w, "ft") && fe_strpos(forts, tag) >= 1) goto accept;
        if (!strcmp(w, "st") && fe_strpos(saint_places, tag) >= 1) goto accept;
        if (!strcmp(t->word_lc, "dr") && fe_strpos(doctors, tag) >= 1) goto accept;
    }

    if (i == 1) goto accept;
    if (i > 1 && strcmp(t->punc[i - 1], "\n") == 0) goto accept;
    /* punctuation before: a title starts the next phrase, unless it follows a street or a measure */
    hash_tag(tag, t->punc[i - 1]);
    if (fe_strpos(puncs, tag) && strcmp(t->qual[i - 1], "street") && strncmp(t->qual[i - 1], "measure", 7)) goto accept;
    if (fe_strpos(dots, tag)) goto accept;
    hash_tag(tag, t->sepr[i - 1]);
    if (fe_strpos(puncs, tag)) goto accept;
    hash_tag(tag, t->word[i - 1]);
    if (fe_strpos(puncs, tag)) goto accept;

    /* "5th St", "12 Main St", "5 ft" */
    if (!strcmp(w, "st") && i - 1 > 0) {
        if (strcmp(t->qual[i - 1], "ordnl") == 0) goto fix;
    }
    if (!strcmp(w, "st") && i - 2 >= 1 && t->num[i - 2]) goto fix;
    if (strcmp(w, "ft") == 0) {
        const char *q = t->qual[i - 1];
        if (t->num[i - 1]) goto fix;
        if (!strcmp(q, "number") || !strcmp(q, "ordnl")) goto fix;
        if (i - 2 > 0) {
            if (t->num[i - 2] || !strcmp(t->qual[i - 2], "number") || !strcmp(t->qual[i - 2], "ordnl")) {
                if (!strcmp(last(q, 3), "mod")) goto fix;
            }
        }
    }

    /* "Drs Smith and Jones" */
    if (!strcmp(w, "drs")) {
        if (strncmp(t->punc[i], ".", 1) != 0 || (t->iuc[i] == 0 && t->auc[i] == 0)) {
            if (strcmp(last(t->punc[i], 1), ",") != 0) {
                if (j <= t->cur_word && starts_and(t->word[i + 1])) goto accept;
                if (j <= t->cur_word && !strcmp(last(t->punc[i + 1], 1), ",")) goto accept;
                if (t->cur_word < i + 2 || !starts_and(t->word[i + 2])) goto e2a1;
            }
        }
        goto accept;
    }
e2a1:
    if (!strcmp(w, "ft")) {
        /* "5 ft-lb" */
        if (!((t->cur_word < j || strncmp(t->word[i + 1], "lb", 2)) &&
              (t->cur_word < i + 2 || strncmp(t->word[i + 2], "lb", 2) || strncmp(t->word[i + 1], "-", 1))))
            goto fix;
        if (!strcmp(last(t->qual[i - 1], 3), "mod")) goto fix;
        if (j <= t->cur_word && !strcmp(last(t->qual[i + 1], 3), "mod")) goto fix;
    }
    /* "St John's" */
    if (j <= t->cur_word) {
        if (!strcmp(last(t->word[i + 1], 2), "'s") || !strcmp(last(t->word[i + 1], 2), "s'")) goto accept;
    }
    /* a title after a pronoun, article, conjunction, preposition or verb: "he saw Dr Smith" */
    if (!strcmp(w, "dr") || !strcmp(w, "st")) {
        strcpy(tag, "#");
        strncat(tag, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
        fe_strlwr(tag);
        strcat(tag, "#");
        if (any_stopword(tag)) goto accept;
    }
    if (is_sdf(w)) {
        if (t->fper[i] == 0 && strlen(t->punc[i]) != 0) goto fix;
        if (strlen(t->punc[i]) > 1 && strncmp(t->punc[i], ".", 1) == 0) goto fix;
    }
    /* after a city of the state/province before: "Dallas, TX Dr" */
    if (t->stprov[i - 1] && is_sdf(w)) {
        short back;
        if (!fe_strpos(t->qual[i - 1], " ") || i - 2 < 1) {
            buf[0] = 0;
            back = 2;
        } else {
            buf[0] = 0;
            strncat(buf, t->word[i - 2], strlen(t->word[i - 2]) - strlen(t->punc[i - 2]));
            strcat(buf, " ");
            back = 3;
        }
        strncat(buf, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
        fe_strlwr(buf);
        strcpy(buf2, t->qual[i - 1]);
        fe_strlwr(buf2);
        if (!strcmp(buf2, buf) && (!strcmp(w, "ft") || (i - back > 0 && t->num[i - back]))) goto fix;
    }
    /* "12 Springville Dr", "12 Oak Dr" */
    bare_lc(t, i - 1, buf);
    for (int k = 1; k < 14; k++) {
        buf2[0] = 0;
        strncat(buf2, suffixes + (k - 1) * 7, 7);
        strcpy(buf2, fe_str_trim_spaces(buf2));
        if (strlen(buf2) <= strlen(buf) && !strcmp(buf + (int)strlen(buf) - (int)strlen(buf2), buf2)) {
            if (!strcmp(w, "ft") || (i - 2 > 0 && t->num[i - 1])) goto fix;
        }
    }
    hash_tag(buf2, buf);
    if (fe_strpos(street_names, buf2)) {
        if (!strcmp(w, "ft") || (i - 2 > 0 && t->num[i - 1])) goto fix;
    }
    /* "St Louis", "Ft Worth" in the city table (see is_city) */
    if (j <= t->cur_word && (!strcmp(w, "st") || !strcmp(w, "ft"))) {
        strcpy(buf2, w);
        strcat(buf2, " ");
        strncat(buf2, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        fe_strlwr(buf2);
        if (is_city(buf2)) goto accept;
    }
    /* "St John", "Sts Peter and Paul" */
    if (j <= t->cur_word) {
        if (!((!strcmp(w, "sts") || !strcmp(w, "drs") || !strcmp(w, "fts")) && (t->iuc[i] == 0 || t->fper[i] == 0))) {
            bare_lc(t, i + 1, buf);
            strcpy(buf2, "#");
            if (!strcmp(last(buf, 2), "'s"))
                strncat(buf2, buf, strlen(buf) - 2);
            else
                strcat(buf2, buf);
            strcat(buf2, "#");
            if (fe_strpos(saints_m, buf2) || fe_strpos(saints_f, buf2) || fe_strpos(saints_clare, buf2) ||
                fe_strpos(surnames, buf2))
                goto accept;
        }
    }
    if (!strcmp(w, "sts")) {
        if (!strncmp(t->punc[i], ".", 1) && (t->iuc[i] || t->auc[i])) goto fix;
        if (i - 2 > 0 && starts_and(t->word[i - 2])) goto fix;
    }
    /* between two capitalised words it stays ambiguous: spelled ("s-t") */
    if (is_sdf(w) && (t->iuc[i - 1] || t->auc[i - 1]) && j <= t->cur_word && (t->iuc[i + 1] || t->auc[i + 1])) {
        t->rec_expansion[0] = 0;
        strncat(t->rec_expansion, w, 1);
        strcat(t->rec_expansion, "-");
        strcat(t->rec_expansion, t->word_lc + 1);
        goto accept;
    }
    if (is_sdf(w) && (t->iuc[i - 1] || t->auc[i - 1]) && t->cur_word <= j && t->ilc[i + 1]) {
        if (!(i - 2 > 0 && strlen(t->punc[i - 2]) != 0 && strncmp(t->qual[i - 2], "direction", 9) != 0)) goto fix;
    }
    if (is_sdf(w) && t->ilc[i - 1] && j <= t->cur_word && (t->iuc[i + 1] || t->auc[i + 1])) goto accept;
    /* lists of streets: "Oak, Elm and Main Sts" */
    if (is_sdf(w)) {
        if (!strcmp(t->punc[i - 1], ",") && street_or_measure(t->qual[i - 1])) goto fix;
        if (i - 2 > 0 && starts_and(t->word[i - 1]) && street_or_measure(t->qual[i - 2])) goto fix;
        if (j <= t->cur_word && !strcmp(t->punc[i], ",") && street_or_measure(t->qual[i + 1])) goto fix;
        if (i + 2 <= t->cur_word && starts_and(t->word[i + 1]) && street_or_measure(t->qual[i + 2])) goto fix;
    }
    /* "Mr, Dr and Mrs Smith" */
    if (!strcmp(w, "dr")) {
        p = 0;
        if (!strcmp(t->punc[i - 1], ",")) p = i - 1;
        else if (i - 2 > 0 && starts_and(t->word[i - 1])) p = i - 2;
        else if (j <= t->cur_word && !strcmp(t->punc[i], ",")) p = i + 1;
        else if (i + 2 <= t->cur_word && starts_and(t->word[i + 1])) p = i + 2;
        if (p) {
            bare_lc(t, p, buf);
            if (!strcmp(buf, "mr") || !strcmp(buf, "mrs")) goto accept;
        }
    }
    if (is_sdf(w) && ((j <= t->cur_word && !strncmp(t->qual[i + 1], "direction", 9)) ||
                      (i - 2 > 0 && !strncmp(t->qual[i - 2], "direction", 9))))
        goto fix;
    if (j <= t->cur_word) {
        strcpy(tag, "#");
        strncat(tag, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        fe_strlwr(tag);
        strcat(tag, "#");
        if (fe_strpos(apartment, tag)) goto fix;
    }
    if (t->cur_word != i || !is_sdf(w)) goto done;

fix:
    if (!strcmp(w, "dr")) { strcpy(t->rec_expansion, "Drive"); strcpy(t->qual[i], "street"); }
    if (!strcmp(w, "drs")) { strcpy(t->rec_expansion, "drives"); strcpy(t->qual[i], "street"); }
    if (!strcmp(w, "ft")) { strcpy(t->rec_expansion, "foot"); strcpy(t->qual[i], "measure2"); }
    if (!strcmp(w, "st")) { strcpy(t->rec_expansion, "Street"); strcpy(t->qual[i], "street"); }
    if (!strcmp(w, "sts")) { strcpy(t->rec_expansion, "streets"); strcpy(t->qual[i], "street"); }
accept:
    ok = 1;
done:
    *res = ok;
}
