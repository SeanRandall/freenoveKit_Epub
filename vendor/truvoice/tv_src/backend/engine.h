/*
 * Engine object: the per-utterance-stream synthesizer state that the SAPI
 * engine thread creates, feeds with text, and steps to produce PCM.
 */
#ifndef TV_ENGINE_H
#define TV_ENGINE_H

#include "tv_common.h"
#include "tv_ref.h"
#include "engine_struct.h" /* generated from engine.fields */

#define TV_RING_SIZE 0x1000
#define TV_NODE_POOL 618

/* TV_ENG32: what Engine.handle points at.  In the DLL it is the tts handle
 * object, whose index-mark callback and its argument sit at +0xbc6c and
 * +0xbc70 (Engine_RunControl, case 'i'); the C keeps only those two.
 * TruVoice never sets the callback. */
typedef struct TvMarkSink {
    void (*fn)(uint32_t ctx, uint32_t mark);
    uint32_t ctx;
} TvMarkSink;

/* Node types (low three bits of Node.flags). */
#define NODE_TYPE_MASK 7u
#define NODE_CONTROL   0u
#define NODE_FREE      6u
#define NODE_SENTINEL  7u
#define NODE_TYPE(n)   ((n)->flags & NODE_TYPE_MASK)

/* Stage-mask bit for each node type (types 6 and 7 are never selected). */
/* @0x1010e420 */
extern const uint32_t g_node_type_bits[8];

/* ---- work list and stage windows (node.c) -------------------------------- */

/* @0x10077620 */
Node *TV_CDECL List_InsertBefore(Node *n, Node *before);
/* @0x10077640 */
Node *TV_CDECL List_Unlink(Node *n);
/* @0x10077970 */
void TV_THISCALL Engine_ResetNodes(Engine *self);
/* @0x10077660 */
Node *TV_THISCALL Engine_AppendNode(Engine *self, int32_t type, int32_t value);
/* @0x100776d0 */
Node *TV_THISCALL Engine_NodeAlloc(Engine *self, Node *ref, int32_t after, int32_t type,
                                   uint8_t value);
/* @0x10077950 */
Node *TV_THISCALL Engine_StageNext(Engine *self, Node *n);
/* @0x10077790 */
Node *TV_THISCALL Engine_StagePrev(Engine *self, Node *n);
/* @0x100778b0 */
Node *TV_THISCALL Engine_NodeFree(Engine *self, Node *n, int32_t forward);
/* @0x10077ab0 */
uint8_t TV_THISCALL Engine_StageBegin(Engine *self, StageCtx *st);
/* @0x100777b0 */
uint8_t TV_THISCALL Engine_StageEnd(Engine *self);

/* ---- lifecycle and setters (engine.c) ----------------------------------- */

/* @0x10001c10 */
Engine *TV_THISCALL Engine_Construct(Engine *self);
/* @0x100240c0 */
int32_t TV_THISCALL Engine_Init(Engine *self);
/* @0x10024120 */
int32_t TV_THISCALL Engine_Reset(Engine *self, int32_t full);
/* @0x100244a0 */
void TV_THISCALL Engine_SetPitch(Engine *self, int32_t pitch);
/* @0x100244d0 */
void TV_THISCALL Engine_SetSpeed(Engine *self, int32_t wpm);
/* @0x10024500 */
void TV_THISCALL Engine_SetVolume(Engine *self, uint32_t vol);

/* volume.c: the tabulated pow()/log10() the original used here. */
uint32_t Volume_FromAtten(int32_t arg);
int32_t Volume_ToAtten(uint32_t vol);
/* @none cg:0x1002c8f0 */
void TV_THISCALL Engine_SetVoice(Engine *self, uint32_t voice);
/* @0x10003640 */
void TV_THISCALL Engine_ResetRings(Engine *self);
/* @0x10044710 */
void TV_THISCALL Prosody_Reset(Engine *self);
/* @0x10024210 */
int32_t TV_THISCALL Engine_Step(Engine *self);

/* ---- synthesizer (synth.c) ---------------------------------------------- */

/* @0x1005e300 */
void TV_THISCALL Synth_ResetTracks(Engine *self);
/* @0x1005c090 */
void TV_THISCALL Synth_InitFilters(Engine *self);
/* @0x10011b00 */
int32_t TV_CDECL Synth_ScaleParam(int32_t index, uint8_t raw);
/* @0x1005c490 */
uint8_t TV_THISCALL Synth_Step(Engine *self);
/* Generate the samples for one parameter frame (not yet decompiled). */
/* @0x10003ae0 */
void TV_THISCALL Synth_Generate(Engine *self, int16_t sample_rate, int16_t *coef);
/* TV_ENG32 only: Synth_Generate's sample store (inlined there in the DLL)
 * and the G.711 encoders it uses (g711.c). */
void Synth_PutSample(Engine *self, int32_t s);
int32_t TV_CDECL G711_Search(int32_t val, const int16_t *table, int32_t n);
uint8_t TV_CDECL G711_LinearToUlaw(int32_t pcm);
uint8_t TV_CDECL G711_LinearToAlaw(int32_t pcm);
/* Build one frame of filt_coef from the parameter tracks (not yet
 * decompiled). */
/* @0x10044730 */
void TV_THISCALL Synth_Frame(Engine *self);
/* @0x1005e590 */
int32_t TV_STDCALL Synth_MulQ15(int32_t a, int32_t b);
/* @0x1005e6c0 */
int32_t TV_STDCALL Synth_MulShr11(int32_t a, int32_t b);
/* @0x1005e6d0 */
int32_t TV_STDCALL Synth_MulShr12(int32_t a, int32_t b, int32_t *hi);
/* @0x1005c410 */
uint8_t TV_THISCALL Synth_Gate(Engine *self, int32_t op);
/* @0x1005e3b0 */
uint8_t TV_THISCALL Tracks_Op(Engine *self, int32_t op, int32_t n);

/* ---- output (output.c) -------------------------------------------------- */

/* @0x100048a0 */
void TV_THISCALL Output_Reset(Engine *self, int16_t sample_rate);

/* ---- stage resets (stages.c) -------------------------------------------- */

/* @0x1000f4d0 */
void TV_THISCALL Stage0_Reset(Engine *self);
/* @0x10027920 */
void TV_THISCALL Stage1_Reset(Engine *self);
/* @0x1005b840 */
void TV_THISCALL Stage2_Reset(Engine *self);
/* @0x10012bf0 */
void TV_THISCALL Stage3_Reset(Engine *self);
/* @0x100130e0 */
void TV_THISCALL Stage3_ResetParams(Engine *self);
/* @0x1000d7d0 */
void TV_THISCALL Stage4_Reset(Engine *self);
/* @0x1000d7e0 */
int32_t TV_THISCALL Stage4_Run(Engine *self);

/* Stage drivers (not yet decompiled).  Stages 0-2 return whether they handed
 * nodes on; stage 3 returns 0 idle, 1 track buffer full, 2 more to do. */
/* @0x1000f520 */
uint8_t TV_THISCALL Stage0_Run(Engine *self);
/* @0x1000f390 */
uint8_t TV_CDECL Stage0_CharClass(int32_t cls, uint8_t c);
/* @0x1000f450 */
uint8_t TV_CDECL Stage0_IsPlain(char c);
/* @0x1000f2d0 */
uint8_t TV_THISCALL Stage0_MatchWord(Engine *self, uint8_t list, uint8_t fold_case);
/* @0x10010130 */
void TV_THISCALL Stage0_Emit(Engine *self, int32_t type, uint8_t value);
/* @0x100100b0 */
uint8_t TV_THISCALL Stage0_Finish(Engine *self, uint8_t done);
/* Phonetic input mode: "[...]" spelled as phoneme names (phonetic.c). */
/* @0x100101f0 */
void TV_THISCALL Stage0_Phonetic(Engine *self);
/* @0x10011250 */
void TV_THISCALL Stage0_PhoneticDigit(Engine *self);
/* @0x100112a0 */
void TV_THISCALL Stage0_PhoneticPair(Engine *self);

/* ---- stage 1: word pronunciation (stage1.c) ----------------------------- */

/* @0x10029710 */
uint8_t TV_THISCALL Stage1_Run(Engine *self);
/* @0x10029c70 */
void TV_THISCALL Stage1_TakeSpan(Engine *self);
/* @0x10029cb0 */
uint8_t TV_THISCALL Stage1_ScanAhead(Engine *self);
/* @0x10029d50 */
uint8_t TV_THISCALL Stage1_Gather(Engine *self);
/* @0x10011740 */
void TV_CDECL Word_Classify(const char *word, Node *n);
/* Letter-to-sound rules, used when the lexicon has no entry (not yet
 * decompiled). */
/* @0x100023e0 */
void TV_THISCALL Stage1_Rules(Engine *self);
/* Settle the vowel of the syllable just finished. */
/* @0x100031e0 */
void TV_THISCALL Lts_Syllable(Engine *self, int32_t final);
/* Condition program: a strong letter before the syllable break. */
/* @0x100aece0 */
extern const uint8_t g_lts_cond_strong[];

/* @0x100297e0 */
Node *TV_THISCALL Stage1_Pronounce(Engine *self);
/* @0x10027960 */
Node *TV_THISCALL Stage1_Vowel(Engine *self, Node *n);
/* A helper of the per-vowel pass (unverified: never reached by the corpus). */
/* @0x1002b0e0 */
uint8_t TV_THISCALL Stage1_VowelAux(Engine *self);
/* Phrase-level prosody, around the '%' marker (not yet decompiled). */
/* @0x1002a780 */
void TV_THISCALL Stage1_Phrase(Engine *self);
/* @0x1002ad20 */
void TV_THISCALL Stage1_SpreadStress(Engine *self);
/* @0x1002ad80 */
void TV_THISCALL Stage1_PhraseEnd(Engine *self);
/* @0x1002a6d0 */
void TV_THISCALL Stage1_Mark(Engine *self, Node *n);
/* @0x1002a040 */
void TV_THISCALL Stage1_Emit(Engine *self);
/* @0x1002afe0 */
void TV_THISCALL Stage1_Close(Engine *self);

/* ---- letter-to-sound rules (stage1.c) ----------------------------------- */

/* One rule of the letter-to-sound table.  The rules for a letter follow each
 * other in memory and are tried in order. */
typedef struct LtsEntry {
    tv_ref left;     /* 0x00 left-context letters, matched backwards */
    tv_ref out;   /* 0x04 control bytes, then the phonemes to emit */
    tv_ref cond;  /* 0x08 right-context condition program */
    tv_ref want; /* 0x0c the two feature masks the rule needs */
    tv_ref set;  /* 0x10 the two feature masks it leaves behind */
} LtsEntry;

/* The rules for each letter, indexed by the letter ('@'..'['). */
/* @0x100b3698 */
extern const tv_ref g_lts_rules[];

/* One affix (prefix/suffix) rule.  The rules for a given letter form a
 * NULL-terminated array of pointers, and `next` chains to the array to try
 * after this one matched. */
typedef struct LtsRule {
    tv_ref text;     /* 0x00 the letters, in match order */
    tv_ref cond;  /* 0x04 context condition program */
    uint8_t b08;          /* 0x08 re-run the lexicon after stripping */
    uint8_t b09;          /* 0x09 word class */
    uint8_t b0a;          /* 0x0a stress level, or > 1: a s1_1c2d code */
    uint8_t b0b;
    tv_ref next; /* 0x0c */
} LtsRule;

/* Rule lists indexed by the last / first letter of the word. */
/* @0x100d0fc4 */
extern const tv_ref g_lts_suffix[256];
/* @0x100d1c64 */
extern const tv_ref g_lts_prefix[256];
/* Letter-class descriptors: high byte selects a g_phone_attr bank, low byte
 * the bit to test. */
/* @0x100a2b90 */
extern const uint32_t g_lts_class[128];

/* @0x10002d60 */
uint8_t TV_THISCALL Lts_TestFeatures(Engine *self, const LtsEntry *r);
/* @0x10002de0 */
uint8_t TV_THISCALL Lts_MatchLeft(Engine *self, const LtsEntry *r);
/* @0x10002e30 */
uint8_t TV_THISCALL Lts_TestContext(Engine *self, const uint8_t *cond, Node *n,
                                    int32_t dir);
/* @0x1002a360 */
tv_ref TV_THISCALL Lts_MatchAffix(Engine *self, Node *a, Node *b,
                                          const tv_ref *set, int32_t dir);
/* @0x1002a100 */
uint8_t TV_THISCALL Stage1_Lookup(Engine *self);
/* @0x1002a480 */
int32_t TV_THISCALL Lts_ApplyAffix(Engine *self, int32_t dir);

/* The suffix rules whose stems need the spelling repaired ("-ING", "-EST",
 * "-ILY", "-ABLE", "-ABLY", "-OR", "-S").  Their text is the suffix reversed,
 * because a suffix is matched backwards from the end of the word. */
/* @0x100d0710 */ extern const LtsRule g_lts_rule_ing;
/* @0x100d0be0 */ extern const LtsRule g_lts_rule_est;
/* @0x100d0d60 */ extern const LtsRule g_lts_rule_ily;
/* @0x100d0500 */ extern const LtsRule g_lts_rule_able;
/* @0x100d0d40 */ extern const LtsRule g_lts_rule_ably;
/* @0x100d0a50 */ extern const LtsRule g_lts_rule_or;
/* @0x100d0b60 */ extern const LtsRule g_lts_rule_s;

/* Condition programs the stem repair uses. */
/* @0x100cf910 */ extern const uint8_t g_lts_cond_stem_ok[];
/* @0x100cf8d8 */ extern const uint8_t g_lts_cond_want_e[];
/* @0x100cf9a8 */ extern const uint8_t g_lts_cond_no_e[];
/* @0x100cf920 */ extern const uint8_t g_lts_cond_want_t[];
/* One user-lexicon entry: the spelling and its phoneme string.  The user
 * lexicon is the small sorted table the SAPI lexicon calls add to; the main
 * dictionary is a packed state machine inside the DLL (Lexicon_Try). */
typedef struct LexEntry {
    const char *word;
    const char *pron;
} LexEntry;

/* The table is written as well as read, and where a pointer is wider
 * than the original's four bytes its entries no longer fit the room the
 * image left for them, so outside the hook build the library owns it
 * (lexicon.c).  The original ships it empty, so that is the same table. */
/* @0x1012d3a0 */ extern LexEntry g_lexicon[];
/* @0x1012d398 */ extern uint32_t g_lexicon_count;

/* @0x10024ff0 */
int TV_CDECL UserLex_Compare(const void *a, const void *b);
/* @0x10025030 */
uint8_t TV_THISCALL UserLex_Try(Engine *self);
void Lexicon_Lock(void);
void Lexicon_Unlock(void);
/* @0x10074f90 */
uint8_t TV_THISCALL Lexicon_Try(Engine *self);

/* ---- built-in dictionary (dict.c) --------------------------------------- */

/* The state machine that packs a word's letters into a lookup key. */
typedef struct DictKeyState {
    uint32_t mask;      /* 0x00 */
    uint32_t mask2;     /* 0x04 */
    uint8_t  next;      /* 0x08 */
    uint8_t  pad09[3];
    uint8_t  shift;     /* 0x0c */
    uint8_t  pad0d[3];
    uint8_t  pstate;    /* 0x10 the phoneme state to start unpacking in */
    uint8_t  pad11[3];
    uint8_t  last_mask; /* 0x14 */
    uint8_t  pad15[3];
} DictKeyState;

/* The state machine that unpacks an entry's phoneme stream. */
typedef struct DictPhState {
    uint32_t mask;      /* 0x00 */
    uint32_t mask2;     /* 0x04 */
    uint8_t  next;      /* 0x08 */
    uint8_t  pad09[3];
    uint8_t  shift;     /* 0x0c */
    uint8_t  pad0d[3];
} DictPhState;

/* @0x10100788 */ extern const DictKeyState g_dict_key[];
/* @0x101007d0 */ extern const DictPhState g_dict_ph[];
/* Entry size per key state and suffix count: [state * 17 + n]. */
/* @0x10100810 */ extern const uint8_t g_dict_skip[];
/* The blob's index: one bucket pointer per letter, then a limit. */
/* The bucket table: a reference to an array of references. */
/* @0x10100844 */ extern const tv_ref g_dict_base;

/* @0x10074f80 */
uint8_t TV_CDECL Dict_Byte(const void *p);

/* @0x10004a30 */
uint8_t TV_STDCALL Phone_IsVowel(uint8_t c);
/* @0x1005b8c0 */
Node *TV_THISCALL Node_PrevBoundary(Engine *self, Node *n);
/* @0x1005b8f0 */
Node *TV_THISCALL Node_NextWord(Engine *self, Node *n);
/* ---- stage 2: timing (stage2.c) ----------------------------------------- */

/* @0x1005b740 */
uint8_t TV_THISCALL Stage2_Run(Engine *self);
/* @0x1005ae50 */
int32_t TV_THISCALL Stage2_Next(Engine *self);
/* @0x1005adb0 */
void TV_THISCALL Stage2_Emit(Engine *self);
/* @0x1005b010 */
int32_t TV_THISCALL Stage2_Begin(Engine *self);
/* @0x10075af0 */
void TV_THISCALL Stage2_Context(Engine *self);
/* @0x1005b970 */
Node *TV_THISCALL Stage2_PrevPhone(Engine *self, Node *n);
/* @0x1005ac10 */
void TV_THISCALL Stage2_Push(Engine *self, int32_t kind);
/* @0x10076cd0 */
void TV_THISCALL Stage2_Close(Engine *self);
/* @0x1005abc0 */
uint8_t TV_CDECL Phone_TestMask(Node *n, int32_t mask, int32_t neg);
/* @0x1005b920 */
Node *TV_THISCALL Stage2_NextPhone(Engine *self, Node *n);
/* @0x10076300 */
void TV_THISCALL Stage2_Pitch(Engine *self);
/* @0x10076560 */
void TV_THISCALL Stage2_Contour(Engine *self);
/* @0x1001d710 */
void TV_THISCALL Stage2_Flush(Engine *self);
/* @0x10076510 */
void TV_THISCALL Stage2_ResetRun(Engine *self);
/* @0x1001f7a0 */
void TV_THISCALL Stage2_Silence(Engine *self);
/* @0x100221d0 */
int32_t TV_THISCALL Stage2_DurFast(Engine *self);
/* @0x1001f7f0 */
int32_t TV_THISCALL Stage2_Pause(Engine *self);
/* @0x1001d940 */
int32_t TV_THISCALL Stage2_DurStress(Engine *self, int32_t dur);
/* @0x1001d7b0 */
uint8_t TV_THISCALL Stage2_Merge(Engine *self);
/* @0x100236c0 */
int32_t TV_THISCALL Stage2_MinDur(Engine *self, int32_t pct);
/* @0x1005bb10 */
Node *TV_THISCALL Stage2_Find(Engine *self, int32_t dir, int32_t count,
                              int32_t mask);
/* @0x10076250 */
void TV_THISCALL Stage2_Aspirate(Engine *self, Node *n, int32_t which);
/* @0x10076180 */
void TV_THISCALL Stage2_Split(Engine *self, int32_t slot0, int32_t value,
                              int32_t slot2);
/* @0x1005b9e0 */
uint8_t TV_THISCALL Stage2_Scan(Engine *self, int32_t dir, int32_t count,
                                int32_t mask1, int32_t mask2, int32_t mode);
/* @0x1001da00 */
int32_t TV_THISCALL Stage2_DurRules(Engine *self);
/* @0x10023720 */
int32_t TV_THISCALL Stage2_DurAdjust(Engine *self);
/* One test in a duration rule. */
typedef struct DurTest {
    uint8_t kind;       /* 0x00 what to compare */
    uint8_t dir;        /* 0x01 1 = step forward first */
    uint8_t count;      /* 0x02 how many steps */
    uint8_t pad03;
    int32_t arg;        /* 0x04 attribute mask or value */
} DurTest;

/* One duration rule: a phoneme string to match, some flag conditions, a
 * list of tests, and the duration it gives. */
typedef struct DurRule {
    uint8_t len;        /* 0x00 phonemes in `text` */
    uint8_t back;       /* 0x01 steps back before matching */
    uint8_t pad02[2];
    tv_ref text;   /* 0x04 */
    uint8_t cond;       /* 0x08 flag-condition bits */
    uint8_t ntests;     /* 0x09 */
    uint8_t pad0a[2];
    tv_ref tests; /* 0x0c */
    uint8_t result;     /* 0x10 */
    uint8_t pad11[3];
} DurRule;

/* @0x10075cc0 */
void TV_THISCALL Stage2_Phrase(Engine *self, int32_t punct);
/* @0x10023c60 */
int32_t TV_THISCALL Stage2_DurTable(Engine *self, int32_t c0, int32_t a1,
                                    int32_t a2);
/* @0x10022230 */
int32_t TV_THISCALL Stage2_DurVowel(Engine *self);
/* @0x1001f890 */
int32_t TV_THISCALL Stage2_DurNasal(Engine *self);
/* @0x10020180 */
int32_t TV_THISCALL Stage2_DurStop(Engine *self);
/* @0x1001dcd0 */
int32_t TV_THISCALL Stage2_DurFric(Engine *self);
/* @0x10076cf0 */
void TV_THISCALL Stage2_Break(Engine *self, int32_t n);
/* @0x10011b60 */
int32_t TV_THISCALL Stage3_Run(Engine *self);

/* ---- feeding (feed.c) --------------------------------------------------- */

/* @0x10024960 */
void TV_THISCALL Engine_Feed(Engine *self, const char *text, uint32_t len, uint32_t *ppos);
/* @0x10024f20 */
void TV_THISCALL Engine_Flush(Engine *self, int32_t new_item);

/* ---- preformatter (preformat.c) ----------------------------------------- */

/* @0x1005c470 */
void TV_THISCALL Preformat_Reset(Engine *self);
/* @0x100775e0 */
uint8_t TV_CDECL FoldAccent(uint8_t *c);
/* @0x1005cd20 */
void TV_THISCALL Preformat_PutChar(Engine *self, uint8_t c);
/* @0x1005c4f0 */
void TV_THISCALL Preformat_Run(Engine *self);

/* The dword at a given offset of the original 32-bit engine layout: set to 0
 * if it is -1 (see Preformat_Run: digits of a 17th+ CSI parameter). */
void Engine_ZeroDwordIfMinus1(Engine *self, uint32_t off32);

/* ---- TextIn front end (textin.c) ---------------------------------------- */

/* @0x10024760 */
int32_t TV_CDECL Bits_Test(int32_t bit, const uint32_t *bits);
/* @0x100247a0 */
uint32_t TV_CDECL Bits_Set(int32_t bit, uint32_t *bits);
/* @0x100247d0 */
int32_t TV_CDECL Bits_Next(int32_t bit, const uint32_t *bits);
/* @0x10024840 */
int32_t TV_CDECL AllocString(char **p, int32_t n);
/* @0x10024540 */
Token *TV_THISCALL TextIn_InsertAfter(TextIn *self, Token *ref);
/* @0x100245c0 */
Token *TV_THISCALL TextIn_InsertBefore(TextIn *self, Token *ref);
/* @0x10024640 */
int32_t TV_THISCALL TextIn_InsertText(TextIn *self, Token *ref, const char *s, int32_t dir);
/* @0x100246d0 */
Token *TV_THISCALL TextIn_RemoveToken(TextIn *self, Token *t, int32_t dir);
/* @0x10024890 */
int32_t TV_THISCALL TextIn_GetChar(TextIn *self);
/* @0x100248d0 */
int32_t TV_THISCALL TextIn_Unget(TextIn *self);
/* @0x100248f0 */
int32_t TV_THISCALL TextIn_PutString(TextIn *self, const char *s);
/* @0x10024940 */
int32_t TV_THISCALL TextIn_Error(TextIn *self, int32_t code);
/* @0x1005d6f0 */
TextIn *TV_THISCALL TextIn_Construct(TextIn *self, int32_t mode);
/* @0x1005d720 */
int32_t TV_THISCALL TextIn_Reset(TextIn *self);
/* @0x10024e90 */
uint8_t TV_THISCALL Engine_CreateTextIn(Engine *self);
/* @0x1005dc90 */
int32_t TV_THISCALL TextIn_ReadEscape(TextIn *self);
/* @0x1005d890 */
int32_t TV_THISCALL TextIn_ReadToken(TextIn *self, Token **out);
/* @0x1005de40 */
int32_t TV_THISCALL TextIn_Split(TextIn *self, Token **pt);
/* @0x1005be70 */
int32_t TV_THISCALL TextIn_Stub(TextIn *self, Token *t);
/* @0x1005bba0 */
int32_t TV_THISCALL TextIn_Classify(TextIn *self, Token *t);
/* @0x1005be80 */
int32_t TV_THISCALL TextIn_ClassifyNumber(TextIn *self, Token *t);
/* @0x1005d800 */
int32_t TV_THISCALL TextIn_Tokenize(TextIn *self);
/* @0x1005cde0 */
int32_t TV_THISCALL TextIn_Advance(TextIn *self);
/* @0x1005d5b0 */
int32_t TV_THISCALL TextIn_ExpandToken(TextIn *self, Token **pt);
/* @0x1005d5e0 */
int32_t TV_THISCALL TextIn_Emit(TextIn *self, int32_t all);
/* @0x1005d750 */
int32_t TV_THISCALL TextIn_Flush(TextIn *self, int32_t final);
/* @0x1005e030 */
int32_t TV_THISCALL TextIn_Mode4(TextIn *self, Token *t);
/* @0x1005ce80 */
int32_t TV_THISCALL TextIn_Expand(TextIn *self, Token *t, char *buf1, char *buf2);

/* ---- input stage (input.c) ---------------------------------------------- */

/* @0x10003690 */
uint8_t TV_THISCALL Engine_InputStage(Engine *self);

/* ---- control commands (control.c) --------------------------------------- */

/* Execute the control node at the running stage's ctl pointer; returns 0 if
 * the stage must stop there. */
/* @0x10011310 */
uint8_t TV_THISCALL Engine_RunControl(Engine *self);

/* Calls back into the owning SAPI layer (sapi.c; no-ops when standalone). */
void Sapi_Lock(SapiCentral *s);
void Sapi_Unlock(SapiCentral *s);
void Sapi_Post(SapiCentral *s, uint32_t msg, uint32_t wp, uint32_t lp);
int32_t Sapi_QueuePush(SapiCentral *s, const void *data, uint32_t size);

/* ---- parameter-track shaping (track.c) ---------------------------------- */

/* @0x1005e650 */
void TV_STDCALL Track_Line(uint8_t *buf, int32_t at, int32_t back,
                           int32_t fwd);
/* @0x1005e5b0 */
void TV_THISCALL Track_Decay(Engine *self, uint8_t *buf, int32_t pos,
                             int32_t shape, int32_t n, uint8_t from,
                             uint8_t to);
/* @0x1005e6f0 */
void TV_THISCALL Track_BlendBack(Engine *self, uint8_t *buf, int32_t pos,
                                 int32_t shape, int32_t n, uint8_t target);
/* @0x1005e770 */
void TV_THISCALL Track_BlendFwd(Engine *self, uint8_t *buf, int32_t pos,
                                int32_t shape, int32_t n, uint8_t target);

/* @0x1006fc40 */
void TV_STDCALL Track_Fill(uint8_t *buf, int32_t pos, int32_t n,
                           uint8_t value);
/* @0x10063690 */
void TV_STDCALL Track_RampTo(uint8_t *buf, int32_t pos, int32_t n,
                             uint8_t from, uint8_t to);
/* @0x1006fc70 */
void TV_THISCALL Track_Nudge(Engine *self, uint8_t *buf, int32_t mode,
                             int32_t pos, int32_t shape, int32_t n,
                             int32_t delta);

/* ---- stage 3: phonetics (stage3.c) -------------------------------------- */

/* One stage-3 rule: a list of conditions on the phonemes around the cursor,
 * the parameter edits to make when they all hold, and the routines to run
 * after them.  The rules for a pair of phoneme classes are tried in order
 * and the first that matches wins. */
/* One parameter edit: what to do, to which value, with what. */
typedef struct S3Edit {
    uint8_t  op;        /* 0x00 low 7 bits the operation, bit 7 "one more" */
    uint8_t  when;      /* 0x01 which passes this edit belongs to */
    uint8_t  pad0[2];
    int32_t  arg;       /* 0x04 */
    uint8_t  field;     /* 0x08 which value it edits */
    uint8_t  index;     /* 0x09 and which one of them */
    uint8_t  pad1[2];
} S3Edit;

/* One entry of a phoneme-pair table: a phoneme and the set it may precede. */
typedef struct S3Pair {
    uint8_t ch;            /* 0x00, zero ends the list */
    uint8_t pad[3];
    tv_ref set;    /* 0x04 */
} S3Pair;

typedef struct S3Rule {
    tv_ref cond;        /* 0x00 conditions, terminated by 0x18 */
    tv_ref edits; /* 0x04 blocks of edits, NULL-terminated */
    tv_ref ops;         /* 0x08 routines to run, terminated by 0 */
} S3Rule;


/* @0x10060b30 */
int32_t TV_STDCALL Vowel_Index(uint8_t c);
/* @0x10002100 */
uint8_t TV_THISCALL Stage3_Char(Engine *self, int32_t which);

/* ---- input rings (ring.c) ------------------------------------------------ */

/* @0x10003990 */
int32_t TV_THISCALL Engine_InFree(Engine *self);
/* @0x100039b0 */
int32_t TV_THISCALL Engine_InGet(Engine *self);
/* @0x100039f0 */
uint8_t TV_THISCALL Engine_InUnget(Engine *self);
/* @0x10003a20 */
uint8_t TV_THISCALL Engine_InPut(Engine *self, uint8_t c);
/* @0x10003a70 */
uint8_t TV_THISCALL Engine_InPutEnd(Engine *self);
/* @0x100038e0 */
int32_t TV_THISCALL Engine_MidFree(Engine *self);
/* @0x10003900 */
int32_t TV_THISCALL Engine_MidGet(Engine *self);
/* @0x10003940 */
uint8_t TV_THISCALL Engine_MidPut(Engine *self, uint8_t c);

/* OpenTV's extra output rate, the one the original never offered.  Changing
 * it means changing this and re-running tools/gen_synth_hifi.py --rate; the
 * generated file static-asserts that the two agree. */
#define TV_SR_HIFI 16000

#endif
