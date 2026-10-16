/* month / day: month and weekday abbreviations, accepted only in a date context (REFERENCE 3.4a). */
#include <stdlib.h>
#include <string.h>
#include "frontend.h"

static size_t bare_len(const TextProc *t, int j) { return strlen(t->word[j]) - strlen(t->punc[j]); }

/* "Jan 5" -> "5th": the day number after a month becomes an ordinal ("1st", "12th", "05" -> "5th"); a comma inside
   the word ("5,1996") is split off after the suffix. QUAL becomes "ordnl" and NUM is cleared. @0x10057310 */
char *fe_mdo_sfx;

void fe_MonthDay_Ordinal(TextProc *t, short j)
{
    /* The suffix buffer is never initialised; when no suffix applies the original reads what the previous call left
       in that stack slot. It is modelled as static, unless the caller passes the real bytes in fe_mdo_sfx. */
    static char sfx_own[4];
    char *sfx = fe_mdo_sfx ? fe_mdo_sfx : sfx_own;   /* a caller that knows the real bytes passes them */
    char last[2 + 4] = "", tail[64] = "", out[256];
    short n = (short)atoi(t->word[j]), c;
    fe_mdo_sfx = NULL;

    if (n == 0 || n > 31) return;
    c = fe_strpos(t->word[j], ",");
    if (c != 0 && (short)bare_len(t, j) > c) {
        if (c > 3) return;
        out[0] = 0;
        strncat(out, t->word[j], c - 1);
        strcat(out, t->punc[j]);
        strcat(tail, ", ");
        strncat(tail, t->word[j] + c, bare_len(t, j) - c);
        strcpy(t->word[j], out);
    }
    if (fe_fixes) {
        /* the suffix follows the last digit ("30'" -> "30th'", "21a" -> "21sta"); 11-13 take "th" */
        char b[256] = "";
        int d;
        strncat(b, t->word[j], bare_len(t, j));
        for (d = (int)strlen(b); d > 0 && !(b[d - 1] >= '0' && b[d - 1] <= '9'); d--)
            ;
        if (d == 0) return;
        strcpy(sfx, "th");
        if (!(d >= 2 && b[d - 2] == '1')) {
            if (b[d - 1] == '1') strcpy(sfx, "st");
            if (b[d - 1] == '2') strcpy(sfx, "nd");
            if (b[d - 1] == '3') strcpy(sfx, "rd");
        }
        out[0] = 0;
        if (d == 2 && b[0] == '0') strncat(out, b + 1, 1);   /* "05" -> "5th" */
        else strncat(out, b, d);
        strcat(out, sfx);
        strcat(out, b + d);
        goto place;
    }
    strcpy(last, "");
    strncat(last, t->word[j] + bare_len(t, j) - 1, 1);
    if (bare_len(t, j) == 2 && !strncmp(t->word[j], "1", 1)) {
        strcpy(sfx, "th");                   /* 10th .. 19th */
    } else {
        if (!strcmp(last, "1")) strcpy(sfx, "st");
        if (!strcmp(last, "2")) strcpy(sfx, "nd");
        if (!strcmp(last, "3")) strcpy(sfx, "rd");
        if (strcmp(last, "4") >= 0 || !strcmp(last, "0")) strcpy(sfx, "th");
        /* any other last character below '4' ("30'") keeps the previous suffix */
    }
    strcpy(out, "");
    strncat(out, t->word[j], bare_len(t, j));
    if (!strncmp(out, "0", 1) && strlen(out) == 2) {
        strcpy(last, "");
        strcat(last, out + 1);
        strcpy(out, last);
    }
    strcat(out, sfx);
place:
    strcat(out, tail);
    strcat(out, t->punc[j]);
    if (strlen(out) < 30) {
        strcpy(t->word[j], out);
    } else {
        fe_free(t->word[j]);
        t->word[j] = fe_strdup(out);
    }
    strcpy(t->qual[j], "ordnl");
    t->num[j] = 0;
}

/* MonthDay_Ordinal(j) from Rule_MonthDay. Under the harness (fr = Rule_MonthDay's frame) its uninitialised suffix buffer
   gets the real bytes: the 4 at [F-0x8c] (then what the previous call left there), its saved `this` and j. */
static void ordinal(TextProc *t, short j, const unsigned char *fr, char *slot)
{
    char st[16];
    int jj = j;

    if (fr) {
        memcpy(st, slot, 4);
        memcpy(st + 4, &fe_frame_this, 4);
        memcpy(st + 8, &jj, 4);
        st[12] = 0;
        fe_mdo_sfx = st;
    }
    fe_MonthDay_Ordinal(t, j);
    if (fr) memcpy(slot, st, 4);
}

/* "#word#" in lower case, "#z!@z#" outside the text */
static void nb_tag(const TextProc *t, int j, char *out)
{
    strcpy(out, "#");
    if (j < 1 || t->cur_word < j) strcat(out, "Z!@Z");
    else strncat(out, t->word[j], bare_len(t, j));
    strcat(out, "#");
    fe_strlwr(out);
}

static int in(const char *w, const char *const *set)
{
    for (; *set; set++)
        if (!strcmp(w, *set)) return 1;
    return 0;
}

/* the day number of an ordinal word ("5th" -> 5) */
static short ord_value(const char *w)
{
    char buf[256] = "";
    size_t n = strlen(w);
    strncat(buf, w, n >= 2 ? n - 2 : n);
    return (short)atoi(buf);
}

/* a time right after position j: "5 o'clock", "5 pm", "5:30" */
static int time_at(const TextProc *t, int j)
{
    return (t->num[j] || !strcmp(t->qual[j], "timenum")) &&
           (!strcmp(t->word[j + 1], "o'clock") || !strcmp(t->qual[j + 1], "ampm") || !strcmp(t->qual[j + 1], "time"));
}

/* @0x10053880 */
void fe_Rule_MonthDay(TextProc *t, short i, short *res)
{
    static const char months[] = "#january#february#march#april#may#june#july#august#september#october#november#december#";
    static const char caps[] = "#feb#apr#jun#jul#aug#sep#sept#oct#nov#mon#tues#tue#wed#thurs#thur#thu#fri#sa#th#";
    static const char allcaps[] = "#feb#jun#jul#aug#sep#sept#oct#mon#tues#tue#wed#thurs#thur#thu#fri#";
    static const char lower[] = "#feb#jun#jul#aug#sept#nov#mon#tue#tues#thurs#thur#thu#fri#";
    /* the original's list ends in "##", so an empty neighbour word counts as a conjunction; fe_fixes drops it */
    const char *conj = fe_fixes ? "#and#&#nor#or#to#-#through#" : "#and#&#nor#or#to#-#through##";
    static const char ytt[] = "#yesterday#today#tomorrow#";
    static const char approx[] = "#approx#approximately#beginning#date#dates#during#ending#half#in#later#previous#prior#";
    static const char sent[] = "#sent:#throughout#time:#times:#upcoming#coming#";
    static const char parts[] =
        "#morn#morning#afternoon#noon#eve#evening#night#nite#midnight#midnite#holiday#birthday#anniv#anniversary#"
        "edition#issue#";
    static const char at_by[] = "#at#by#before#after#exactly#";
    static const char finance[] =
        "#day#days#index#indices#future#futures#put#puts#call#calls#bill#bills#invoice#invoices#earnings#report#"
        "reports#statement#statements#harvest#harvests#crop#crops#monthly#";
    static const char open_p[] = "#.#,#;#:#(#[#!#?#/#-#&#+#";
    static const char symbols[] = "#$#%###@#";
    static const char on_before[] = "#on#before#after#around#about#";
    static const char seasons[] = "#on#before#after#any#alt#alternate#winter#spring#summer#fall#autumn#";
    static const char events[] =
        "#last#noon#breakfast#lunch#dinner#supper#tv#television#sports#movie#show#opening#closing#prices#hi#high#lo#"
        "low#Dow#S&P#quote#quotation#quotations#index#daily#";
    static const char *const s_jan_dec_sun[] = {"jan", "dec", "sun", 0}, *const s_mar_sat_fr[] = {"mar", "sat", "fr", 0},
                            *const s_auc_mon[] = {"jan", "mar", "apr", "nov", "dec", 0},
                            *const s_auc_day[] = {"tu", "th", "fr", "sat", "sun", "su", 0},
                            *const s_auc_day2[] = {"tu", "th", "fr", "sat", "sa", "sun", "su", 0},
                            *const s_typed_mon[] = {"jan", "mar", "apr", "sep", "oct", "dec", 0},
                            *const s_typed_day[] = {"tu", "wed", "th", "fr", "sat", "sa", "su", 0},
                            *const s_jan_dec_mar[] = {"jan", "dec", "mar", 0}, *const s_sun_fr_sat[] = {"sun", "fr", "sat", 0},
                            *const s_long[] = {"march", "april", "may", "june", "august", 0},
                            *const s_of_what[] = {"all", "beginning", "end", "half", "part", 0};
    short ok = *res;
    const char *w = t->word_lc, *typed = t->word_as_typed;
    int cur = t->cur_word;
    char tag[128], buf[256], prv[256], nxt[256], prv2[256], nxt2[256];
    short O_[5], W_[5], M_[5], *O = O_ + 2, *W = W_ + 2, *M = M_ + 2;   /* ordinal / weekday / month at i-2 .. i+2 */
    int b1 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0, b8 = 0, b9 = 0, b10 = 0, b11 = 0, b12, bfin, bday = 0, bseas, bev;
    int any3, any4;
    short v;

    strcpy(tag, "#");
    strcat(tag, w);
    strcat(tag, "#");
    if (fe_strpos(months, tag) || (fe_strpos(caps, tag) && t->iuc[i]) || (fe_strpos(allcaps, tag) && t->auc[i]))
        goto accept;
    if (fe_strpos(lower, tag) && !strncmp(w, t->word[i], bare_len(t, i))) goto accept;
    if ((!strcmp(w, "tu") || !strcmp(w, "su")) && t->iuc[i] && i + 2 <= cur &&
        (t->num[i + 1] == 0 || !strcmp(t->qual[i + 2], "month")))
        goto accept;

    for (int k = -2; k <= 2; k++) {
        int j = i + k;
        O[k] = W[k] = M[k] = 0;
        if (j < 1 || cur < j) continue;
        if (!strcmp(t->qual[j], "ordnl")) {
            v = ord_value(t->word[j]);
            if (v > 0 && v < 32) O[k] = 1;
        }
        if (!strcmp(t->qual[j], "month")) M[k] = 1;
        buf[0] = 0;
        strncat(buf, t->word[j], bare_len(t, j));
        fe_strlwr(buf);
        if (!strcmp(t->qual[j], "weekday") || (strlen(buf) != 0 && !strncmp(buf, "monday", strlen(buf)))) W[k] = 1;
    }
    nb_tag(t, i - 1, prv);
    nb_tag(t, i + 1, nxt);
    nb_tag(t, i - 2, prv2);
    nb_tag(t, i + 2, nxt2);

    /* "Mon. the" ... a sentence does not end here */
    if (i + 1 <= cur && !strcmp(t->punc[i], ".") && t->ilc[i + 1]) b1 = 1;
    /* an ordinal day next to it: "Jan 5th", "Jan the 5th", "5th of Jan" */
    if (i + 1 <= cur) {
        if (O[1] || (!strcmp(t->word[i + 1], "the") && O[2])) b3 = 1;
    }
    if (!b3 && i - 1 > 0) {
        if (O[-1] || (!strcmp(t->word[i - 1], "of") && O[-2])) b3 = 1;
    }
    /* month and weekday next to each other: "Mon Jan", "Jan Feb" */
    if ((M[0] && W[-1]) || (W[0] && M[1])) {
        b4 = 1;
        if (!strcmp(w, "may") && (!strcmp(prv, "#sun#") || !strcmp(prv, "#we#"))) b4 = 0;
        if (!strcmp(nxt, "#may#") && (!strcmp(w, "sun") || !strcmp(w, "we"))) b4 = 0;
    }
    if (M[0] && (M[-1] || M[1])) b4 = 1;
    if (W[0] && (W[-1] || W[1])) b4 = 1;
    /* "Mon 5th Jan", "Jan Mon 5th" */
    if ((M[0] && W[-2] && O[-1]) || (W[0] && M[2] && O[1])) b5 = 1;
    if (i + 1 <= cur && W[0] && M[2] && !strcmp(t->qual[i + 1], "day")) b5 = 1;
    else if (i - 1 >= 1 && M[0] && W[-2] && !strcmp(t->qual[i - 1], "day")) b5 = 1;
    /* lists and ranges: "Jan and Feb", "Jan-Mar", "Mon to Fri", "Mon and today" */
    if (M[0] && ((fe_strpos(conj, nxt) && M[2]) || ((!strcmp(t->sepr[i], "-") || !strcmp(t->sepr[i], ",")) && M[1])))
        b6 = 1;
    if (i - 1 > 0 && M[0] &&
        ((fe_strpos(conj, prv) && M[-2]) || ((!strcmp(t->sepr[i - 1], "-") || !strcmp(t->sepr[i - 1], ",")) && M[-1])))
        b6 = 1;
    if (W[0] && ((fe_strpos(conj, nxt) && W[2]) || ((!strcmp(t->sepr[i], "-") || !strcmp(t->sepr[i], ",")) && W[1])))
        b6 = 1;
    if (i - 1 >= 1 && W[0] &&
        ((fe_strpos(conj, prv) && W[-2]) || ((!strcmp(t->sepr[i - 1], "-") || !strcmp(t->sepr[i - 1], ",")) && W[-1])))
        b6 = 1;
    if (W[0] && fe_strpos(conj, nxt) && fe_strpos(ytt, nxt2)) b6 = 1;
    if (W[0] && fe_strpos(conj, prv) && fe_strpos(ytt, prv2)) b6 = 1;
    /* "jan" next to a long month name is a name ("Jan and Robert") */
    if (!strcmp(w, "jan")) {
        if (i + 2 <= cur && M[2] && strlen(t->word[i + 2]) >= 4) b6 = 0;
        else if (i - 2 > 0 && M[-2] && strlen(t->word[i - 2]) > 3) b6 = 0;
    }
    /* a time next to it: "5 pm Mon", "Mon 5 pm", "Mon at 5 pm" */
    if (i - 2 > 0 && time_at(t, i - 2)) b7 = 1;
    if (i + 2 <= cur && time_at(t, i + 1)) b7 = 1;
    if (i + 3 <= cur && (fe_strpos(at_by, nxt) || t->num[i + 1] || !strcmp(t->qual[i + 1], "ordnl")) && time_at(t, i + 2))
        b7 = 1;
    /* a year: "Jan 1996", "Jan 55 BC", "Jan '96" */
    if (i + 2 <= cur && (!strcmp(t->qual[i + 1], "year") || (t->num[i + 1] && !strcmp(t->qual[i + 2], "bcad")))) b8 = 1;
    if (i + 1 <= cur && M[0] && !strncmp(t->word[i + 1], "'", 1) && strlen(t->word[i + 1]) == 3 &&
        atoi(t->word[i + 1] + 1) != 0)
        b8 = 1;
    /* "5/Jan/96" */
    if (i - 1 > 0 && !strcmp(t->sepr[i - 1], "/") && !strcmp(t->sepr[i], "/")) b9 = 1;
    /* a date word before it: "during Jan", "beginning of Jan", "prior to Mon" */
    if (i - 1 > 0 && strlen(t->punc[i - 1]) == 0 && (fe_strpos(approx, prv) || fe_strpos(sent, prv))) b10 = 1;
    if (i - 2 > 0) {
        if (!strcmp(t->word[i - 1], "of") && in(t->word[i - 2], s_of_what)) b10 = 1;
        if (!strcmp(t->word[i - 1], "to") && !strcmp(t->word[i - 2], "prior")) b10 = 1;
    }
    /* "Mon morning", "Sat clearance" */
    if (fe_strpos(parts, nxt) || fe_strpos("#clearance#", nxt)) b11 = 1;

    any3 = b3 || b4 || b5 || b6 || b7 || b8 || b9 || b10 || b11;
    any4 = b4 || b5 || b6 || b7 || b8 || b9 || b10 || b11;
    if (t->iuc[i] && in(w, s_jan_dec_sun) && (b1 || any3)) goto accept;
    if (t->iuc[i] && in(w, s_mar_sat_fr) && any3) goto accept;
    if ((t->iuc[i] || t->auc[i]) ? (!strcmp(w, "we") || !strcmp(w, "mo")) : !strcmp(w, "mo"))
        if (any4) goto accept;
    if (t->auc[i] && in(w, s_auc_mon) && any3) goto accept;
    if (t->auc[i] && in(w, s_auc_day) && any3) goto accept;
    if (in(typed, s_typed_mon) && any3) goto accept;
    if (in(typed, s_typed_day) && any3) goto accept;

    /* "in Jan", "of the week of Mon" */
    b12 = !strcmp(prv, "#in#") || !strcmp(prv, "#within#") ||
          (!strcmp(prv, "#of#") && (!strcmp(prv2, "#week#") || !strcmp(prv2, "#day#") || !strcmp(prv2, "#month#")));
    /* "Jan futures", "Mar calls" */
    bfin = fe_strpos(finance, nxt) != 0;
    /* a day number before it: "on 5 Jan", "5 Jan." at the end, "5 Jan 1996" */
    v = 99;
    if (i - 1 > 0) {
        if (!strcmp(t->qual[i - 1], "ordnl")) v = ord_value(t->word[i - 1]);
        if (t->num[i - 1] && (!strcmp(t->qual[i - 1], "day") || (v > 0 && v < 32))) {
            if (!strcmp(nxt, "##")) strcpy(nxt, "#Z!@Z#");
            if (!strcmp(prv2, "##")) strcpy(prv2, "#Z!@Z#");
            bday = cur == i || !strcmp(prv2, "#on#");
            if ((strcmp(t->punc[i], "") || strcmp(t->sepr[i], "")) && !fe_strpos(symbols, nxt) && !fe_strpos(symbols, prv2))
                bday = 1;
            if (i + 1 <= cur &&
                (!strncmp(t->prepunc[i + 1], "(", 1) || !strncmp(t->word[i + 1], "[", 1) || fe_strpos(open_p, nxt)) &&
                !fe_strpos(symbols, nxt) && !fe_strpos(symbols, prv2))
                bday = 1;
            if (i + 1 <= cur && !strcmp(t->qual[i + 1], "year")) bday = 1;
        }
    }
    /* a day number after it: "on Jan 5", "Jan 5." at the end, "Jan 5 1996" */
    v = 99;
    if (i + 1 <= cur && !strcmp(t->qual[i + 1], "ordnl")) v = ord_value(t->word[i + 1]);
    if (i + 1 <= cur && t->num[i + 1] && (!strcmp(t->qual[i + 1], "day") || (v > 0 && v < 32))) {
        if (!strcmp(nxt2, "##")) strcpy(nxt2, "#Z!@Z#");
        if (!strcmp(prv, "##")) strcpy(prv, "#Z!@Z#");
        if (fe_strpos(on_before, prv)) bday = 1;
        if (cur - i == 1) bday = 1;
        if ((strcmp(t->punc[i + 1], "") || strcmp(t->sepr[i + 1], "") || fe_strpos(open_p, nxt2)) &&
            !fe_strpos(symbols, nxt2) && !fe_strpos(symbols, prv))
            bday = 1;
        if (i + 2 <= cur && (!strncmp(t->prepunc[i + 2], "(", 1) || !strncmp(t->word[i + 2], "[", 1)) &&
            !fe_strpos(symbols, nxt2) && !fe_strpos(symbols, prv))
            bday = 1;
        if (i + 2 <= cur && !strcmp(t->qual[i + 2], "year")) bday = 1;
    }
    bseas = fe_strpos(seasons, prv) != 0;
    bev = fe_strpos(events, nxt) != 0;

    if (t->iuc[i] && in(w, s_jan_dec_mar) && (b12 || bfin || bday)) goto accept;
    if (t->auc[i] && in(w, s_auc_mon) && (b12 || bfin || bday)) goto accept;
    if (in(typed, s_typed_mon) && (b12 || bfin || bday)) goto accept;
    if (t->iuc[i] && in(w, s_sun_fr_sat) && (bseas || bev)) goto accept;
    if (t->auc[i] && in(w, s_auc_day2) && (bseas || bev)) goto accept;
    if (in(typed, s_typed_day) && (bseas || bev)) goto accept;
    if (t->auc[i] && in(w, s_long)) goto accept;
    if (in(w, s_long) && (any3 || bday)) goto accept;
    goto dec;

accept:
    ok = 1;
dec:
    /* "dec" as an angle: 5 dec 30' */
    if (!strcmp(w, "dec") &&
        (!strcmp(t->sepr[i], "'") || !strcmp(t->sepr[i], "\"") ||
         (i + 1 <= cur && (!strncmp(t->word[i + 1], "deg", 3) || !strncmp(t->word[i + 1], "min", 3) ||
                           !strncmp(t->word[i + 1], "sec", 3)))))
        ok = 0;
    /* an accepted month turns the day numbers after it into ordinals: "Jan 5" -> "Jan 5th", also "Jan 5-7", "Jan 5 to 7" */
    if (i < cur && ok && !strcmp(t->qual[i], "month") && t->num[i + 1] && strcmp(t->qual[i + 1], "ordnl")) {
        const unsigned char *fr = fe_frame_of(0x10053880);
        char slot[4] = {0};
        if (fr) memcpy(slot, fr - 0x8c, 4);
        ordinal(t, i + 1, fr, slot);
        if (i + 2 <= cur && t->num[i + 2] && !strcmp(t->sepr[i + 1], "-") && strcmp(t->qual[i + 2], "ordnl"))
            ordinal(t, i + 2, fr, slot);
        else if (i + 3 <= cur && t->num[i + 3] && (!strcmp(t->word[i + 2], "to") || !strcmp(t->word[i + 2], "-")) &&
                 strcmp(t->qual[i + 3], "ordnl"))
            ordinal(t, i + 3, fr, slot);
    }
    *res = ok;
}
