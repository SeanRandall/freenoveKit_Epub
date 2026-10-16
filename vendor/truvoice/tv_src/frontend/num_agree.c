/* Number stage: does "n - m" / "n m" form a range or an agreeing pair (patent 142; REFERENCE 3.8, 8.4). */
#include <string.h>
#include <stdlib.h>
#include "frontend.h"
#ifdef _WIN32
#include <windows.h>
#endif

#pragma GCC diagnostic ignored "-Wrestrict"   /* the work buffers share one frame array */

/* The work buffers keep their frame offsets: the last-character tests read the byte in front of an empty buffer,
   and long words run on into the next buffer. SLACK keeps the port's own overruns inside the array. */
#define FRAME 0x274
#define SLACK 0x1000
#define F14 (F + 0x14)    /* 1-2 characters for atoi */
#define F1C (F + 0x1c)    /* sepr of the next number (4 bytes) */
#define F20 (F + 0x20)    /* a QUAL being looked up (12) */
#define F2C (F + 0x2c)    /* QUAL of the word after the next number (12) */
#define F38 (F + 0x38)    /* QUAL of the next number (12) */
#define F44 (F + 0x44)    /* word i, and scratch (60) */
#define F80 (F + 0x80)    /* the next number, without trailing .,;:?! (60) */
#define FBC (F + 0xbc)    /* QUAL two words on (16) */
#define FCC (F + 0xcc)    /* the word after the next number (60) */
#define F108 (F + 0x108)
#define F120 (F + 0x120)  /* the next number as typed (60) */
#define F15C (F + 0x15c)
#define F18C (F + 0x18c)
#define F200 (F + 0x200)

/* A slot past cur_word can hold any value; the port does not follow one the original would fault on. */
static int readable(const char *p)
{
#ifdef _WIN32
    return p && !IsBadReadPtr(p, 1);
#else
    return p != NULL;
#endif
}

/* s + strlen(s) - 1: an empty s gives the byte in front of it [original] */
static const char *last(const char *s) { return fe_tail(s, 1); }

/* Drop trailing .,;:?! from buf (F44 is the scratch buffer). The original takes the last character of an empty buf
   from the byte in front of it and loops forever when that byte is one of the marks; the port stops. */
static void strip_marks(char *F, char *buf)
{
    for (;;) {
        if (!*buf) return;
        F14[0] = buf[strlen(buf) - 1];
        F14[1] = 0;
        if (!strchr(".,;:?!", F14[0])) return;
        F44[0] = 0;
        strncat(F44, buf, strlen(buf) - 1);
        strcpy(buf, F44);
    }
}

/* Is the number word i the start of a range or pair with the next number ("5-10 miles", "1995-96", "5 - 10'",
   "3-5 pm", "500-300 BC", "Jan 5-10", "1/2-3/4", "5 - 10 people")? *res = 1 when it is; a "-" word between the two
   is then emptied. Mostly the second number must be larger than the first. @0x1003cd70 */
void fe_Num_Agreement(TextProc *t, short i, short *res)
{
    unsigned char Fb[FRAME + SLACK];
    char *F = (char *)Fb;
    const unsigned char *fr = fe_frame_of(0x1003cd70);
    short r = *res, nn = 0, si, ax;
    int n = i;

    if (fr) memcpy(Fb, fr, FRAME); else memset(Fb, 0, FRAME);
    memset(Fb + FRAME, 0, SLACK);
    memcpy(F108, "monthprerangemeasure", 21);
    memcpy(F15C, "ampmbcadnumodpremodppmodmeasurepstrangemonth", 45);
    memcpy(F18C, "#his#its#us#has#is#was#does#as#goes#themselves#yourselves#unless#whereas#across#address#compress#"
                 "recess#skis#says#", 115);
    memcpy(F200, "#people#children#men#women#feet#teeth#mice#geese#cacti#stimuli#criteria#data#bacteria#phenomena#"
                 "deer#fish#sheep#", 113);
    F80[0] = F120[0] = FCC[0] = F38[0] = F2C[0] = FBC[0] = F1C[0] = 0;
    if (t->cur_word < n + 1) return;

    /* the next number, skipping a "-" word */
    if (strcmp(t->word[n + 1], "-")) {
        strcpy(F80, t->word[n + 1]);
        strcpy(F120, t->word[n + 1]);
        strcpy(F38, t->qual[n + 1]);
        strcpy(F1C, t->sepr[n + 1]);
        if (t->cur_word >= n + 2) {
            strcpy(FCC, t->word[n + 2]);
            strcpy(F2C, t->qual[n + 2]);
            nn = t->num[n + 2];
            if (n + 3 <= t->cur_word) strcpy(FBC, t->qual[n + 3]);
        }
    } else if (t->cur_word >= n + 2) {
        strcpy(F80, t->word[n + 2]);
        strcpy(F120, t->word[n + 2]);
        strcpy(F38, t->qual[n + 2]);
        strcpy(F1C, t->sepr[n + 2]);
        if (n + 3 <= t->cur_word) {
            strcpy(FCC, t->word[n + 3]);
            strcpy(F2C, t->qual[n + 3]);
            nn = t->num[n + 3];
            if (n + 4 <= t->cur_word) strcpy(FBC, t->qual[n + 4]);
        }
    }
    strip_marks(F, F80);
    F44[0] = 0;
    strcat(F44, t->word[n]);

    /* years: "1995-2001", "1995-96", "1995-6" */
    if (!strcmp(t->qual[n], "year")) {
        if (!strcmp(F38, "year")) {
            if ((short)atoi(F80) > (short)atoi(F44)) goto yes;
            goto done;
        }
        if (strlen(F80) == 2) {
            F14[0] = 0;
            strcat(F14, fe_tail(F44, 2));
            if ((short)atoi(F80) > (short)atoi(F14)) goto yes;
            goto done;
        }
        if (strlen(F80) == 1) {
            F14[0] = *last(F44);
            F14[1] = 0;
            if ((short)atoi(F80) > (short)atoi(F14)) goto yes;
            goto done;
        }
    }

    /* after a month ("Jan 5-10"; two days: QUAL day), "pre", range or measure word */
    if (n - 1 >= 1 && (t->cur_word < n + 2 || t->num[n + 2] == 0) && strcmp(t->qual[n - 1], "")) {
        strcpy(F20, t->qual[n - 1]);
        if (!strncmp(F20, "measure", 7)) strcpy(F20, "measure");
        switch (fe_strpos(F108, F20)) {
        case 1:
            if (!strcmp(t->qual[n], "day") && !strcmp(F38, "day")) goto compare;
            break;
        case 6: case 9: case 14:
            goto compare;
        }
    }
    /* before am/pm (hours 1-12), BC (either order below 2100), or a modifier, measure, range or month word */
    if (t->cur_word >= n + 2 && strcmp(F2C, "")) {
        strcpy(F20, F2C);
        if (!strncmp(F20, "measure", 7)) strcpy(F20, "measure");
        switch (fe_strpos(F15C, F20)) {
        case 1:
            F14[0] = *last(F44);
            F14[1] = 0;
            si = (short)atoi(F14);
            ax = (short)atoi(F80);
            if (ax > si) {
                if (ax <= 12 && ax >= 1 && si >= 1 && si <= 12) goto yes;
                break;
            }
            /* a descending am/pm pair goes on as for "bc" [original] */
            /* fall through */
        case 5: {
            short di;
            F14[0] = *last(F44);
            F14[1] = 0;
            si = (short)atoi(F14);
            di = (short)atoi(F80);
            if (!strncmp(FCC, "b", 1) || !strncmp(FCC, "B", 1)) {
                if (di < si) goto yes;
                if (di > si && di < 2100 && si < 2100) goto yes;
            }
            break;
        }
        case 9: case 14: case 20: case 25: case 32: case 35: case 40:
            goto compare;
        }
    }

    /* feet and inches: 5' - 10', 5" - 10", 5' - 6" */
    if (!strcmp(last(F80), "'") || !strcmp(last(F80), "\"")) {
        if (strcmp(last(F44), "\"") && strcmp(last(F44), "'")) goto marks;
        if (!strcmp(last(F44), last(F80))) goto marks;
        if (strcmp(last(F44), "\"")) goto fractions;
        if (!strcmp(last(F80), "'")) goto yes;
    }
fractions:
    /* fractions: "1/2-3/4" */
    if (n - 1 >= 1 && t->cur_word >= n + 1 && t->num[n - 1] && !strcmp(t->sepr[n - 1], "/") && strlen(t->word[n - 1]) <= 2) {
        if (strcmp(F1C, "/")) goto yes;
        if (t->cur_word >= n + 2 && !strcmp(F1C, "/") && nn && strlen(F120) <= 2 && strlen(FCC) <= 2) goto yes;
    }
    /* a plural noun after the second number: -s words except the exception list, and the irregular plurals */
    strip_marks(F, FCC);
    strcpy(F44, "#");
    strcat(F44, FCC);
    strcat(F44, "#");
    /* With no word after the second number, the original reads the stack byte in front of FCC, where the DLL finds
       an 's' [likely]: a range at the end of a paragraph is read as one. fe_fixes makes that the rule. */
    F14[0] = fe_fixes && !*FCC ? 's' : *last(FCC);
    F14[1] = 0;
    if (F14[0] == 's' && !fe_strpos(F18C, F44)) goto compare;   /* F44 is now "#word#": atoi gives 0 */
    if (!fe_strpos(F200, F44)) goto done;
    goto compare;

marks:
    /* both numbers carry the same mark (or the first none): drop it before comparing */
    if (!strcmp(last(F80), "\"") || !strcmp(last(F80), "'")) {
        F44[0] = 0;
        strncat(F44, F80, strlen(F80) - 1);
        strcpy(F80, F44);
        F44[0] = 0;
        strcat(F44, t->word[n]);
    }
    if (!strcmp(last(F44), "\"") || !strcmp(last(F44), "'")) {
        F44[0] = 0;
        strncat(F44, t->word[n], strlen(t->word[n]) - 1);
    }

compare:
    si = (short)atoi(F44);
    ax = (short)atoi(F80);
    if (ax <= si) goto done;
    strcpy(F38, t->qual[n]);
    strcpy(F2C, t->qual[n + 1]);
    if (!strcmp(t->word[n + 1], "-")) {
        /* qual[i+2] is read even when word i+2 does not exist [original] */
        if (n + 2 > t->cur_word && (fe_fixes || !readable(t->qual[n + 2]))) F2C[0] = 0;
        else strcpy(F2C, t->qual[n + 2]);
    }
    /* a Roman numeral counts only before another one */
    if (strcmp(F38, "roman")) goto yes;
    if (strcmp(F2C, "roman")) goto done;
yes:
    r = 1;
done:
    if (r && !strcmp(t->word[n + 1], "-")) strcpy(t->word[n + 1], "");
    *res = r;
}
