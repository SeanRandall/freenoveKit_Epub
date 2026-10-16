/*
 * C port of TV_ENG32.DLL's email front end (patent US 5,634,084). Function names follow the Ghidra project;
 * each definition carries the VA of the original. REFERENCE.md 3.4 / 3.4a describe what the rules do.
 */
#ifndef FE_FRONTEND_H
#define FE_FRONTEND_H

#include <stddef.h>
#include "textproc.h"

/* ---- bug fixes ---------------------------------------------------------------------------------------------- */
/* 1 (the default): the defects of the original listed in REFERENCE 3.4c are corrected. 0: the port behaves exactly as
   the original, stale reads included; tools/fediff uses this to check it against the DLL. Every place that differs
   is marked "if (fe_fixes)". */
extern int fe_fixes;

/* The last n characters of s. The original computes s + strlen(s) - n even when s is shorter and reads the bytes in
   front of it; with fe_fixes a shorter s gives "". */
#include <string.h>
static inline const char *fe_tail(const char *s, int n)
{
    int l = (int)strlen(s);
    return fe_fixes && l < n ? s + l : s + l - n;
}

/* ---- runtime (fe_crt.c) ------------------------------------------------------------------------------------ */
/* Allocation goes through hooks so the test harness can use the DLL's own heap. */
extern void *(*fe_malloc_hook)(size_t);
extern void (*fe_free_hook)(void *);
void *fe_malloc(size_t n);
void fe_free(void *p);
char *fe_strdup(const char *s);

/* The DLL's statically linked MSVC CRT, "C" locale. */
short fe_strpos(const char *hay, const char *needle);   /* 100446e0: 1-based position of needle, 0 if absent */
char *fe_strlwr(char *s);                               /* A-Z only */
char *fe_strupr(char *s);
char *fe_str_trim_spaces(char *s);                      /* 10044460 */
void fe_str_replace_all(const char *src, const char *find, const char *repl, char *dst); /* 10044560 */
#define fe_strcmp strcmp
#define fe_strncmp strncmp
#define fe_strncat strncat
#define fe_strncpy strncpy
#define fe_strrchr strrchr
#define fe_atoi atoi

/* ---- rule dispatcher and handlers ---------------------------------------------------------------------------- */
typedef void FeRule(TextProc *t, short i, short *res);

void fe_AbbrevRuleDispatch(TextProc *t, short i, short *res);   /* 10045df0 */
FeRule fe_Rule_Measure1, fe_Rule_Measure2, fe_Rule_Measure3, fe_Rule_Measure4, fe_Rule_Measure5;
FeRule fe_Rule_Title1234, fe_Rule_Title5, fe_Rule_State123, fe_Rule_State56, fe_Rule_City, fe_Rule_Street;
FeRule fe_Rule_Qual1, fe_Rule_Qual2, fe_Rule_Qual3, fe_Rule_Qual4, fe_Rule_MonthDay, fe_Rule_Time;
FeRule fe_Rule_Direction, fe_Rule_Direction1, fe_Rule_Direction2, fe_Rule_Direction3, fe_Rule_GenlAmpm;
FeRule fe_Rule_Range, fe_Rule_Bcad, fe_Rule_Fname;

/* helpers */
void fe_Rule_MeasureModifiers(TextProc *t, short i);                       /* 1004af00 */
void fe_Rule_Context_StreetTitleCity(TextProc *t, short i, short res);     /* 10047410 */
void fe_Measure_Accept(TextProc *t, short i, short *res);                  /* 10048460 */
void fe_Measure_Finalize(TextProc *t, short i, short *res);                /* 10048910 */
void fe_Qual_PerSlash(TextProc *t, short i);                               /* 10053680 */
void fe_IsPersonName(TextProc *t, short i, short *res);                    /* 100586c0 */
void fe_Range_ItemCheck(TextProc *t, short j, char *out);                  /* 1004bed0 */
void fe_Num_SayDigitGroups(const char *s, char *out);                      /* 1003b6f0 */
void fe_MonthDay_Ordinal(TextProc *t, short j);                            /* 10057310 */
extern char *fe_mdo_sfx;   /* harness only: MonthDay_Ordinal's uninitialised suffix buffer for the next call */
void fe_CityState_LetterRange(TextProc *t, short *start, short *end);        /* 100402f0 */
void fe_Street_ArticleCheck(TextProc *t, short i, short *res);             /* 10052ee0 */
void fe_State_Helper_A(TextProc *t, short i, short *res);                  /* 10069bd0 */
void fe_State_Helper_B(TextProc *t, short i, short *res);                  /* 10069640 */

/* ---- classifier (classify.c) --------------------------------------------------------------------------------- */
void fe_ClassifyWord(TextProc *t);                                         /* 100330f0 */
extern const unsigned char *fe_cw_stack;    /* harness only: the original's frame bytes at entry, else NULL */
void fe_Abbrev_LetterRange(TextProc *t, short *start, short *end);         /* 10040280 */

/* ---- tokenizer (tokenize.c) ---------------------------------------------------------------------------------- */
extern short fe_emph_state;                                                /* 0x10137074 */
extern const unsigned char *fe_tok_stack;   /* harness only: initial frame bytes for Tokenize_FirstPass, else NULL */
short fe_Tokenize_FirstPass(TextProc *t, short pos, short *end);           /* 1002e800 */
void fe_Tokenize_Dots(TextProc *t);                                        /* 1003fe90 */
void fe_Tokenize_SplitSeparators(TextProc *t, short pos, short *end, char *sep); /* 1002e410 */

/* ---- number stage (num_*.c; patent 142, REFERENCE 3.8, 8.4) ---------------------------------------------------- */
/* Called by the paragraph driver: */
void fe_NumberExpander(TextProc *t, short i, short *a, short *b);          /* 10035940 */
void fe_Num_YearsRanges(TextProc *t, short i, short *res);                 /* 1003df70 (also from NumberExpander) */
void fe_Num_QualContext(TextProc *t, short i, short *res);                 /* 1003f2a0 */
void fe_Num_Roman(TextProc *t, short i);                                   /* 10040760 */
void fe_Num_Time(TextProc *t, short i);                                    /* 10041230 */
void fe_Num_Money(TextProc *t, short i);                                   /* 10041800 */
void fe_Classify_PeriodContext(TextProc *t, short i);                      /* 100403a0 */
/* Called by NumberExpander: */
void fe_Num_DatesFractionsOrdinals(TextProc *t, short i, char *next);      /* 100370f0 */
void fe_Num_Date_MDY(TextProc *t, short i, short *res);                        /* 1003a1e0 */
void fe_Num_Phone(TextProc *t, short i, short *res);                       /* 1003a690 */
void fe_Num_PhoneContext(TextProc *t, short i, short *res);                /* 1003aec0 */
void fe_Num_DigitString(TextProc *t, short i, short *res);                 /* 1003c130 */
void fe_Num_SpecialNumbers(TextProc *t, short i, short *res);              /* 1003c6b0 */
void fe_Num_Agreement(TextProc *t, short i, short *res);                   /* 1003cd70 */
/* Called by Num_Phone (signatures are set by their port): Num_Phone_Group 1003b4e0, Num_Phone_Extension 1003bcf0. */

/* ---- acronym stage (acr_*.c; patent 150, REFERENCE 3.9, 8.4) ---------------------------------------------------- */
/* Argument names are provisional until each function is ported. Called by the paragraph driver: */
void fe_Acronym_MilitaryRating(TextProc *t, short i, short *res);          /* 100502f0 */
void fe_AcronymInitialismExpander(TextProc *t, short i, short *res);       /* 10013460 */
/* Called by AcronymInitialismExpander: */
void fe_Acronym_KnownLists(TextProc *t, short i, short *res);              /* 10016ae0 */
void fe_StripMorphs_A(TextProc *t, short *split, short *front, short *tail_morph, char *head, char *tail); /* 1001d030 */
void fe_StripMorphs_B(TextProc *t, short *split, short *front, short *tail_morph, char *head, char *tail); /* 1001d410 */
void fe_Acronym_CountUpper(TextProc *t, short *n, char *s);                /* 1001cf30 */
void fe_Acronym_SyllableParse(TextProc *t, short i, char *buf);            /* 10017630 */
/* Called by Acronym_SyllableParse: */
void fe_Acronym_OnsetSplit(TextProc *t, short *ok, char *s, short *skip);  /* 10019380 */
void fe_Acronym_CodaSplit(TextProc *t, short *ok, char *word, short *stripped); /* 1001ac00 */
void fe_Acronym_JoinHyphen(TextProc *t, char *rest, char *out);            /* 1001cdf0 */

/* ---- email stage (em_*.c; patent 134, REFERENCE 8.4) ------------------------------------------------------------ */
/* Called by TextReaderControl for every input line (para_buf); the return value tells it what to do with the line and
   the paragraph collected so far (REFERENCE 3.10): */
short fe_EmailHeaderBodyAnalyzer(TextProc *t);                             /* 1006fe00 */
extern short fe_hdr_key;   /* 0x10137100: position of the line's header keyword in the keyword list */
/* Called by EmailHeaderBodyAnalyzer: */
short fe_Email_Separators(TextProc *t, short a);                           /* 10074aa0 */
void fe_Email_SubjectLine(TextProc *t);                                    /* 10072940 (empty) */
char *fe_str_trim_leading(char *s);                                        /* 100442e0 (em_header.c) */
void fe_Email_FromLine(TextProc *t, const char *kw);                       /* 100712a0: kw "From:" or ">From:" */
void fe_Email_AddressList(TextProc *t);                                    /* 10071dd0 */
void fe_Email_DateLine(TextProc *t);                                       /* 10072c30 */
/* Called by Email_AddressList: */
void fe_Email_UncapWords(TextProc *t);                                     /* 10072950 */
/* Called by Email_DateLine: */
void fe_Email_DateMDY(TextProc *t, short sep1, short sep2);                /* 100741a0: m/d/yy (cc:Mail, Lotus Notes) */
void fe_Email_DateYMD(TextProc *t, short sep1, short sep2);                /* 100745e0: yyyy/m/d (MS Mail) */
void fe_Email_OrdinalSuffix(TextProc *t, char *day);                       /* 10074c50 */

/* ---- paragraph driver (paragraph.c; patent 140, REFERENCE 3.11) ----------------------------------------------- */
/* Called by TextReaderControl with the paragraph in text; appends its expansion to out_buf */
short fe_TextExpander_Paragraph(TextProc *t);                              /* 1002b8d0 */
void fe_Tokenize_JoinLetters(TextProc *t);                                 /* 10043fb0: "I B M" -> "~IBM" */
void fe_Output_AppendText(TextProc *t);                                    /* 1002e180: text -> out_buf, then cleared */

/* ---- control procedure (control.c; patent 132, REFERENCE 3.12) ------------------------------------------------ */
/* The front end's entry: tts_Speak and tts_SaveFile (email context) call it with a new TextProc, the settings line
   and the text, a zeroed output buffer and its size, and mode 1. The expanded text is appended to out. */
short fe_FrontEnd_ProcessText(TextProc *t, const char *in, char *out, uint32_t out_size, short mode); /* 10076f30 */
short fe_TextReaderControl(TextProc *t);                                   /* 1002b270: one input line (para_buf) */
/* Mode 2 (numline.c): every line on its own, its numbers read as in a formula. Neither caller uses it. */
void fe_NumLine_Read(TextProc *t);                                         /* 100450e0 */
void fe_NumLine_Fraction(TextProc *t);                                     /* 10045320 */
void fe_NumLine_MonthAbbrev(TextProc *t);                                  /* 100459e0 */
void fe_NumLine_Minus(TextProc *t);                                        /* 10045af0 */
void fe_NumLine_Parens(TextProc *t);                                       /* 10045b50 */
void fe_NumLine_Exponent(TextProc *t);                                     /* 10045b90 */
void fe_NumLine_Date(TextProc *t);                                         /* 10045bd0 */

/* Harness only: where the original reads stack bytes it never wrote, the port gets the real ones. The harness sets
   the frame of the hooked function at entry: byte x is [F+x] (ESP after the prologue's SUB and its PUSHes), followed
   by the return address and the first stack argument, and the bytes below F (the callees' frames: 0x400, 0x1400 for
   EmailHeaderBodyAnalyzer) are at negative x. fe_frame_of(va) returns it
   once, to the function at va (so not to a nested call), and NULL when there is none or fe_fixes is set; the port
   then uses 0 for those bytes. */
extern const unsigned char *fe_frame_bytes;
extern unsigned fe_frame_va;
extern const void *fe_frame_this;   /* the original's own TextProc address (it shows up in stale stack bytes) */
const unsigned char *fe_frame_of(unsigned va);
/* For a call made by ported code (a nested call), the harness can supply the frame the original's own nested call
   found (it runs the outer port after the original, see tools/fediff: NumberExpander). */
extern const unsigned char *(*fe_frame_nested)(unsigned va);

/* ---- bridge (fe_bridge.c): original DLL code for what is not ported yet (harness builds only) ----------------- */
void fe_bridge_init(unsigned char *dll_base);

#endif
