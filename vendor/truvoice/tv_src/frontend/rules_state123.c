/* state1-3: US state abbreviations (REFERENCE 3.4a). Written from the assembly (tools/asm_condense.py): the
   decompiler merges the per-state tails. State_Helper_B handles the two-word keys ("N Dak", "New Mex"). */
#include <stdlib.h>
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
static int ends(const char *w, const char *s)
{
    size_t n = strlen(w), m = strlen(s);
    return n >= m && !strcmp(w + n - m, s);
}

/* A two-word key: 1 = it matches (the expansion replaces the next word), 2 = it does not, else unchanged.
   @0x10069640 */
void fe_State_Helper_B(TextProc *t, short i, short *res)
{
    short r = *res;
    char w[64], nw[64], pun[64], buf[160], lastc[8];

    if (fe_strpos(t->rec_keyword, " ")) {
        r = 2;
        fe_str_replace_all(t->word_lc, ".", " ", w);
        if (!strcmp(w, t->rec_keyword)) {
            r = 1;
        } else if (i + 1 <= t->cur_word && t->iuc[i + 1]) {
            strcpy(nw, t->word[i + 1]);
            fe_strlwr(nw);
            strcpy(pun, "");
            /* The original never returns when the next word ends in four or more of these marks: its pun buffer
               is 4 bytes, directly followed by nw, so the fourth mark's terminator empties nw, and the "last
               character" of the empty nw is then pun[3], stripped and appended again for ever (REFERENCE 3.4c).
               The port ends the loop normally. */
            for (;;) {
                strcpy(lastc, fe_tail(nw, 1));
                if (strcmp(lastc, ".") && strcmp(lastc, ",") && strcmp(lastc, "\"") && strcmp(lastc, "?") &&
                    strcmp(lastc, "!") && strcmp(lastc, ":") && strcmp(lastc, ";"))
                    break;
                strcpy(w, "");
                strncat(w, nw, strlen(nw) - 1);
                strcpy(nw, w);
                strcat(pun, lastc);
            }
            strcpy(buf, t->word_lc);
            strcat(buf, " ");
            strcat(buf, nw);
            if (!strcmp(buf, t->rec_keyword)) {
                r = 1;
                strcpy(buf, t->rec_expansion);
                strcat(buf, pun);
                strcpy(t->rec_expansion, "");
                if (strlen(buf) < 30) {
                    strcpy(t->word[i + 1], buf);
                } else {
                    fe_free(t->word[i + 1]);
                    t->word[i + 1] = fe_strdup(buf);
                }
            } else {
                r = 2;
            }
        }
    }
    *res = r;
}

/* The context flags. */
enum { F_ZIP, F_CITY, F_PERIOD, F_HEADER, F_TOWN, F_UNIV, F_PLACE, F_SAINT, F_LIST, F_POLIT, F_FROM, NFLAGS };

/* s contains no common word and at most one of the ambiguous abbreviations */
static int list_ok(const char *s)
{
    static const char *const amb[] = {"al", "cal", "ida", "del", "mo", "tex", "miss", "ms", "penn", "pa", "ma", "col", "md"};
    int cnt = 0;
#define H(x) (fe_strpos(s, x) != 0)
    if (!(!H("ok") && !H("oh") && !H("hi") && !H("me") && (!H("pa") || !H("ma")) &&
          (!H("la") || (!H("ny") && !H("ca") && !H("cal") && !H("calif") && !H("sc"))) && (!H("sc") || !H("cal"))))
        return 0;
#undef H
    for (int k = 0; k < 13; k++)
        if (fe_strpos(s, amb[k])) cnt++;
    return cnt < 2;
}

/* @0x100641d0 */
void fe_Rule_State123(TextProc *t, short i, short *res)
{
    static const char state1_caps[] = "#AK#Ca#CA#Conn#DE#IL#La#Mon#MT#N.C#N.D#N.M#N.H#n.J#O#R.I#S.C#s.D#TX#WA#WY#Wy#";
    static const char abbr_caps[] =
        "#Ala#Ariz#Ark#Calif#Colo#Fla#Ga#Ia#Kan#Kans#Ky#Md#Mich#Minn#Neb#Nebr#Nev#Okla#Oreg#Penna#Tenn#Va#Vt#Wis#Wisc#"
        "Wyo#NDak#SDak#NMex#WVa#AZ#CA#FL#DC#GA#IA#KS#KY#MN#ND#NV#NM#NJ#NY#VT#WI#WV#WY#";
    static const char abbr2_caps[] = "#N Dak#N Mex#S Dak#W Va#";
    static const char abbr_lc[] =
        "#ala#ariz#ark#calif#colo#fla#ga#ia#kan#kans#ky#md#mich#minn#neb#nebr#nev#okla#oreg#penna#tenn#va#vt#wis#wisc#"
        "wyo#ndak#sdak#nmex#wva#az#ca#fl#dc#ga#ia#ks#ky#mn#nd#nv#nm#nj#ny#wi#wv#wy#";
    static const char abbr2_lc[] = "#n dak#n mex#s dak#w va#";
    static const char saints[] = "#san#santa#los#las#st#saint#north#south#east#west#new#";
    static const char places1[] =
        "#valley#hts#heights#falls#city#village#town#township#beach#pk#park#bay#harbor#ridge#peak#summit#shore#spring#"
        "springs#hill#hills#bend#bluff#bluffs#";
    static const char places2[] = "#island#jct#junction#plain#plains#pt#point#rapids#ctr#center#cyn#canyon#afb#";
    static const char places3[] = "#mt#mount#mtn#mountain#ft#fort#lake#lk#port#pt#mcas#camp#";
    static const char california[] = "#california#calif#cal#ca#";
    static const char polit1[] =
        "#state#govt#government#lottery#lotteries#delegate#delegation#primary#primaries#election#law#legistlature#"
        "assembly#assemblies#senate#gov#governor#lt#lieutenant#assemblyman#assemblywoman#assemblymen#assemblywomen#"
        "senator#";
    static const char polit2[] =
        "#congressman#congresswoman#congressmen#congresswomen#legistlator#legistlative#representative#atty#attorney#"
        "natl#national#hwy#highway#ptrl#coastline#border#dept#department#court#judge#judiciary#judicial#supreme#dist#"
        "district#";
    static const char polit3[] = "#appellate#rptr#";
    static const char from[] =
        "#state of#sen from#senator from#congressman from#congreswoman from#representative from#governor of#federal and#";
    short ok = *res, r;
    const char *w = t->word_lc;
    char n1[64], n2[64], n3[64], p1[64], p2[64], p3[64], tag[200], s[300];
    char *slot[7] = {p3, p2, p1, NULL, n1, n2, n3};
    short f[NFLAGS] = {0};
    int any;

    if (!strcmp(w, "md") && t->iuc[i]) goto accept;
    if (!strcmp(w, "id") && (t->ilc[i] || (i + 1 <= t->cur_word && t->num[i + 1]))) goto done;
    r = 0;
    fe_State_Helper_B(t, i, &r);
    if (r == 1) goto accept;
    if (r == 2) goto done;
    if (!strcmp(w, "la")) {
        if (t->auc[i]) strcpy(t->rule_type, "state2");
        if (t->iuc[i]) strcpy(t->rule_type, "state1");
    }
    if (!strcmp(w, "ca") && t->ilc[i]) strcpy(t->rule_type, "state3");
    if (!strcmp(w, "md") && t->auc[i]) strcpy(t->rule_type, "state2");
    for (short k = -3; k < 4; k++) {
        char tmp[64];
        int j = i + k;
        if (j < 1 || t->cur_word < j) strcpy(tmp, "Z!@Z");
        else bare(t, j, tmp);
        fe_strlwr(tmp);
        fe_str_replace_all(tmp, ".", " ", tmp);
        if (slot[k + 3]) strcpy(slot[k + 3], tmp);
    }

    r = 0;
    fe_State_Helper_A(t, i, &r);
    if (r) f[F_CITY] = 1;
    if (t->fe_mode == 3 && t->line_no > 1) f[F_HEADER] = 1;   /* never: both callers set fe_mode to 1 */
    if (!strcmp(t->rule_type, "state1")) {
        if (f[F_CITY] || f[F_HEADER]) goto accept;
        if (t->iuc[i] == 0 && t->auc[i] == 0 && strcmp(w, "il") != 0) goto done;
    }
    /* a zip code follows */
    if (i + 1 <= t->cur_word && t->num[i + 1] && strlen(n1) == 5 &&
        (!strcmp(t->punc[i], "") || (!strcmp(t->punc[i], ".") && t->auc[i] != 1)))
        f[F_ZIP] = 1;
    if (t->iuc[i] && f[F_ZIP]) goto accept;

    if (i + 1 <= t->cur_word && !strcmp(t->punc[i], ".") && t->ilc[i + 1]) f[F_PERIOD] = 1;
    if (i - 1 > 0 && (!strcmp(t->punc[i - 1], "") || !strcmp(t->punc[i - 1], ","))) {
        if (ends(p1, "burg") || ends(p1, "boro") || ends(p1, "bury") || ends(p1, "town") || ends(p1, "port") ||
            ends(p1, "wood") || ends(p1, "ville") || ends(p1, "burgh") || ends(p1, "field") || ends(p1, "meadow") ||
            ends(p1, "ington") || ends(p1, "ingham") || ends(p1, "borough"))
            f[F_TOWN] = 1;
    }
    if (i - 2 > 0 && !strcmp(t->punc[i - 1], "") && !strcmp(p1, "of") && cap(t, i - 2) &&
        (!strcmp(p2, "u") || !strcmp(p2, "univ") || !strcmp(p2, "university") || !strcmp(p2, "bank")))
        f[F_UNIV] = 1;
    if (i + 1 <= t->cur_word && !strcmp(t->punc[i], "") && cap(t, i + 1) &&
        (!strcmp(n1, "u") || !strcmp(n1, "univ") || !strcmp(n1, "university") || !strcmp(n1, "coll") ||
         !strcmp(n1, "college") || !strcmp(n1, "tech") || !strcmp(n1, "bank")))
        f[F_UNIV] = 1;
    if (i - 1 > 0) {
        hash_tag(tag, p1);
        if (fe_strpos(places1, tag) || fe_strpos(places2, tag)) f[F_PLACE] = 1;
        hash_tag(tag, p2);
        if (fe_strpos(places3, tag)) f[F_PLACE] = 1;
    }
    hash_tag(tag, p1);
    if (i - 1 >= 1 && cap(t, i - 1) && fe_strpos(saints, tag)) {
        f[F_SAINT] = 1;
    } else {
        hash_tag(tag, p2);
        if (i - 2 > 0 && cap(t, i - 1) && cap(t, i - 2) && fe_strpos(saints, tag)) {
            f[F_SAINT] = 1;
        } else {
            hash_tag(tag, p3);
            if (i - 3 > 0 && cap(t, i - 1) && cap(t, i - 2) && cap(t, i - 3) && fe_strpos(saints, tag)) f[F_SAINT] = 1;
        }
    }
    /* a list of states */
    if (!(t->cur_word < i + 2 || t->stprov[i + 1] == 0)) {
        if (t->stprov[i + 2] == 0 && !(i + 3 <= t->cur_word && is_conj3(n2) && t->stprov[i + 3])) goto a55;
        strcpy(s, w); strcat(s, " "); strcat(s, n1); strcat(s, " "); strcat(s, n2); strcat(s, " "); strcat(s, n3);
        if (!strcmp(w, n1) || !strcmp(w, n2) || !strcmp(w, n3) || !strcmp(n1, n2) || !strcmp(n1, n3) || !strcmp(n2, n3))
            goto a55;
        goto list;
    }
a55:
    if (i - 1 > 0 && i + 2 <= t->cur_word && t->stprov[i - 1]) {
        if (t->stprov[i + 1] || (is_conj3(n1) && t->stprov[i + 2])) {
            strcpy(s, p1); strcat(s, " "); strcat(s, w); strcat(s, " "); strcat(s, n1); strcat(s, " "); strcat(s, n2);
            if (strcmp(p1, w) && strcmp(p1, n1) && strcmp(p1, n2) && strcmp(w, n1) && strcmp(w, n2) && strcmp(n1, n2))
                goto list;
        }
    }
    if (i - 2 > 0 && t->stprov[i - 2]) {
        if (t->stprov[i - 1] || (i - 3 > 0 && is_conj3(p1) && t->stprov[i - 3])) {
            strcpy(s, w); strcat(s, " "); strcat(s, p1); strcat(s, " "); strcat(s, p2); strcat(s, " "); strcat(s, p3);
            if (strcmp(w, p1) && strcmp(w, p2) && strcmp(w, p3) && strcmp(p1, p2) && strcmp(p1, p3) && strcmp(p2, p3))
                goto list;
        }
    }
    goto polit;
list:
    if (list_ok(s)) f[F_LIST] = 1;
polit:
    /* a political word follows ("Tex Senate", "Calif legislators") */
    if (i + 1 <= t->cur_word) {
        hash_tag(tag, n1);
        if (fe_strpos(polit1, tag) || fe_strpos(polit2, tag) || fe_strpos(polit3, tag)) f[F_POLIT] = 1;
        if (!f[F_POLIT] && !strcmp(fe_tail(n1, 1), "s")) {
            strcpy(tag, "#");
            strncat(tag, n1, strlen(n1) - 1);
            strcat(tag, "#");
            if (fe_strpos(polit1, tag) || fe_strpos(polit2, tag) || fe_strpos(polit3, tag)) f[F_POLIT] = 1;
        }
    }
    /* "Senator from Tex", "Orange County, Calif", military bases */
    if (i - 2 > 0) {
        strcpy(tag, "#"); strcat(tag, p2); strcat(tag, " "); strcat(tag, p1); strcat(tag, "#");
        if (fe_strpos(from, tag)) f[F_FROM] = 1;
    }
    if (i - 1 > 0 && (!strcmp(p1, "county") || !strcmp(p1, "afb"))) f[F_FROM] = 1;
    if (!strcmp(p1, "mcas") || !strcmp(p2, "mcas") || !strcmp(p3, "mcas")) f[F_FROM] = 1;
    if (!strcmp(p1, "mcb") || !strcmp(p2, "mcb") || !strcmp(p3, "mcb")) f[F_FROM] = 1;
    if (!strcmp(p1, "usnas") || !strcmp(p2, "usnas") || !strcmp(p3, "usnas")) f[F_FROM] = 1;

    any = 0;
    for (int k = 0; k < NFLAGS; k++) any |= f[k] != 0;
    /* two independent pieces of evidence together are enough */
    {
#define F(x) f[F_##x]
        if (F(ZIP) && (F(CITY) || F(PERIOD) || F(HEADER) || F(TOWN) || F(UNIV) || F(PLACE) || F(SAINT) || F(LIST) ||
                       F(POLIT) || F(FROM)))
            goto accept;
        if (F(CITY) && (F(PERIOD) || F(HEADER) || F(UNIV) || F(LIST) || F(POLIT) || F(FROM))) goto accept;
        if (F(PERIOD) && (F(HEADER) || F(TOWN) || F(UNIV) || F(PLACE) || F(SAINT) || F(LIST) || F(FROM))) goto accept;
        if (F(HEADER) && (F(TOWN) || F(UNIV) || F(PLACE) || F(SAINT) || F(LIST) || F(POLIT) || F(FROM))) goto accept;
        if (F(TOWN) && (F(UNIV) || F(LIST) || F(POLIT) || F(FROM))) goto accept;
        if (F(UNIV) && (F(PLACE) || F(SAINT) || F(LIST) || F(POLIT) || F(FROM))) goto accept;
        if (F(PLACE) && (F(LIST) || F(POLIT) || F(FROM))) goto accept;
        if (F(SAINT) && (F(LIST) || F(POLIT) || F(FROM))) goto accept;
        if (F(LIST) && (F(POLIT) || F(FROM))) goto accept;
        if (F(POLIT) && F(FROM)) goto accept;
    }
    strcpy(tag, "#");
    strncat(tag, t->word[i], strlen(t->word[i]) - strlen(t->punc[i]));
    strcat(tag, "#");
    if (!strcmp(t->rule_type, "state1") && fe_strpos(state1_caps, tag)) {
        if (strcmp(w, "o") != 0 && any) goto accept;
        if (!strcmp(w, "o") && (F(ZIP) || F(CITY) || F(PERIOD) || F(HEADER) || F(TOWN) || F(UNIV) || F(PLACE) ||
                                F(SAINT) || F(LIST)))
            goto accept;
    }
    if (!strcmp(t->rule_type, "state1") && (F(CITY) || F(HEADER))) goto accept;
    if (!strcmp(w, "il") && t->ilc[i] && (F(ZIP) || F(TOWN) || F(PLACE) || F(LIST))) goto accept;

    if (!strcmp(t->rule_type, "state2") && (t->iuc[i] || t->auc[i])) {
        /* the ambiguous ones, each with the evidence it needs */
        if (!strcmp(w, "oh") || !strcmp(w, "hi") || !strcmp(w, "me") || !strcmp(w, "ha") || !strcmp(w, "or")) {
            if (F(ZIP) || F(CITY) || F(PERIOD) || F(HEADER)) goto accept;
            goto spelled;
        }
        if (!strcmp(w, "al") || !strcmp(w, "cal") || !strcmp(w, "penn") || !strcmp(w, "ida") || !strcmp(w, "del") ||
            !strcmp(w, "mo")) {
            if (!(F(ZIP) || F(CITY) || F(PERIOD) || F(HEADER))) {
                if (F(POLIT)) goto state_word;
                if (!F(FROM)) goto done;
            }
            if (!F(POLIT)) goto accept;
state_word:
            if (strcmp(w, "cal") && strcmp(w, "penn")) goto accept;
            if (strcmp(n1, "state") != 0) goto accept;
            goto done;
        }
        if (!strcmp(w, "mont") || !strcmp(w, "tex") || !strcmp(w, "pa") || !strcmp(w, "ma") || !strcmp(w, "mass") ||
            !strcmp(w, "ore")) {
            if (F(ZIP) || F(CITY) || F(PERIOD) || F(HEADER) || F(POLIT) || F(FROM)) goto accept;
            goto done;
        }
        if (!strcmp(w, "miss") || !strcmp(w, "wash") || !strcmp(w, "alas") || !strcmp(w, "ill")) {
            if (F(ZIP) || F(CITY) || F(PERIOD) || F(HEADER)) goto accept;
            goto done;
        }
        if (!strcmp(w, "col") || !strcmp(w, "ok") || !strcmp(w, "md") || !strcmp(w, "ms") || !strcmp(w, "ne")) {
            if (F(ZIP) || F(CITY) || F(HEADER) || F(POLIT) || F(FROM)) goto accept;
            goto done;
        }
        if (!strcmp(w, "in")) {
            if (F(ZIP) || F(CITY) || F(HEADER) || F(POLIT) || F(FROM)) goto accept;
            goto spelled_caps;
        }
        if (!strcmp(w, "ct") && strcmp(n1, "scan") != 0) {
            if (F(ZIP) || F(CITY) || F(UNIV) || F(LIST)) goto accept;
            if (!F(POLIT) && !F(FROM)) goto done;
            if (!strcmp(n1, "election") || !strcmp(n1, "law") || !strcmp(n1, "supreme") || !strcmp(n1, "appellate") ||
                !strcmp(n1, "district") || !strcmp(n1, "court") || !strcmp(n1, "rptr"))
                goto done;
            if (strcmp(p1, "county") != 0) goto accept;
            goto done;
        }
        if (!strcmp(w, "ind") || !strcmp(w, "id")) {
            if (F(ZIP) || F(CITY) || F(HEADER) || F(POLIT) || F(FROM)) goto accept;
            goto done;
        }
        if (!strcmp(w, "la")) {
            if (!(F(ZIP) || F(CITY) || F(LIST) || F(POLIT))) goto done;
            if (F(POLIT) && strcmp(n1, "state") && strcmp(n1, "lottery") && strcmp(n1, "delegation") &&
                strcmp(n1, "primary") && strcmp(n1, "legislature") && strcmp(n1, "lt") && strcmp(n1, "gov") &&
                strcmp(n1, "governor") && strcmp(n1, "senator") && strcmp(n1, "atty") && strcmp(n1, "attorney") &&
                strcmp(n1, "natl") && strcmp(n1, "national") && strcmp(n1, "rptr"))
                goto done;
            /* "LA 90012": Los Angeles */
            if (t->auc[i] && F(ZIP) && !strncmp(n1, "9", 1) && strncmp(n1 + 1, "0", 1) >= 0 && strncmp(n1 + 1, "5", 1) <= 0) {
                strcpy(t->rec_expansion, "Los Angeles");
                goto accept;
            }
            /* Louisiana, unless California is mentioned nearby */
            for (short k = -5; k <= 5; k++) {
                int j = i + k;
                if (j >= 1 && j <= t->cur_word) {
                    strcpy(tag, "#");
                    strncat(tag, t->word[j], strlen(t->word[j]) - strlen(t->punc[j]));
                    strcat(tag, "#");
                    fe_strlwr(tag);
                    if (fe_strpos(california, tag)) goto done;
                }
            }
            goto accept;
        }
        if (!strcmp(w, "va")) {
            if (F(ZIP) || F(CITY) || F(UNIV) || F(LIST) || F(POLIT) || F(FROM)) goto accept;
            goto done;
        }
        if (!strcmp(w, "sc") || !strcmp(w, "nc") || !strcmp(w, "nd") || !strcmp(w, "nm") || !strcmp(w, "nh") ||
            !strcmp(w, "nj") || !strcmp(w, "ri") || !strcmp(w, "sd")) {
            if (!strcmp(w, "sc") ? (F(PERIOD) || F(UNIV) || F(SAINT)) : (F(PERIOD) || F(TOWN) || F(SAINT))) goto done;
            if (F(PLACE)) {
                if (!strcmp(p1, "beach") || !strcmp(p1, "park") || !strcmp(p1, "harbor") || !strcmp(p1, "shore") ||
                    !strcmp(p1, "lake") || !strcmp(p1, "springs") || !strcmp(p1, "hills"))
                    goto done;
            }
            if (any) goto accept;
            goto done;
        }
        if (!strcmp(w, "co")) {
            if (F(PERIOD) || F(HEADER) || F(SAINT)) goto done;
            if (!strcmp(t->sepr[i], "-")) goto done;
            if (t->iuc[i] && (F(TOWN) || F(PLACE) || F(LIST))) goto done;
            if (any) goto accept;
            goto done;
        }
    }
    goto state2_lc;

spelled:
    if (F(ZIP) || F(CITY) || F(PERIOD) || F(HEADER)) goto accept;
spelled_caps:
    /* "OH", "IN" as letters after a place word: "O-H" */
    if (t->auc[i] == 0) goto done;
    if (strcmp(w, "hi") && strcmp(w, "in") && strcmp(w, "me") && strcmp(w, "oh") && strcmp(w, "or")) goto done;
    if (!(F(TOWN) || F(UNIV) || F(PLACE) || F(SAINT) || F(LIST))) goto done;
    t->rec_expansion[0] = 0;
    strncat(t->rec_expansion, t->word[i], 1);
    strcat(t->rec_expansion, "-");
    strncat(t->rec_expansion, t->word[i] + 1, 1);
    goto accept;

state2_lc:
    if (!strcmp(t->rule_type, "state2") && t->ilc[i] && strcmp(w, "oh") && strcmp(w, "hi") && strcmp(w, "me") &&
        strcmp(w, "ha") && strcmp(w, "ok") && strcmp(w, "in") && strcmp(w, "or")) {
        if (F(ZIP) || F(CITY)) goto accept;
    }
    if (strcmp(t->rule_type, "state3") != 0) goto done;

    /* state3: a known abbreviation (evidence needed for the lower-case spelling), then the exceptions */
    strcpy(tag, "#");
    strncat(tag, t->word[i], strlen(t->word[i]) - strlen(t->punc[i]));
    strcat(tag, "#");
    if (fe_strpos(abbr_caps, tag)) goto e84;
    if (F(ZIP) || F(CITY) || F(PERIOD) || F(TOWN) || F(UNIV) || F(PLACE)) {
        fe_strlwr(tag);
        if (fe_strpos(abbr_lc, tag)) goto e84;
    }
    if (i + 1 <= t->cur_word) {
        strcpy(tag, "#");
        strncat(tag, t->word[i], strlen(t->word[i]) - strlen(t->punc[i]));
        strcat(tag, " ");
        strncat(tag, t->word[i + 1], strlen(t->word[i + 1]) - strlen(t->punc[i + 1]));
        strcat(tag, "#");
        if (fe_strpos(abbr2_caps, tag)) goto e84;
        if (F(ZIP) || F(CITY) || F(PERIOD) || F(TOWN) || F(UNIV) || F(PLACE)) {
            fe_strlwr(tag);
            if (fe_strpos(abbr2_lc, tag)) goto e84;
        }
    }
    if (t->ilc[i] == 0) {
        if (t->auc[i] == 0) goto done;
        if (strlen(w) != 2 && (strlen(w) != 3 || !fe_strpos(w, "."))) goto done;
    }
    if (t->auc[i] && (!strcmp(w, "ny") || !strcmp(w, "n.y"))) goto e84;
    if (!any) goto done;
e84:
    if (!strcmp(w, "md") || !strcmp(w, "va")) {
        if (F(ZIP) || F(CITY)) goto accept;
        goto done;
    }
    if (!strcmp(w, "fl")) {
        /* "5 fl oz" */
        if (i + 1 <= t->cur_word && fe_strpos(t->qual[i + 1], "measure") && strcmp(t->word[i + 1], "in")) goto done;
        if (i - 1 >= 1 && !F(CITY) && fe_strpos(t->qual[i - 1], "measure") && strcmp(t->word[i - 1], "in")) goto done;
    }
    if (!strcmp(w, "mi")) {
        if (i + 1 <= t->cur_word && fe_strpos(t->qual[i + 1], "mod")) goto done;
        if (i - 1 >= 1) {
            if (t->num[i - 1]) goto done;
            if (fe_strpos(t->qual[i - 1], "mod")) goto done;
            if (!strcmp(t->qual[i - 1], "which")) goto done;
        }
    }
    if ((t->ilc[i] && !strcmp(w, "nev")) || !strcmp(w, "ne")) {
        if (F(ZIP) || F(CITY) || F(HEADER)) goto accept;
        goto done;
    }
    if (t->auc[i] && !strcmp(w, "mn")) {
        for (short k = -4; k <= 4; k++) {
            int j = i + k;
            if (j >= 1 && j <= t->cur_word && (!strcmp(t->word[j], "Can") || !strcmp(t->word[j], "Canada"))) goto done;
        }
        if (strlen(n1) == 6) goto done;
    }
    if (!strcmp(w, "ca") && t->ilc[i]) {
        /* "ca 1850" is circa */
        if (i + 1 <= t->cur_word && t->num[i + 1] && (short)atoi(n1) <= 2500) goto done;
        if (i + 1 <= t->cur_word && !strcmp(t->qual[i + 1], "bcad")) goto done;
        if (i + 2 <= t->cur_word && !strcmp(t->qual[i + 2], "bcad")) goto done;
        if (i + 3 <= t->cur_word && !strcmp(t->qual[i + 3], "bcad")) goto done;
        if (i - 1 >= 1 && !strcmp(t->qual[i - 1], "bcad")) goto done;
        if (i - 2 >= 1 && !strcmp(t->qual[i - 2], "bcad")) goto done;
        if (i - 3 >= 1 && !strcmp(t->qual[i - 3], "bcad")) goto done;
    }
    if (!strcmp(w, "ark") && i - 1 >= 1 && (!strcmp(p1, "van") || !strcmp(p1, "noah's") || !strcmp(p1, "lost")))
        goto done;
#undef F
accept:
    ok = 1;
done:
    *res = ok;
}
