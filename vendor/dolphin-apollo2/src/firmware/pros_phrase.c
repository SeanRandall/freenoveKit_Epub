#include "pros_phrase.h"
#include "pros_decl.h"
#include "pros_flags.h"
#include "fw_ops.h"
#include "tn_convert.h"
#include "data/fw_data.h"

#define PROS_LONG_WORD    5                    /* CMP #5: a word of 5 syllables or more is shortened */

/* $CEF6-$CF17: the pass's state, in the ROM's order. */
static void init_state(fw *f)
{
    fw_wr8(f, FW_PROS_FINAL_LENGTHEN, 0);
    fw_wr8(f, PROS_ACCENT_STATE, 0);
    fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0);
    fw_wr8(f, PROS_WORD_POS, 0);
    fw_wr8(f, PROS_FEATURES, 0);
    fw_wr8(f, FW_PROS_ACCENT_PENDING, 1);
    fw_wr8(f, PROS_PHRASE_LEN, 1);
    fw_wr8(f, FW_PROS_WRITE_ONLY_1C1C, 1);                   /* never read */
    fw_wr8(f, FW_PROS_FINAL, 1);
    fw_wr8(f, FW_PROS_NUCLEUS_MASK, PSF_NUCLEUS);
}

/* $CF2B-$CF41: the position's flags, symbol, class and features. Returns the symbol (TAY). */
static uint8_t load_symbol(fw *f, uint8_t x)
{
    fw_wr8(f, PROS_SYM_FLAG, fw_rd8(f, (uint16_t)(PROS_SYM_FLAGS + x)));
    uint8_t sym = fw_rd8(f, (uint16_t)(TN_PHONETIC + x));
    fw_wr8(f, PROS_SYMBOL, sym);
    fw_wr8(f, PROS_CLASS, fw_rom8(f, (uint16_t)(PH_SYMBOL_CLASS + sym)));
    fw_wr8(f, PROS_FEATURES, fw_rom8(f, (uint16_t)(PH_SYMBOL_FEATURES + sym)));
    return sym;
}

/* $CF6B-$CF8A: a space or punctuation ends the word walked (the one after it in the text): its flags get that
   word's phonetic-symbol count, and the syllable count pros_mark_stress stored here is the next word's. */
static void word_end(fw *f, uint8_t x)
{
    fw_wr8(f, FW_PROS_SYLLABLES, fw_rd8(f, (uint16_t)(PROS_SYM_LENGTHEN + x)));
    fw_wr8(f, (uint16_t)(PROS_SYM_FLAGS + x), fw_rd8(f, PROS_WORD_POS));
    fw_wr8(f, FW_PROS_WRITE_ONLY_1C1C, 1);                   /* never read */
    fw_wr8(f, FW_PROS_FINAL, 1);
    fw_wr8(f, FW_PROS_FINAL_LENGTHEN, 0);
    fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0);
    fw_wr8(f, PROS_PREV_FEATURES, 0);
    fw_wr8(f, PROS_WORD_POS, 0);
}

void pros_load_params(fw *f, uint16_t set)
{
    fw_wr8(f, PROS_STRESS_PARAM, fw_rd8(f, (uint16_t)(set + 2)));
    fw_wr8(f, PROS_NUCLEUS_PARAM, fw_rd8(f, (uint16_t)(set + 3)));
    fw_wr8(f, PROS_FINAL_PARAM, fw_rd8(f, (uint16_t)(set + 4)));
    fw_wr8(f, PROS_BASELINE, fw_rd8(f, set));
    fw_wr8(f, PROS_BASELINE_END, fw_rd8(f, (uint16_t)(set + 1)));
}

/* $CF9A-$D04A: the five parameters of the punctuation's phrase type, and the nucleus mask back to 7; any other
   punctuation keeps the current set and mask ($D001 JMP $D04D). */
static void phrase_params(fw *f, uint8_t sym)
{
    uint16_t set;
    switch (sym) {
    case '?':
        set = PROS_PARAM_SET_K;
        break;
    case '!':
        set = PROS_PARAM_SET_P;
        break;
    case '.': case ';':
        set = PROS_PARAM_SET_A;
        break;
    case ',': case ':':
        set = PROS_PARAM_SET_F;
        break;
    default:
        return;
    }
    pros_load_params(f, set);
    fw_wr8(f, FW_PROS_NUCLEUS_MASK, PSF_NUCLEUS);
}

/* $CF97-$D05A: a punctuation: the count of the phrase after it stored here, its phrase type's parameters, the
   declination step (from that count), and the phrase-final lengthening after one with class bit 3. r: X and Y as
   pros_declination leaves them. */
static void phrase_end(fw *f, fw_regs *r)
{
    pros_store_phrase_len(f);
    phrase_params(f, fw_rd8(f, PROS_SYMBOL));
    pros_declination(f);
    r->x = fw_rd8(f, PROS_POS);                              /* pros_declination_step's LDX pros_pos */
    r->y = pros_declination_y(f);
    uint8_t k = fw_rd8(f, PROS_CLASS) & PHC_KIND_3;          /* AND #8; BEQ */
    if (k)
        fw_wr8(f, FW_PROS_FINAL_LENGTHEN, k);
}

/* $D062-$D0BB: a phonetic symbol counts in its phrase and word; its last syllable, the phrase-final lengthening
   and the shortening of vowels outside them and of long words. */
static void symbol_counts(fw *f, uint8_t x)
{
    fw_inc8(f, PROS_PHRASE_LEN);
    fw_inc8(f, PROS_WORD_POS);
    uint8_t features = fw_rd8(f, PROS_FEATURES);             /* LDY pros_features; TYA */
    if ((features & PHF_CONSONANT) && (fw_rd8(f, PROS_PREV_FEATURES) & (PHF_VOWEL | PHF_LONG))) {
        fw_wr8(f, FW_PROS_FINAL, 0);                         /* a consonant before a vowel or long symbol */
        fw_wr8(f, FW_PROS_FINAL_LENGTHEN, 0);
    }
    if (fw_rd8(f, FW_PROS_FINAL_LENGTHEN))
        fw_add8(f, PROS_SYM_LENGTHEN, x, 8);
    if (features & (PHF_VOWEL | PHF_LONG)) {                 /* TYA; AND #$82 */
        if (fw_rd8(f, FW_PROS_FINAL) == 0)
            fw_add8(f, PROS_SYM_SHORTEN, x, 3);
        if (fw_rd8(f, FW_PROS_FINAL_LENGTHEN) == 0)
            fw_add8(f, PROS_SYM_SHORTEN, x, 7);
    }
    if (fw_rd8(f, FW_PROS_SYLLABLES) >= PROS_LONG_WORD)     /* CMP #5; BCC */
        fw_add8(f, PROS_SYM_SHORTEN, x, 4);
}

/* $D0BE-$D0F7: the accent on the first stressed vowel or long symbol walked while one is pending (BRA $D0CF: a
   stressed symbol without features bit 7 or 1 gets none), then the nucleus mask ORed in; a stressed symbol ends the
   nucleus marks of its phrase. */
static void accent_and_nucleus(fw *f, uint8_t x)
{
    if (fw_rd8(f, FW_PROS_ACCENT_PENDING) && (fw_rd8(f, PROS_SYM_FLAG) & PSF_STRESS)
        && (fw_rd8(f, PROS_FEATURES) & (PHF_VOWEL | PHF_LONG))) {   /* BBS7; BBS1; BRA */
        fw_add8(f, PROS_SYM_LENGTHEN, x, 8);
        fw_wr8(f, (uint16_t)(PROS_SYM_FLAGS + x), PSF_ACCENT | PSF_STRESS);
        fw_wr8(f, FW_PROS_ACCENT_PENDING, 0);
    }
    uint8_t flags = (uint8_t)(fw_rd8(f, (uint16_t)(PROS_SYM_FLAGS + x)) | fw_rd8(f, FW_PROS_NUCLEUS_MASK));
    fw_wr8(f, (uint16_t)(PROS_SYM_FLAGS + x), flags);
    if (flags & PSF_STRESS) {                                /* BBC6 A: the ORed value */
        fw_wr8(f, PROS_ACCENT_STATE, fw_rd8(f, PROS_WORD_POS));
        fw_wr8(f, FW_PROS_NUCLEUS_MASK, 0);
    }
}

/* $CEEC-$CEF5, reached by BEQ from $CF26 when pros_pos reaches 0: the first phrase's count, and pros_sym_flags[0] =
   the first word's phonetic-symbol count. r: A and X as the ROM leaves them at its RTS (Y untouched). */
static void exit_block(fw *f, fw_regs *r)
{
    pros_store_phrase_len(f);
    r->x = fw_rd8(f, PROS_POS);                              /* its LDX pros_pos */
    r->a = fw_rd8(f, PROS_WORD_POS);
    fw_wr8(f, PROS_SYM_FLAGS, r->a);
}

uint8_t pros_store_phrase_len(fw *f)
{
    uint8_t a = fw_rd8(f, PROS_PHRASE_LEN);
    if (fw_rd8(f, FW_PROS_PUNCT_NOT_KIND3) == 0)
        a |= PROS_PHRASE_LEN_MARK;                           /* ORA #$80, in A only */
    uint8_t x = fw_rd8(f, PROS_POS);
    fw_wr8(f, (uint16_t)(PROS_SYM_LENGTHEN + x), a);
    fw_wr8(f, (uint16_t)(PROS_SYM_SHORTEN + x), a);
    fw_wr8(f, PROS_PHRASE_LEN, 0);
    fw_wr8(f, FW_PROS_PUNCT_NOT_KIND3, 0);
    a = fw_rd8(f, PROS_CLASS);
    if (!(a & PHC_KIND_3)) {                                 /* BBS3 A */
        a = 1;
        fw_wr8(f, FW_PROS_PUNCT_NOT_KIND3, a);
    }
    return a;
}

void pros_mark_phrases(fw *f, fw_regs *r)
{
    init_state(f);
    for (;;) {
        r->a = fw_rd8(f, PROS_FEATURES);                     /* $CF1A */
        fw_wr8(f, PROS_PREV_FEATURES, r->a);
        r->x = (uint8_t)(fw_rd8(f, PROS_POS) - 1);           /* DEC pros_pos; LDX pros_pos */
        fw_wr8(f, PROS_POS, r->x);
        if (r->x == 0)
            break;                                           /* BEQ $CEEC */
        fw_par_ready(f, *r);
        r->y = load_symbol(f, r->x);
        uint8_t sym = r->y, cls = fw_rd8(f, PROS_CLASS);
        if (sym == '{') {                                    /* $CF4B: nothing else at a '{' */
            fw_wr8(f, FW_PROS_STRESS_TOGGLE, 1);
            continue;
        }
        if (sym == '~')                                      /* $CF5A: past a function word, an accent again */
            fw_wr8(f, FW_PROS_ACCENT_PENDING, 1);
        if (sym == ' ' || (cls & PHC_PUNCT)) {               /* $CF5F; BBS0 */
            word_end(f, r->x);
            if (sym != ' ')
                phrase_end(f, r);
        } else if (cls & PHC_PHONETIC) {                     /* $D060 BBC7: a marker goes on */
            r->y = fw_rd8(f, PROS_FEATURES);                 /* $D068 LDY pros_features */
            symbol_counts(f, r->x);
            accent_and_nucleus(f, r->x);
        }
    }
    exit_block(f, r);
}
