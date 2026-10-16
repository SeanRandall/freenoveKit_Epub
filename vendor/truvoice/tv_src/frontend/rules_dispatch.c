/* Abbreviation-rule dispatcher (patent 148). REFERENCE 3.4. */
#include <string.h>
#include "frontend.h"

/* One string; a rule is selected by where its name first occurs in it (strpos), so e.g. "direction1" is found
   at 164 but "direction" at 137, and an unknown name falls to the default case. */
static const char rule_names[] =
    "measure1measure2measure3measure4measure5title1title2title3title4state1state2state3state5state6citystreet"
    "qual1qual2qual3qual4monthdaytimedirectiongenlrangebcadfnamedirection1direction2direction3title5ampm";

/* Fix (REFERENCE 3.4c, stale pointer): the rules read word[i+1], qual[i+1], num[i+1] ... at the last word of the
   paragraph without checking cur_word, and the original then sees whatever an earlier paragraph left in the slot.
   With fe_fixes the two slots after the last word are empty words (empty strings, zero flags) while a rule runs;
   their previous contents are put back afterwards. */
#define GUARD 2
typedef struct {
    int n;
    char *s[5];
    int16_t f[8];
    char buf[5][256];
} Guard;

static int16_t *flag(TextProc *t, int k, int n)
{
    int16_t *a[8] = {t->fper, t->ilc, t->iuc, t->auc, t->num, t->stprov, t->psnum, t->sep_dash};
    return &a[k][n];
}
static char **slot(TextProc *t, int k, int n)
{
    char **a[5] = {t->word, t->qual, t->punc, t->sepr, t->prepunc};
    return &a[k][n];
}
static void guard_on(TextProc *t, Guard *g)
{
    for (int m = 0; m < GUARD; m++) {
        Guard *h = &g[m];
        h->n = t->cur_word + 1 + m;
        if (h->n >= FE_MAXWORDS) continue;
        for (int k = 0; k < 5; k++) {
            h->s[k] = *slot(t, k, h->n);
            memset(h->buf[k], 0, sizeof h->buf[k]);
            *slot(t, k, h->n) = h->buf[k];
        }
        for (int k = 0; k < 8; k++) {
            h->f[k] = *flag(t, k, h->n);
            *flag(t, k, h->n) = 0;
        }
    }
}
static void guard_off(TextProc *t, Guard *g)
{
    for (int m = 0; m < GUARD; m++) {
        Guard *h = &g[m];
        if (h->n >= FE_MAXWORDS) continue;
        for (int k = 0; k < 5; k++) *slot(t, k, h->n) = h->s[k];
        for (int k = 0; k < 8; k++) *flag(t, k, h->n) = h->f[k];
    }
}

static void dispatch(TextProc *t, short i, short *res);

/* @0x10045df0 */
void fe_AbbrevRuleDispatch(TextProc *t, short i, short *res)
{
    Guard g[GUARD];

    (void)fe_frame_of(0x10045df0);   /* harness: in a replay, Rule_MonthDay's frame follows this call's record */
    if (!fe_fixes) {
        dispatch(t, i, res);
        return;
    }
    guard_on(t, g);
    dispatch(t, i, res);
    guard_off(t, g);
}

static void dispatch(TextProc *t, short i, short *res)
{
    char names[sizeof rule_names];
    short ok = 0;

    memcpy(names, rule_names, sizeof names);
    strcpy(t->measure_sfx, "");
    switch (fe_strpos(names, t->rule_type)) {
    case 1:   fe_Rule_Measure1(t, i, &ok); break;
    case 9:   fe_Rule_Measure2(t, i, &ok); break;
    case 17:  fe_Rule_Measure3(t, i, &ok); break;
    case 25:  fe_Rule_Measure4(t, i, &ok); break;
    case 33:  fe_Rule_Measure5(t, i, &ok); break;
    case 41: case 47: case 53: case 59:
              fe_Rule_Title1234(t, i, &ok); break;
    case 65: case 71: case 77:
              fe_Rule_State123(t, i, &ok); break;
    case 83: case 89:
              fe_Rule_State56(t, i, &ok); break;
    case 95:  fe_Rule_City(t, i, &ok); break;
    case 99:  fe_Rule_Street(t, i, &ok); break;
    case 105: fe_Rule_Qual1(t, i, &ok); break;
    case 110: fe_Rule_Qual2(t, i, &ok); break;
    case 115: fe_Rule_Qual3(t, i, &ok); break;
    case 120: fe_Rule_Qual4(t, i, &ok); break;
    case 125: case 130:                          /* month, day */
              fe_Rule_MonthDay(t, i, &ok); break;
    case 133: fe_Rule_Time(t, i, &ok); break;
    case 137: fe_Rule_Direction(t, i, &ok); break;
    case 146: case 200:                          /* genl, ampm */
              fe_Rule_GenlAmpm(t, i, &ok); break;
    case 150: fe_Rule_Range(t, i, &ok); break;
    case 155: fe_Rule_Bcad(t, i, &ok); break;
    case 159: fe_Rule_Fname(t, i, &ok); break;
    case 164: fe_Rule_Direction1(t, i, &ok); break;
    case 174: fe_Rule_Direction2(t, i, &ok); break;
    case 184: fe_Rule_Direction3(t, i, &ok); break;
    case 194: fe_Rule_Title5(t, i, &ok); break;
    default:  ok = 1; break;                     /* no condition: always accepted */
    }

    if (ok) {
        strcpy(t->out_text, t->rec_expansion);
        if (fe_strpos(t->rule_type, "measure") && strcmp(t->measure_sfx, "") != 0)
            fe_Rule_MeasureModifiers(t, i);
        /* "5 ft/sec": a measure followed by '/' reads "per" unless the next word already says so */
        if (fe_strpos(t->rule_type, "measure") && strcmp(t->sepr[i], "/") == 0 && !fe_strpos(t->out_text, "per") &&
            (t->cur_word < i + 1 ||
             (strncmp(t->word[i + 1], "per", 3) != 0 && !fe_strpos(t->qual[i + 1], "mod") &&
              !fe_strpos(t->qual[i + 1], "qual"))))
            strcat(t->out_text, " per");
        if (strcmp(t->rec_expansion, "") != 0)
            fe_Rule_Context_StreetTitleCity(t, i, ok);
        strcpy(t->accepted_text, t->out_text);
    }
    *res = ok;
}
