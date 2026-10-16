/* street: street types (REFERENCE 3.4a). */
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
static int is_article(const char *w) { return !strcmp(w, "the") || !strcmp(w, "an") || !strcmp(w, "a"); }

/* Is s a city name? The original compares with the state column of the city table, from the second row, so it never
   matches; fe_fixes compares with the city column. */
static int is_city(const char *s)
{
    for (short k = 0; k <= fe_city_state_count - 2; k++)
        if (!strcmp(fe_fixes ? fe_city_state[k].b : fe_city_state[k + 1].a, s)) return 1;
    return fe_fixes && !strcmp(fe_city_state[fe_city_state_count - 1].b, s);
}

/* Univ / Pt (Port): before "of" + a capital, after a capitalised word that is not an article. @0x10052ee0 */
void fe_Street_ArticleCheck(TextProc *t, short i, short *res)
{
    short ok = *res;
    char w[64];

    if (!(t->cur_word < i + 1)) {
        if ((t->iuc[i + 1] || t->auc[i + 1]) && strcmp(t->word_lc, "univ") != 0) goto accept;
        if (i + 2 <= t->cur_word && !strcmp(t->word[i + 1], "of") && (t->iuc[i + 2] || t->auc[i + 2])) goto accept;
    }
    if (!strcmp(t->word_lc, "univ") && t->iuc[i]) goto accept;
    if (i - 1 < 1 || strcmp(t->word_lc, "univ") != 0) goto done;
    if (t->iuc[i - 1] || t->auc[i - 1]) {
        strcpy(w, t->word[i - 1]);
        fe_strlwr(w);
        if (is_article(w) || i - 2 < 1 || !strcmp(t->punc[i - 2], ".")) goto done;
    }
accept:
    ok = 1;
done:
    *res = ok;
}

/* @0x10050f80 */
void fe_Rule_Street(TextProc *t, short i, short *res)
{
    static const char units[] = "#pt#apt#univ#sq#rm#ste#";
    static const char always[] = "#blvd#expy#fwy#hwy#pkwy#tpke#xing#ave#ctr#hts#jct#mtn#rte#rt#pky#";
    static const char after_cap[] = "#av#ave#rd#pl#crk#cyn#ctr#jct#ln#mtn#pk#rte#trl#univ#cir#aly#ter#terr#tr#";
    static const char street_like[] = "#ct#ctr#dr#pt#sq#st#univ#wy#";
    static const char suffixes[] = "ville  burg   burgh  boro   boroughbury   town   port   field  meadow wood   ington ingham ";
    static const char street_names[] =
        "#central#main#washington#lincoln#jefferson#birch#maple#elm#oak#pine#palm#hill#wall#state#spring#broad#baker#"
        "bourbon#downing#high#park#ash#beech#forest#river#riverside#lake#lakeshore#";
    static const char *const not_before[] = {
        "#anyone#everyone#he#his#her#him#he's#himself#herself#i#it#its#i'm#i'd#i'll#i've#itself#my#me#much#myself#our#"
        "per#pre#re#so#she#such#someone#somebody#this#they#than#them#their#these#those#they'd#there#themselves#us#used#"
        "very#we#",
        "#you#your#you'd#you're#yourself#yourselves#be#can#have#the#a#an#am#for#from#of#shall#away#about#after#among#"
        "along#above#around#across#against#amongst#below#before#behind#beyond#beside#between#during#despite#except#into#"
        "inside#",
        "#over#outside#throughout#upon#under#until#within#without#all#any#anything#both#double#each#every#either#"
        "everything#few#half#more#most#many#no#none#nothing#neither#some#several#something#can't#cannot#couldn't#don't#"
        "didn't#isn't#",
        "#not#never#won't#wasn't#wouldn't#do#did#does#as#at#by#down#in#near#on#out#off#to#toward#through#with#up#may#"
        "must#might#are#been#could#had#has#is#should#was#were#will#would#how#who#why#when#what#whom#which#where#whose#"
        "who'd#",
        "#whoever#whatever#and#but#nor#or#altho#although#because#even#furthermore#however#henceforth#if#insofar#"
        "inasmuch#moreover#since#that#then#though#thereby#therefore#thereupon#unless#while#whether#whereas#whereby#"
        "wherein#whenever#",
        "#wherever#wherupon#wherefore#yet#",
    };
    static const char rooms[] = "#building#bldg#suite#ste#room#rm#condominium#condo#townhouse#th###box#drop#";
    static const char on_along[] = "#on#along#across#at#down#";
    short ok = *res;
    const char *w = t->word_lc;
    char tag[140], buf[140], buf2[140], first[64], lastc[8];
    int j = i + 1;
    short pos;

    if (!strcmp(w, "ave") && j <= t->cur_word) {
        bare_lc(t, i + 1, buf);
        if (!strcmp(buf, "maria")) goto done;                 /* Ave Maria */
    }
    hash_tag(tag, w);
    if (fe_strpos(always, tag)) goto accept;
    if (fe_strpos(after_cap, tag) && t->iuc[i] && i > 1 && t->iuc[i - 1]) {
        bare(t, i - 1, buf);
        if (strcmp(buf, "A") && strcmp(buf, "An") && strcmp(buf, "The")) goto accept;
    }

    /* next to another street word: "Oak St Ct", "Univ Ct" */
    if (i - 1 >= 1) {
        strcpy(tag, "#");
        strncat(tag, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
        fe_strlwr(tag);
        strcat(tag, "#");
        if (!strcmp(t->qual[i - 1], "street") || fe_strpos(street_like, tag)) {
            if ((!strcmp(w, "ct") || !strcmp(w, "wy")) && !strncmp(t->word[i - 1], "univ", 4)) goto main;
            if (strncmp(w, "univ", 4) != 0) goto accept;
            if (!strncmp(t->word[i - 1], "ct", 2)) goto main;
            if (strncmp(t->word[i - 1], "wy", 2) != 0) goto accept;
            goto main;
        }
    }
    if (j <= t->cur_word) {
        strcpy(tag, "#");
        strncat(tag, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        fe_strlwr(tag);
        strcat(tag, "#");
        if (!strcmp(t->qual[i + 1], "street") || fe_strpos(street_like, tag)) {
            if ((!strcmp(w, "ct") || !strcmp(w, "wy")) && !strncmp(t->word[i + 1], "univ", 4)) goto main;
            if (strncmp(w, "univ", 4) != 0) goto accept;
            if (!strncmp(t->word[i + 1], "ct", 2)) goto main;
            if (strncmp(t->word[i + 1], "wy", 2) != 0) goto accept;
        }
    }

main:
    /* capitalised abbreviations and the unit words go to the unit switch first */
    if (t->iuc[i] == 1) goto units;
    if (!strcmp(w, "univ") || !strcmp(w, "apt") || !strcmp(w, "rm") || !strcmp(w, "ste")) goto units;
general:
    if (i - 1 > 0) {
        const char *p = t->punc[i - 1];
        if (strlen(p) != 0) {
            if (!fe_strpos(p, ".") || t->stprov[i - 1] == 0) goto done;
            if (strlen(t->word[i - 1]) - strlen(p) > 5) goto done;
        }
        strcpy(tag, "#");
        strncat(tag, t->word[i - 1], strlen(t->word[i - 1]) - strlen(p));
        fe_strlwr(tag);
        strcat(tag, "#");
        for (int l = 0; l < 6; l++)
            if (fe_strpos(not_before[l], tag)) goto done;
    }
    if (i == 1) goto done;
    if (i > 1 && !strcmp(t->punc[i - 1], "\n")) goto done;
    if (j <= t->cur_word) {
        strcpy(tag, "#");
        strncat(tag, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        fe_strlwr(tag);
        strcat(tag, "#");
        if (fe_strpos(rooms, tag)) goto accept;
    }
    if (!strcmp(w, "ct") || !strcmp(w, "wy")) {
        /* "12 Oak Ct", "on Oak Wy" */
        if (t->iuc[i] == 0 || i - 2 < 1) goto done;
        if (t->iuc[i - 1] == 0 && (strncmp(t->word[i - 1], "1", 1) < 0 || strncmp(t->word[i - 1], "9", 1) > 0)) goto done;
        strcpy(tag, "#");
        strncat(tag, t->word[i - 2], strlen(t->word[i - 2]) - strlen(t->punc[i - 2]));
        fe_strlwr(tag);
        strcat(tag, "#");
        if (!fe_strpos(on_along, tag) && t->num[i - 2] == 0 &&
            (i - 3 < 1 || (t->num[i - 3] == 0 && (i - 4 < 1 || t->num[i - 4] == 0))))
            goto done;
        goto accept;
    }
    if (i - 1 >= 1) {
        const char *q = t->qual[i - 1];
        if (!strcmp(q, "ordnl")) goto accept;                 /* "5th Ave" */
        bare_lc(t, i - 1, buf);
        if (is_city(buf)) goto accept;                         /* "Washington St" */
        if (i - 2 > 0) {
            bare_lc(t, i - 2, buf2);
            strcat(buf2, " ");
            strcat(buf2, buf);
            if (is_city(buf2)) goto accept;                    /* "San Jose Ave" */
        }
        if (t->stprov[i - 1]) {
            if (!fe_strpos(q, " ") || i - 2 < 1) {
                buf2[0] = 0;
            } else {
                bare(t, i - 2, buf2);
                strcat(buf2, " ");
            }
            strncat(buf2, t->word[i - 1], strlen(t->word[i - 1]) - strlen(t->punc[i - 1]));
            fe_strlwr(buf2);
            strcpy(tag, q);
            fe_strlwr(tag);
            if (!strcmp(tag, buf2)) goto accept;
        }
        bare_lc(t, i - 1, buf);
        for (int k = 1; k < 14; k++) {
            buf2[0] = 0;
            strncat(buf2, suffixes + (k - 1) * 7, 7);
            strcpy(buf2, fe_str_trim_spaces(buf2));
            if (strlen(buf2) <= strlen(buf) && !strcmp(buf + (int)strlen(buf) - (int)strlen(buf2), buf2)) goto accept;
        }
        hash_tag(tag, buf);
        if (fe_strpos(street_names, tag)) goto accept;
    }
    /* "Main and Elm Sts", "Oak St or Elm St" */
    if (t->cur_word < j || strncmp(t->qual[i + 1], "direction", 9) != 0) {
        if (i - 2 > 0 && !strcmp(t->qual[i - 2], "street") &&
            (!strcmp(t->word[i - 1], "and") || !strcmp(t->word[i - 1], "or")))
            goto accept;
        if (t->cur_word < i + 2 || strcmp(t->qual[i + 2], "street") != 0) goto done;
        if (strcmp(t->word[i + 1], "and") && strcmp(t->word[i + 1], "or")) goto done;
    }
    goto accept;

units:
    hash_tag(tag, w);
    pos = fe_strpos(units, tag);
    if (pos == 0) goto general;
    switch (pos) {
    case 1:                                                   /* pt */
        if (!strcmp(t->rec_expansion, "Port")) {
            fe_Street_ArticleCheck(t, i, &ok);
            goto done;
        }
        break;
    case 4: case 16: case 19:                                 /* apt, rm, ste: before a number or a capital letter */
        if (j <= t->cur_word) {
            strcpy(buf, t->word[i + 1]);
            first[0] = 0;
            strncat(first, buf, 1);
            strcpy(lastc, fe_tail(buf, 1));
            if (!strcmp(lastc, ".") || !strcmp(lastc, ",") || !strcmp(lastc, "\"") || !strcmp(lastc, "?") ||
                !strcmp(lastc, "!") || !strcmp(lastc, ";") || !strcmp(lastc, ":")) {
                buf2[0] = 0;
                strncat(buf2, buf, strlen(buf) - 1);
                strcpy(buf, buf2);
            }
            strcpy(lastc, first);
            fe_strlwr(lastc);
            if (strcmp(first, "0") >= 0 && strcmp(first, "9") <= 0) goto accept;
            if (strlen(buf) == 1 && strcmp(first, lastc) != 0) goto accept;
        }
        break;
    case 8:                                                   /* univ */
        fe_Street_ArticleCheck(t, i, &ok);
        goto done;
    case 13:                                                  /* sq: Square, unless a military unit is near */
        ok = 1;
        for (short s = i - 3; s <= i + 3; s++) {
            if (s > 0 && s <= t->cur_word) {
                if (!strcmp(t->qual[s], "ordnl")) { ok = 0; break; }
                strcpy(buf, t->word[s]);
                fe_strlwr(buf);
                strcpy(buf2, "");
                if (s + 1 <= t->cur_word) {
                    strcpy(buf2, t->word[s + 1]);
                    fe_strlwr(buf2);
                }
                if (!strncmp(buf, "fighter", 7) || !strncmp(buf, "bomber", 6) || !strncmp(buf, "transport", 9) ||
                    !strncmp(buf, "tactical", 8) || (!strncmp(buf, "air", 3) && !strncmp(buf2, "force", 5))) {
                    ok = 0;
                    break;
                }
            }
        }
        goto done;
    }
    /* after a capitalised word that is not an article */
    if (i - 1 > 0 && t->iuc[i - 1]) {
        strcpy(buf, t->word[i - 1]);
        fe_strlwr(buf);
        if (!is_article(buf)) goto accept;
    }
    goto done;

accept:
    ok = 1;
done:
    *res = ok;
}
