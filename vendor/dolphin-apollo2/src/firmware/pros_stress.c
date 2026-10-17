#include "pros_stress.h"
#include "pros_flags.h"
#include "fw_ops.h"
#include "tn_convert.h"
#include "data/fw_data.h"

#define SYM_NONE  0x0D   /* $CB9D: the look-ahead ran into tn_phon_len: no next symbol (a carriage return) */

static uint8_t sym_class(fw *f, uint8_t sym) { return fw_rom8(f, (uint16_t)(PH_SYMBOL_CLASS + sym)); }
static uint8_t sym_features(fw *f, uint8_t sym) { return fw_rom8(f, (uint16_t)(PH_SYMBOL_FEATURES + sym)); }
static uint8_t phonetic(const fw *f, uint8_t x) { return fw_rd8(f, (uint16_t)(TN_PHONETIC + x)); }   /* $1900,X */

/* $CABD-$CB1C: the pass state, the arrays' position 0, the event queue. r: A and X as at the first isr_par_ready
   (Y is the caller's). */
static void start(fw *f, fw_regs *r)
{
    static const uint16_t zeroed[] = {   /* in the ROM's order */
        PROS_SYMBOL, PROS_FEATURES, PROS_CLASS, PROS_PREV_FEATURES, FW_PROS_PREV_FEATURES_COPY, PROS_WORD_POS,
        PROS_PHRASE_LEN, FW_PROS_SYLLABLES, FW_PROS_VOWEL_SEEN, PROS_FUNC_WORD, FW_PROS_BACKQUOTE,
        FW_PROS_PUNCT_NOT_KIND3, FW_PROS_NUCLEUS_MASK, PROS_SYM_FLAGS, CMD_EV_NEXT,
    };
    for (unsigned i = 0; i < sizeof zeroed / sizeof zeroed[0]; i++)
        fw_wr8(f, zeroed[i], 0);
    r->x = 0;                                           /* TAX */
    fw_wr8(f, PROS_SYM_LENGTHEN, 0);
    fw_wr8(f, PROS_SYM_SHORTEN, 0);
    fw_wr8(f, PROS_POS, 0xFF);
    fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0xFF);
    fw_wr8(f, PROS_ACCENT_STATE, 0xFF);
    fw_wr8(f, CMD_EV_POS, 0xFF);                        /* the low byte only */
    fw_wr8(f, FW_PROS_ACCENT_PENDING, 1);
    fw_wr8(f, FW_PROS_TILDE_SHORTEN, 1);
    r->a = fw_rd8(f, CMD_EV_COUNT);
    if (r->a != 0) {                                    /* the first event's position, both bytes */
        fw_wr8(f, CMD_EV_POS, fw_rd8(f, CMD_EV_POSITIONS));
        r->a = fw_rd8(f, CMD_EV_POSITIONS + 1);
        fw_wr8(f, CMD_EV_POS + 1, r->a);
        fw_wr8(f, FW_UTT_CONTENT, 0xFF);                /* LDM #$FF */
    }
}

/* $CB4B-$CBAA: from position x + 1 up to tn_phon_len, the next symbol with class != 0 (pros_class, the next
   phonetic symbol and its features); a space before it is the next symbol, with features 0. None: $0D and 0. */
static void look_ahead(fw *f, uint8_t x)
{
    uint8_t len = fw_rd8(f, TN_PHON_LEN);
    bool space = false;
    for (;;) {
        if (++x >= len) {                               /* $CB9D */
            fw_wr8(f, FW_PROS_NEXT_PHON, SYM_NONE);
            fw_wr8(f, FW_PROS_NEXT_SYM, SYM_NONE);
            fw_wr8(f, FW_PROS_NEXT_PHON_FEATURES, 0);
            fw_wr8(f, FW_PROS_NEXT_FEATURES, 0);
            return;
        }
        uint8_t sym = phonetic(f, x);
        if (!space && sym == ' ') {                     /* $CB75: then on to the next phonetic symbol ($CB7D) */
            fw_wr8(f, FW_PROS_NEXT_SYM, sym);
            fw_wr8(f, FW_PROS_NEXT_FEATURES, 0);
            space = true;
            continue;
        }
        uint8_t cls = sym_class(f, sym);
        if (cls == 0)
            continue;
        fw_wr8(f, PROS_CLASS, cls);
        uint8_t feat = sym_features(f, sym);
        fw_wr8(f, FW_PROS_NEXT_PHON_FEATURES, feat);
        if (!space)
            fw_wr8(f, FW_PROS_NEXT_FEATURES, feat);
        fw_wr8(f, FW_PROS_NEXT_PHON, phonetic(f, x));
        if (!space)
            fw_wr8(f, FW_PROS_NEXT_SYM, phonetic(f, x));
        return;
    }
}

/* where a block goes on: $CB1F (the next position), $CC21 (TYA: the features decide) or $CC27 (the stress) */
typedef enum { NEXT_POSITION, NEXT_FEATURES, NEXT_STRESS } next_block;

/* $CBAD-$CC1E: the markers. r in: X = pros_pos, A = pros_symbol, Y = pros_features ($CBAD-$CBB3). */
static next_block marker(fw *f, fw_regs *r)
{
    if (r->a == '~') {                                  /* a function word follows: shortened, no accent */
        fw_wr8(f, FW_PROS_TILDE_SHORTEN, 1);
        fw_wr8(f, FW_PROS_ACCENT_PENDING, 0);
        fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0);
        r->a = PSF_FUNC_WORD;
        fw_wr8(f, (uint16_t)(PROS_SYM_FLAGS + r->x), r->a);
        return NEXT_POSITION;
    }
    if (r->a == '|') {
        r->a = fw_rd8(f, FW_PROS_NEXT_PHON_FEATURES);
        if (!(r->a & PHF_VOWEL))
            return NEXT_POSITION;
        r->a = 'R';                                     /* $CBDB: the linking R, before a vowel */
        fw_wr8(f, (uint16_t)(TN_PHONETIC + r->x), r->a);
        fw_wr8(f, PROS_SYMBOL, r->a);
        r->y = r->a;
        r->a = sym_features(f, r->y);
        fw_wr8(f, PROS_FEATURES, r->a);
        r->y = r->a;
        r->a = fw_rd8(f, PROS_SYMBOL);
        return NEXT_STRESS;
    }
    if (r->a == '}') {                                  /* stress on */
        r->a = 0xFF;
        fw_wr8(f, FW_PROS_STRESS_TOGGLE, r->a);
        fw_wr8(f, PROS_ACCENT_STATE, r->a);
        return NEXT_POSITION;
    }
    if (r->a == '{') {                                  /* stress off */
        r->a = 0;
        fw_wr8(f, FW_PROS_STRESS_TOGGLE, r->a);
        return NEXT_POSITION;
    }
    if (r->a == '`') {                                  /* the next ? is a statement */
        r->a = 1;
        fw_wr8(f, FW_PROS_BACKQUOTE, r->a);
        return NEXT_POSITION;
    }
    if (r->a == 'x') {                                  /* an unstressed vowel */
        r->a = 0;
        fw_wr8(f, FW_PROS_STRESS_TOGGLE, r->a);
    }
    return NEXT_FEATURES;
}

/* $CC27-$CC5D: after a vowel in the word, a consonant that is syllabic (features bit 1) or comes before a vowel
   toggles the stress; the toggle to 0 clears the accent state, and a syllabic consonant with no accent state
   clears the stress. The symbol after pros_pos is read with no bound ($CC37). */
static void stress_toggle(fw *f)
{
    if (fw_rd8(f, FW_PROS_VOWEL_SEEN) == 0)
        return;
    uint8_t feat = fw_rd8(f, PROS_FEATURES);
    if (!(feat & PHF_CONSONANT))
        return;
    if (!(feat & PHF_LONG)) {
        uint8_t next = phonetic(f, (uint8_t)(fw_rd8(f, PROS_POS) + 1));
        if (!(sym_features(f, next) & PHF_VOWEL))
            return;
    }
    uint8_t t = (uint8_t)(fw_rd8(f, FW_PROS_STRESS_TOGGLE) ^ 0xFF);
    fw_wr8(f, FW_PROS_STRESS_TOGGLE, t);
    if (t == 0)
        fw_wr8(f, PROS_ACCENT_STATE, 0);
    if ((fw_rd8(f, PROS_FEATURES) & PHF_LONG) && fw_rd8(f, PROS_ACCENT_STATE) == 0)
        fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0);
}

void pros_stress_counts(fw *f)
{
    uint8_t x = fw_rd8(f, PROS_POS), y = fw_rd8(f, PROS_FEATURES);   /* $CC60 LDX, LDY */
    if (fw_rd8(f, FW_PROS_STRESS_TOGGLE) && fw_rd8(f, PROS_ACCENT_STATE) && (y & (PHF_VOWEL | PHF_LONG)))
        fw_wr8(f, (uint16_t)(PROS_SYM_FLAGS + x), PSF_STRESS);   /* STA, not ORA */
    if (fw_rd8(f, FW_PROS_ACCENT_PENDING)) {            /* $CC7A */
        fw_wr8(f, FW_PROS_TILDE_SHORTEN, 0);
        fw_add8(f, PROS_SYM_LENGTHEN, x, 4);
    }
    if (fw_rd8(f, FW_PROS_TILDE_SHORTEN))               /* $CC8D */
        fw_add8(f, PROS_SYM_SHORTEN, x, 3);
    if (fw_rd8(f, FW_PROS_STRESS_TOGGLE) == 0)          /* $CC9B */
        fw_add8(f, PROS_SYM_SHORTEN, x, 5);
    if (fw_rd8(f, PROS_ACCENT_STATE) != 0)              /* $CCA9 */
        return;
    uint16_t at = (uint16_t)(PROS_SYM_SHORTEN + x);
    uint8_t c = 0;                                      /* CLC */
    fw_wr8(f, at, fw_adc(fw_rd8(f, at), 3, &c));        /* $CCB2: its carry reaches $CCD5 (LDA, BBC keep it) */
    if (fw_rd8(f, FW_PROS_SYLLABLES) != 0 && (y & PHF_VOWEL)) {
        fw_add8(f, PROS_SYM_SHORTEN, x, 3);             /* $CCBF, then BRA $CCDA */
    } else if ((y & PHF_LIQUID) && (fw_rd8(f, FW_PROS_NEXT_FEATURES) & PHF_VOWEL)) {   /* $CCCA */
        fw_wr8(f, at, fw_adc(fw_rd8(f, at), 0x0C, &c)); /* $CCD5: no CLC */
    }
}

/* $CCE0-$CD4F: a consonant (features bit 6). r: X = pros_pos, Y = pros_features. Returns true when the next symbol
   is the same: the first of the two becomes | and the pass goes on to the next position (A = '|'). */
static bool consonant(fw *f, fw_regs *r)
{
    uint8_t x = r->x, feat = r->y;
    uint16_t flags = (uint16_t)(PROS_SYM_FLAGS + x);
    if ((feat & PHF_STOP) && (fw_rd8(f, FW_PROS_NEXT_FEATURES) & PHF_STOP))   /* a stop before a stop */
        fw_wr8(f, flags, (uint8_t)(fw_rd8(f, flags) | PSF_UNRELEASED));
    if ((feat & (PHF_NASAL | PHF_LIQUID)) && (fw_rd8(f, PROS_PREV_FEATURES) & PHF_VOWEL)) {   /* $CCF0 */
        /* INX; LDA $9BB4,X: the class table read at the position pos + 1, not at a symbol; the count it lengthens
           is the next position's, which that position clears again (it stays only at pos + 1 = tn_phon_len) */
        uint8_t x1 = (uint8_t)(x + 1);
        if (sym_class(f, x1) & (PHC_KIND_2 | PHC_KIND_1 | PHC_PUNCT))
            fw_add8(f, PROS_SYM_LENGTHEN, x1, 8);
    }
    x = fw_rd8(f, PROS_POS);                            /* $CD0B */
    if (fw_rd8(f, PROS_SYMBOL) == fw_rd8(f, FW_PROS_NEXT_SYM)) {
        r->a = '|';
        fw_wr8(f, (uint16_t)(TN_PHONETIC + x), r->a);
        return true;
    }
    if ((fw_rd8(f, PROS_PREV_FEATURES) & PHF_CONSONANT) && !(fw_rd8(f, PROS_FEATURES) & PHF_LONG))   /* $CD1E */
        fw_add8(f, PROS_SYM_SHORTEN, x, 6);
    if ((fw_rd8(f, FW_PROS_NEXT_PHON_FEATURES) & PHF_CONSONANT) && !(fw_rd8(f, PROS_FEATURES) & PHF_LONG))
        fw_add8(f, PROS_SYM_SHORTEN, x, 6);
    if (fw_rd8(f, FW_PROS_VOWEL_SEEN))                  /* $CD44 */
        fw_add8(f, PROS_SYM_SHORTEN, x, 3);
    return false;
}

/* $CD5E-$CE95: a vowel (features != 0, bit 6 clear). A syllable unless a vowel follows; then the counts from the
   stress, the neighbours and the consonants after it. r out: X = pros_pos, Y = the next symbol's features. */
static void vowel(fw *f, fw_regs *r)
{
    uint8_t x = r->x;
    uint8_t next = fw_rd8(f, FW_PROS_NEXT_FEATURES);
    if (next == 0 || (next & PHF_CONSONANT))            /* INC: 8 bits */
        fw_inc8(f, FW_PROS_SYLLABLES);
    fw_wr8(f, FW_PROS_VOWEL_SEEN, 1);
    if (fw_rd8(f, PROS_SYMBOL) == 'x')
        fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0);
    if (fw_rd8(f, FW_PROS_STRESS_TOGGLE) != 0) {        /* $CD81: stressed after an unvoiced stop */
        uint8_t p = fw_rd8(f, FW_PROS_PREV_FEATURES_COPY) & (PHF_STOP | PHF_VOICED);
        if (!(p & PHF_VOICED) && (p & PHF_STOP))
            fw_add8(f, PROS_SYM_LENGTHEN, x, 5);
    }
    if (fw_rd8(f, PROS_PREV_FEATURES) & PHF_VOWEL)      /* $CD93 */
        fw_add8(f, PROS_SYM_SHORTEN, x, 6);
    if (fw_rd8(f, FW_PROS_NEXT_PHON_FEATURES) & PHF_VOWEL)
        fw_add8(f, PROS_SYM_LENGTHEN, x, 4);
    uint8_t s = fw_rd8(f, FW_PROS_NEXT_PHON);           /* $CDAF */
    if (s == '1' || s == 'L') {                         /* the symbol after it becomes the next phonetic one */
        x = (uint8_t)(x + 2);
        s = phonetic(f, x);
        fw_wr8(f, FW_PROS_NEXT_PHON, s);
        fw_wr8(f, FW_PROS_NEXT_PHON_FEATURES, sym_features(f, s));
        fw_wr8(f, FW_PROS_WRITE_ONLY_1C12, 1);
        x++;
    } else {
        fw_wr8(f, FW_PROS_WRITE_ONLY_1C12, 0);
        x = (uint8_t)(x + 2);
    }
    uint8_t y;
    for (;;) {                                          /* $CDD9: no bound, X wraps within page $19 */
        y = phonetic(f, x);
        if (!(sym_features(f, y) & PHF_CONSONANT))
            break;
        x++;
    }
    uint8_t final = sym_class(f, y) & PHC_PUNCT;
    fw_wr8(f, FW_PROS_FINAL, final);
    x = fw_rd8(f, PROS_POS);                            /* $CDED */
    r->x = x;
    r->y = fw_rd8(f, FW_PROS_NEXT_FEATURES);
    y = r->y;
    if (y & PHF_STOP) {
        if (y & PHF_VOICED)
            fw_add8(f, PROS_SYM_LENGTHEN, x, final ? 4 : 1);
        else
            fw_add8(f, PROS_SYM_SHORTEN, x, final ? 6 : 2);   /* $CE15 */
    } else if ((y & (PHF_FRICATIVE | PHF_VOICED)) == (PHF_FRICATIVE | PHF_VOICED)) {   /* $CE32 */
        fw_add8(f, PROS_SYM_LENGTHEN, x, final ? 10 : 4);
    } else if (y & PHF_NASAL) {                         /* $CE56 */
        fw_add8(f, PROS_SYM_SHORTEN, x, final ? 6 : 1);
    } else if (fw_rd8(f, FW_PROS_NEXT_FEATURES) == 0) { /* $CE76: the end of the word or text */
        fw_add8(f, PROS_SYM_LENGTHEN, x, final ? 4 : 1);
    }
}

/* $CD52-$CD5B: the common tail. A = pros_features at the JMP $CB1F. */
static void save_features(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PROS_FEATURES);
    fw_wr8(f, PROS_PREV_FEATURES, r->a);
    fw_wr8(f, FW_PROS_PREV_FEATURES_COPY, r->a);
}

/* $CE98-$CEE9: a symbol with features 0 that is no marker (a space, punctuation): the word ends. Its syllable count
   goes into both count arrays. r: X = pros_pos; A = $FF at the JMP $CB1F. */
static void word_end(fw *f, fw_regs *r)
{
    uint8_t x = r->x;
    uint8_t sym = fw_rd8(f, PROS_SYMBOL);
    if (sym == ' ') {
        fw_wr8(f, FW_PROS_ACCENT_PENDING, fw_rd8(f, FW_PROS_TILDE_SHORTEN));
    } else {
        if (sym == '?' && fw_rd8(f, FW_PROS_BACKQUOTE)) {   /* after a backquote: a statement */
            fw_wr8(f, (uint16_t)(TN_PHONETIC + x), '.');
            fw_wr8(f, FW_PROS_BACKQUOTE, 0);
        }
        /* pros_class is the look-ahead's: the next symbol with class != 0, or this one's at the text's end */
        if (fw_rd8(f, PROS_CLASS) & PHC_KIND_1)
            fw_wr8(f, FW_PROS_BACKQUOTE, 0);
        fw_wr8(f, FW_PROS_ACCENT_PENDING, 1);
    }
    uint8_t n = fw_rd8(f, FW_PROS_SYLLABLES);            /* $CECA */
    fw_wr8(f, (uint16_t)(PROS_SYM_LENGTHEN + x), n);
    fw_wr8(f, (uint16_t)(PROS_SYM_SHORTEN + x), n);
    fw_wr8(f, FW_PROS_SYLLABLES, 0);
    fw_wr8(f, FW_PROS_TILDE_SHORTEN, 0);
    fw_wr8(f, FW_PROS_VOWEL_SEEN, 0);
    fw_wr8(f, FW_PROS_PREV_FEATURES_COPY, 0);
    r->a = 0xFF;
    fw_wr8(f, FW_PROS_STRESS_TOGGLE, r->a);
    fw_wr8(f, PROS_ACCENT_STATE, r->a);
}

/* $CB2D-$CEE9: one position. r in: X = pros_pos; r out: A, X, Y as at the JMP $CB1F. */
static void position(fw *f, fw_regs *r)
{
    uint8_t x = r->x;
    fw_wr8(f, (uint16_t)(PROS_SYM_FLAGS + x), 0);
    fw_wr8(f, (uint16_t)(PROS_SYM_LENGTHEN + x), 0);
    fw_wr8(f, (uint16_t)(PROS_SYM_SHORTEN + x), 0);
    uint8_t sym = phonetic(f, x);
    fw_wr8(f, PROS_SYMBOL, sym);
    fw_wr8(f, PROS_CLASS, sym_class(f, sym));
    fw_wr8(f, PROS_FEATURES, sym_features(f, sym));
    look_ahead(f, x);
    r->x = fw_rd8(f, PROS_POS);                         /* $CBAD */
    r->a = fw_rd8(f, PROS_SYMBOL);
    r->y = fw_rd8(f, PROS_FEATURES);
    next_block next = marker(f, r);
    if (next == NEXT_POSITION)
        return;
    if (next == NEXT_FEATURES) {
        r->a = r->y;                                    /* $CC21 TYA */
        if (r->a == 0) {
            word_end(f, r);
            return;
        }
    }
    stress_toggle(f);
    pros_stress_counts(f);
    r->x = fw_rd8(f, PROS_POS);                         /* as $CC60 left them */
    r->y = fw_rd8(f, PROS_FEATURES);
    if (r->y & PHF_CONSONANT) {                         /* $CCDA */
        if (consonant(f, r))
            return;
    } else {
        vowel(f, r);
    }
    save_features(f, r);
}

void pros_mark_stress(fw *f, fw_regs *r)
{
    start(f, r);
    for (;;) {
        fw_par_ready(f, *r);                            /* $CB1F */
        fw_inc8(f, PROS_POS);
        r->x = fw_rd8(f, PROS_POS);
        if (r->x >= fw_rd8(f, TN_PHON_LEN))
            return;                                     /* $CB2C RTS */
        position(f, r);
    }
}
