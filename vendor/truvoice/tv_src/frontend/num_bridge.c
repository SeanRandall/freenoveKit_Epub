/* Harness only: the number-, acronym- and email-stage functions named by -DB_<name> run the original on the copy (thiscall),
   so that a ported caller can be checked before its callees are ported. tools/fediff/build.sh compiles this file when BRIDGE
   lists names. Bridged functions are not hooked (fe_bridge_is); fe_bridge_target gives the address to call. */
#include "frontend.h"

void *(*fe_bridge_target)(unsigned va);
typedef void (__attribute__((thiscall)) *NumIP)(TextProc *, int, short *);
typedef void (__attribute__((thiscall)) *NumIB)(TextProc *, int, char *);
#define IP(name, va) void fe_##name(TextProc *t, short i, short *r) { ((NumIP)fe_bridge_target(va))(t, i, r); }

#ifdef B_Num_Date_MDY
IP(Num_Date_MDY, 0x1003a1e0)
#endif
#ifdef B_Num_Phone
IP(Num_Phone, 0x1003a690)
#endif
#ifdef B_Num_PhoneContext
IP(Num_PhoneContext, 0x1003aec0)
#endif
#ifdef B_Num_DigitString
IP(Num_DigitString, 0x1003c130)
#endif
#ifdef B_Num_SpecialNumbers
IP(Num_SpecialNumbers, 0x1003c6b0)
#endif
#ifdef B_Num_Agreement
IP(Num_Agreement, 0x1003cd70)
#endif
#ifdef B_Num_YearsRanges
IP(Num_YearsRanges, 0x1003df70)
#endif
#ifdef B_Num_DatesFractionsOrdinals
void fe_Num_DatesFractionsOrdinals(TextProc *t, short i, char *b) { ((NumIB)fe_bridge_target(0x100370f0))(t, i, b); }
#endif

typedef void (__attribute__((thiscall)) *AcrPPPBB)(TextProc *, short *, short *, short *, char *, char *);
typedef void (__attribute__((thiscall)) *AcrPB)(TextProc *, short *, char *);
typedef void (__attribute__((thiscall)) *AcrPBP)(TextProc *, short *, char *, short *);
typedef void (__attribute__((thiscall)) *AcrBB)(TextProc *, char *, char *);
#ifdef B_Acronym_MilitaryRating
IP(Acronym_MilitaryRating, 0x100502f0)
#endif
#ifdef B_AcronymInitialismExpander
IP(AcronymInitialismExpander, 0x10013460)
#endif
#ifdef B_Acronym_KnownLists
IP(Acronym_KnownLists, 0x10016ae0)
#endif
#ifdef B_StripMorphs_A
void fe_StripMorphs_A(TextProc *t, short *a, short *b, short *c, char *s1, char *s2)
{ ((AcrPPPBB)fe_bridge_target(0x1001d030))(t, a, b, c, s1, s2); }
#endif
#ifdef B_StripMorphs_B
void fe_StripMorphs_B(TextProc *t, short *a, short *b, short *c, char *s1, char *s2)
{ ((AcrPPPBB)fe_bridge_target(0x1001d410))(t, a, b, c, s1, s2); }
#endif
#ifdef B_Acronym_CountUpper
void fe_Acronym_CountUpper(TextProc *t, short *n, char *s) { ((AcrPB)fe_bridge_target(0x1001cf30))(t, n, s); }
#endif
#ifdef B_Acronym_SyllableParse
void fe_Acronym_SyllableParse(TextProc *t, short i, char *b) { ((NumIB)fe_bridge_target(0x10017630))(t, i, b); }
#endif
#ifdef B_Acronym_OnsetSplit
void fe_Acronym_OnsetSplit(TextProc *t, short *a, char *s, short *b) { ((AcrPBP)fe_bridge_target(0x10019380))(t, a, s, b); }
#endif
#ifdef B_Acronym_CodaSplit
void fe_Acronym_CodaSplit(TextProc *t, short *a, char *s, short *b) { ((AcrPBP)fe_bridge_target(0x1001ac00))(t, a, s, b); }
#endif
#ifdef B_Acronym_JoinHyphen
void fe_Acronym_JoinHyphen(TextProc *t, char *a, char *b) { ((AcrBB)fe_bridge_target(0x1001cdf0))(t, a, b); }
#endif

typedef short (__attribute__((thiscall)) *EmlV)(TextProc *);
typedef short (__attribute__((thiscall)) *EmlI)(TextProc *, int);
typedef void (__attribute__((thiscall)) *EmlB)(TextProc *, char *);
typedef void (__attribute__((thiscall)) *EmlII)(TextProc *, int, int);
/* The email stage's global (0x10137100): a bridged function uses the DLL's, so the port's (fe_hdr_key) is copied in
   before the call and back after it, and the DLL's is left as it was. */
#define EML(call) do { short *g_ = fe_bridge_target(0x10137100), s_ = *g_; *g_ = fe_hdr_key; call;                        fe_hdr_key = *g_; *g_ = s_; } while (0)
#ifdef B_EmailHeaderBodyAnalyzer
short fe_EmailHeaderBodyAnalyzer(TextProc *t) { short r; EML(r = ((EmlV)fe_bridge_target(0x1006fe00))(t)); return r; }
#endif
#ifdef B_Email_Separators
short fe_Email_Separators(TextProc *t, short a) { short r; EML(r = ((EmlI)fe_bridge_target(0x10074aa0))(t, a)); return r; }
#endif
#ifdef B_Email_FromLine
void fe_Email_FromLine(TextProc *t, char *s) { EML(((EmlB)fe_bridge_target(0x100712a0))(t, s)); }
#endif
#ifdef B_Email_AddressList
void fe_Email_AddressList(TextProc *t) { EML(((EmlV)fe_bridge_target(0x10071dd0))(t)); }
#endif
#ifdef B_Email_UncapWords
void fe_Email_UncapWords(TextProc *t) { EML(((EmlV)fe_bridge_target(0x10072950))(t)); }
#endif
#ifdef B_Email_DateLine
void fe_Email_DateLine(TextProc *t) { EML(((EmlV)fe_bridge_target(0x10072c30))(t)); }
#endif
#ifdef B_Email_DateMDY
void fe_Email_DateMDY(TextProc *t, short a, short b) { EML(((EmlII)fe_bridge_target(0x100741a0))(t, a, b)); }
#endif
#ifdef B_Email_DateYMD
void fe_Email_DateYMD(TextProc *t, short a, short b) { EML(((EmlII)fe_bridge_target(0x100745e0))(t, a, b)); }
#endif
#ifdef B_Email_OrdinalSuffix
void fe_Email_OrdinalSuffix(TextProc *t, char *s) { EML(((EmlB)fe_bridge_target(0x10074c50))(t, s)); }
#endif

typedef void (__attribute__((thiscall)) *SplIPB)(TextProc *, int, short *, char *);
#ifdef B_Tokenize_SplitSeparators
void fe_Tokenize_SplitSeparators(TextProc *t, short pos, short *end, char *sep)
{ ((SplIPB)fe_bridge_target(0x1002e410))(t, pos, end, sep); }
#endif

int fe_bridge_is(const void *fn)
{
    static const void *const b[] = {
#ifdef B_Num_Date_MDY
        fe_Num_Date_MDY,
#endif
#ifdef B_Num_Phone
        fe_Num_Phone,
#endif
#ifdef B_Num_PhoneContext
        fe_Num_PhoneContext,
#endif
#ifdef B_Num_DigitString
        fe_Num_DigitString,
#endif
#ifdef B_Num_SpecialNumbers
        fe_Num_SpecialNumbers,
#endif
#ifdef B_Num_Agreement
        fe_Num_Agreement,
#endif
#ifdef B_Num_YearsRanges
        fe_Num_YearsRanges,
#endif
#ifdef B_Num_DatesFractionsOrdinals
        fe_Num_DatesFractionsOrdinals,
#endif
#ifdef B_Acronym_MilitaryRating
        fe_Acronym_MilitaryRating,
#endif
#ifdef B_AcronymInitialismExpander
        fe_AcronymInitialismExpander,
#endif
#ifdef B_Acronym_KnownLists
        fe_Acronym_KnownLists,
#endif
#ifdef B_StripMorphs_A
        fe_StripMorphs_A,
#endif
#ifdef B_StripMorphs_B
        fe_StripMorphs_B,
#endif
#ifdef B_Acronym_CountUpper
        fe_Acronym_CountUpper,
#endif
#ifdef B_Acronym_SyllableParse
        fe_Acronym_SyllableParse,
#endif
#ifdef B_Acronym_OnsetSplit
        fe_Acronym_OnsetSplit,
#endif
#ifdef B_Acronym_CodaSplit
        fe_Acronym_CodaSplit,
#endif
#ifdef B_Acronym_JoinHyphen
        fe_Acronym_JoinHyphen,
#endif
#ifdef B_EmailHeaderBodyAnalyzer
        fe_EmailHeaderBodyAnalyzer,
#endif
#ifdef B_Email_Separators
        fe_Email_Separators,
#endif
#ifdef B_Email_FromLine
        fe_Email_FromLine,
#endif
#ifdef B_Email_AddressList
        fe_Email_AddressList,
#endif
#ifdef B_Email_UncapWords
        fe_Email_UncapWords,
#endif
#ifdef B_Email_DateLine
        fe_Email_DateLine,
#endif
#ifdef B_Email_DateMDY
        fe_Email_DateMDY,
#endif
#ifdef B_Email_DateYMD
        fe_Email_DateYMD,
#endif
#ifdef B_Email_OrdinalSuffix
        fe_Email_OrdinalSuffix,
#endif
#ifdef B_Tokenize_SplitSeparators
        fe_Tokenize_SplitSeparators,
#endif
        0};
    for (size_t k = 0; b[k]; k++)
        if (fn == b[k]) return 1;
    return 0;
}
