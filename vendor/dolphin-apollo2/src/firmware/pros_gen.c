#include "pros_gen.h"
#include "pros_seg.h"
#include "pros_decl.h"
#include "pros_flags.h"
#include "ph_expand.h"
#include "fw_ops.h"
#include "tn_convert.h"
#include "data/fw_data.h"

/* pros_params read here, by @R letter (index = letter - 'A') */
#define PROS_PARAM_U  (PROS_PARAMS + 20)   /* pros_term_stress of a word's first symbol */
#define PROS_PARAM_V  (PROS_PARAMS + 21)   /* added to pros_term_tail at a word's last symbol */
#define PROS_PARAM_W  (PROS_PARAMS + 22)   /* the accent */
#define PROS_PARAM_Y  (PROS_PARAMS + 24)   /* a later stress of a word */
#define PROS_PARAM_Z  (PROS_PARAMS + 25)   /* pros_term_stress of a word's later symbols */
/* the end of a phoneme list (its other items: the triples, and PH_WORD_PAUSE, the space's word pause of @Q frames) */
#define PH_LIST_END    0xFF

/* $D158: the command events due at pros_pos (low bytes compared) get the segment index pros_seg_count in their
   cmd_ev_positions entry; cmd_ev_pos becomes the next entry's low byte (read even past cmd_ev_count). */
static void rewrite_events(fw *f)
{
    for (;;) {
        uint8_t n = fw_rd8(f, CMD_EV_NEXT);
        if (n >= fw_rd8(f, CMD_EV_COUNT) || fw_rd8(f, PROS_POS) != fw_rd8(f, CMD_EV_POS))
            return;
        uint8_t i = (uint8_t)(n << 1);                  /* TXA; ASL; TAX: an index of 128 or more aliases */
        fw_wr8(f, (uint16_t)(CMD_EV_POSITIONS + i), fw_rd8(f, PROS_SEG_COUNT));
        fw_wr8(f, (uint16_t)(CMD_EV_POSITIONS + 1 + i), fw_rd8(f, PROS_SEG_COUNT + 1));
        fw_wr8(f, CMD_EV_POS, fw_rd8(f, (uint16_t)(CMD_EV_POSITIONS + 2 + i)));
        fw_wr8(f, CMD_EV_POS + 1, 0);
        fw_inc8(f, CMD_EV_NEXT);
    }
}

/* $D1E0: a space or punctuation ends the word. Its flags hold the next word's symbol count. */
static void word_start(fw *f)
{
    fw_wr8(f, PROS_FUNC_WORD, 0);
    fw_wr8(f, PROS_TERM_STRESS, 0);
    fw_wr8(f, FW_PROS_STRESS_TOGGLE, 0);
    fw_wr8(f, PROS_WORD_POS, 0);
    fw_wr8(f, FW_PROS_WORD_U, fw_rd8(f, PROS_PARAM_U));
    fw_wr8(f, PROS_WORD_LEN, fw_rd8(f, PROS_SYM_FLAG));
}

/* $D215-$D2DE: the accent, stress and tail terms of a phonetic symbol. */
static void symbol_terms(fw *f)
{
    uint8_t wp = (uint8_t)(fw_rd8(f, PROS_WORD_POS) + 1);
    fw_wr8(f, PROS_WORD_POS, wp);
    if (wp != 1) {                                      /* after a word's first symbol: Z instead of U */
        fw_wr8(f, FW_PROS_WORD_U, 0);
        fw_wr8(f, PROS_TERM_STRESS, fw_rd8(f, PROS_PARAM_Z));
    }
    fw_wr8(f, PROS_TERM_STRESS, (uint8_t)(fw_rd8(f, PROS_TERM_STRESS) + fw_rd8(f, FW_PROS_WORD_U)));
    fw_wr8(f, PROS_TERM_ACCENT, 0);
    uint8_t flag = fw_rd8(f, PROS_SYM_FLAG);
    if (fw_rd8(f, PROS_FUNC_WORD) == 0) {
        if (flag & PSF_ACCENT) {                        /* $D243 */
            fw_wr8(f, PROS_TERM_ACCENT, fw_rd8(f, PROS_PARAM_W));
            fw_wr8(f, PROS_TERM_TAIL, 0);
            if (fw_rd8(f, PROS_ACCENT_STATE)) {
                fw_wr8(f, PROS_ACCENT_STATE, 0);
            } else {
                fw_wr8(f, FW_PROS_STRESS_TOGGLE, 1);
                fw_wr8(f, PROS_ACCENT_STATE, 1);
            }
        } else if (flag & PSF_STRESS) {                 /* $D269 */
            if (fw_rd8(f, FW_PROS_STRESS_TOGGLE)) {
                fw_wr8(f, PROS_TERM_STRESS, fw_rd8(f, PROS_PARAM_Y));
            } else {
                fw_wr8(f, FW_PROS_STRESS_TOGGLE, 1);
                fw_wr8(f, PROS_TERM_TAIL, 0);
                if (fw_rd8(f, PROS_ACCENT_STATE)) {     /* right after an accent: 0 */
                    fw_wr8(f, PROS_ACCENT_STATE, 0);
                    fw_wr8(f, PROS_TERM_STRESS, 0);
                } else {
                    fw_wr8(f, PROS_TERM_STRESS, fw_rd8(f, PROS_STRESS_PARAM));
                }
            }
        } else if (fw_rd8(f, PROS_ACCENT_STATE) && fw_rd8(f, PROS_TERM_TAIL) == 0) {   /* $D297 */
            fw_wr8(f, PROS_TERM_TAIL, (uint8_t)(0 - fw_rd8(f, PROS_PARAM_W)));         /* LDA tail (0); SEC; SBC W */
        }
    }
    if (flag & PSF_NUCLEUS) {                           /* $D2A8 */
        if (flag & PSF_STRESS) {
            fw_wr8(f, PROS_TERM_STRESS, fw_rd8(f, PROS_NUCLEUS_PARAM));
        } else if (fw_rd8(f, PROS_WORD_LEN) == fw_rd8(f, PROS_WORD_POS)) {
            fw_wr8(f, PROS_TERM_TAIL, fw_rd8(f, PROS_FINAL_PARAM));
            return;                                     /* BRA $D2DF: no V */
        }
    }
    if (fw_rd8(f, PROS_WORD_LEN) == fw_rd8(f, PROS_WORD_POS))   /* $D2CD: the word's last symbol */
        fw_wr8(f, PROS_TERM_TAIL, (uint8_t)(fw_rd8(f, PROS_TERM_TAIL) + fw_rd8(f, PROS_PARAM_V)));
}

/* $D2DF: pros_target, then the consonant's stress reset and the declination of pros_baseline. */
static void pitch_target(fw *f)
{
    uint8_t b = fw_rd8(f, PROS_BASELINE);
    uint8_t c = (b >> 2) & 1;                           /* LSR x3: the carry is the last bit shifted out */
    uint8_t a = (uint8_t)(b >> 3);
    a = fw_adc(a, fw_rd8(f, PROS_TERM_ACCENT), &c);     /* no CLC between: each carry feeds the next ADC */
    a = fw_adc(a, fw_rd8(f, PROS_TERM_STRESS), &c);
    a = fw_adc(a, fw_rd8(f, PROS_TERM_TAIL), &c);
    a = fw_adc(a, 105, &c);
    fw_wr8(f, PROS_TARGET, a);
    if (fw_rd8(f, PROS_FEATURES) & PHF_CONSONANT)      /* a consonant clears pros_term_stress after its target */
        fw_wr8(f, PROS_TERM_STRESS, 0);
    if (fw_rd8(f, PROS_FEATURES) != 0)                  /* a symbol with no features keeps the baseline */
        fw_wr8(f, PROS_BASELINE, (uint8_t)(fw_rd8(f, PROS_BASELINE) - fw_rd8(f, PROS_DECL_STEP)));
}

/* CLC; ADC #1; LSR A: (n + 1) / 2 in 8 bits, so n = $FF gives 0. */
static uint8_t half_count(uint8_t n) { return (uint8_t)((uint8_t)(n + 1) >> 1); }

/* steps lookups of ph_dur_range in the table at table (TAX; LDA table,X; DEY; BNE: 0 steps runs 256).
   *x is left as the last TAX leaves it, the index of the last lookup. */
static void scale_range(fw *f, uint16_t table, uint8_t steps, uint8_t *x)
{
    uint8_t a = fw_rd8(f, PH_DUR_RANGE);
    do {
        *x = a;
        a = fw_rom8(f, (uint16_t)(table + *x));
    } while (--steps != 0);
    fw_wr8(f, PH_DUR_RANGE, a);
}

/* $D363-$D3CC: the triple at list index r->y: ph_phoneme, ph_max_dur, ph_min_dur ($78 = the index of min); the
   range max - min shortened (ph_dur_shorten) and lengthened (ph_dur_lengthen) by the position's counts; min + range,
   at most 255, / 8 through the speed map (cmd_speed_map) is ph_duration; then the segment. */
static void append_triple(fw *f, fw_regs *r)
{
    fw_wr8(f, PH_PHONEME, fw_ind_y(f, PH_LIST_PTR, r->y));
    r->y++;
    fw_wr8(f, PH_MAX_DUR, fw_ind_y(f, PH_LIST_PTR, r->y));
    r->y++;
    fw_wr8(f, PH_MIN_DUR, fw_ind_y(f, PH_LIST_PTR, r->y));
    fw_wr8(f, FW_TMP_78, r->y);
    r->x = fw_rd8(f, PROS_POS);
    fw_wr8(f, PH_DUR_RANGE, (uint8_t)(fw_rd8(f, PH_MAX_DUR) - fw_rd8(f, PH_MIN_DUR)));
    uint8_t n = fw_rd8(f, (uint16_t)(PROS_SYM_SHORTEN + r->x));
    if (n != 0)
        scale_range(f, PH_DUR_SHORTEN, half_count(n), &r->x);
    r->x = fw_rd8(f, PROS_POS);
    n = fw_rd8(f, (uint16_t)(PROS_SYM_LENGTHEN + r->x));
    if (n != 0)
        scale_range(f, PH_DUR_LENGTHEN, ph_lengthen_count(f, half_count(n)), &r->x);
    unsigned d = (unsigned)fw_rd8(f, PH_DUR_RANGE) + fw_rd8(f, PH_MIN_DUR);
    r->y = (uint8_t)((d > 0xFF ? 0xFF : d) >> 3);       /* BCC; LDY #$FF; then LSR x3 */
    r->a = fw_ind_y(f, CMD_SPEED_MAP, r->y);
    fw_wr8(f, PH_DURATION, r->a);
    pros_append_segment(f, r->a, r->x, r->y);
}

/* $D31B ph_decode_symbol: ph_list_ptr = the phoneme list of pros_symbol; a segment per triple, or the space's word
   pause; the list's end goes on to the next position. r: the registers (X = pros_pos on entry), left as the ROM
   has them at its JMP $D152. */
static void decode_symbol(fw *f, fw_regs *r)
{
    /* ASL: a symbol from $80 carries into the next page ($9E74), so the pointer is at ph_list_ptrs + 2 * symbol */
    uint16_t p = (uint16_t)(PH_LIST_PTRS + 2 * fw_rd8(f, PROS_SYMBOL));
    fw_wr8(f, PH_LIST_PTR, fw_rom8(f, p));
    fw_wr8(f, PH_LIST_PTR + 1, fw_rom8(f, (uint16_t)(p + 1)));
    r->y = 0;
    for (;;) {                                          /* $D33F */
        r->a = fw_ind_y(f, PH_LIST_PTR, r->y);
        if (r->a == PH_LIST_END)
            return;
        if (r->a == PH_WORD_PAUSE) {                    /* $D348 */
            r->a = fw_rd8(f, CMD_WORD_PAUSE);
            if (r->a == 0)
                return;                                 /* @Q0: no pause */
            fw_wr8(f, FW_TMP_78, r->y);
            ph_append_pause(f, r->a);
            r->a = PROS_SEG_FLAGS_GEN;                  /* ph_append_pause's last LDA #$80 (the segment flags) */
            r->y = fw_rd8(f, FW_TMP_78);
            r->x = fw_rd8(f, PROS_POS);
            fw_wr8(f, FW_UTT_CONTENT, 0xFF);            /* LDM #$FF */
            return;
        }
        fw_wr8(f, FW_UTT_CONTENT, 0xFF);                /* $D360 LDM #$FF */
        append_triple(f, r);
        r->a = fw_rd8(f, PROS_SYM_FLAG);
        if (r->a & PSF_UNRELEASED)
            return;                                     /* the closure only */
        r->y = (uint8_t)(fw_rd8(f, FW_TMP_78) + 1);
    }
}

uint8_t pros_phrase_gen(fw *f, uint8_t *x, uint8_t *y)
{
    fw_regs r = { 0, 0, 0, 0 };
    r.a = pros_declination(f);
    r.x = fw_rd8(f, PROS_POS);                          /* pros_declination_step's LDX pros_pos */
    r.y = pros_declination_y(f);                        /* pros_declination_step's STY, before any negation */
    fw_wr8(f, PROS_LAST_TARGET, 0x80);
    fw_wr8(f, PROS_TERM_TAIL, 0);
    fw_wr8(f, PROS_ACCENT_STATE, 0);
    fw_wr8(f, PROS_TERM_STRESS, 0);
    fw_wr8(f, FW_PROS_WORD_U, 0);
    fw_wr8(f, PROS_WORD_LEN, fw_rd8(f, PROS_SYM_FLAGS));   /* pros_sym_flags[0]: the first word's symbol count */
    fw_wr8(f, PROS_SEG_COUNT, 0);
    fw_wr8(f, PROS_SEG_COUNT + 1, 0);
    r.a = 0xFF;
    fw_wr8(f, PROS_POS, r.a);
    for (;;) {
        fw_inc8(f, PROS_POS);   /* $D152 */
        fw_par_ready(f, r);
        rewrite_events(f);
        r.x = fw_rd8(f, PROS_POS);
        if (r.x >= fw_rd8(f, TN_PHON_LEN))
            break;
        uint8_t sym = fw_rd8(f, (uint16_t)(TN_PHONETIC + r.x));   /* $D1A2 */
        fw_wr8(f, PROS_SYMBOL, sym);
        r.y = sym;
        fw_wr8(f, PROS_CLASS, fw_rom8(f, (uint16_t)(PH_SYMBOL_CLASS + r.y)));
        fw_wr8(f, PROS_FEATURES, fw_rom8(f, (uint16_t)(PH_SYMBOL_FEATURES + r.y)));
        fw_wr8(f, PROS_SYM_FLAG, fw_rd8(f, (uint16_t)(PROS_SYM_FLAGS + r.x)));
        if (sym == '~')
            fw_wr8(f, PROS_FUNC_WORD, 1);               /* a function word (for hypermode) */
        uint8_t cls = fw_rd8(f, PROS_CLASS);
        if (sym == ' ' || (cls & PHC_PUNCT)) {
            if (sym != ' ' && fw_rd8(f, CMD_PHRASE_PAUSE)) {   /* $D1D6: @D0 adds no pause */
                ph_append_phrase_pause(f);              /* A and Y preserved */
                ph_append_phrase_pause(f);
            }
            word_start(f);
            if (sym != ' ') {                           /* punctuation also ends the accent */
                fw_wr8(f, PROS_TERM_TAIL, 0);
                fw_wr8(f, PROS_ACCENT_STATE, 0);
                r.a = 0;
                continue;                               /* no segment */
            }
        } else {
            r.y = cls;                                  /* $D20F LDY pros_class; TYA */
            r.a = cls;
            if (!(cls & PHC_PHONETIC))
                continue;                               /* a marker: no segment */
            symbol_terms(f);
            pitch_target(f);
        }
        if (fw_rd8(f, CMD_HYPER) && fw_rd8(f, PROS_FUNC_WORD)) {   /* $D312: @H1 skips a function word */
            r.a = fw_rd8(f, PROS_FUNC_WORD);
            continue;
        }
        decode_symbol(f, &r);
    }
    ph_append_pause(f, 4);                              /* $D18E */
    r.y = ph_append_pause(f, PROS_SEG_FLAGS_GEN);       /* the first pause left A = $80: this one lasts $80 frames */
    r.a = fw_rd8(f, PROS_SEG_COUNT);                    /* 16-bit decrement: the index of the last segment */
    if (r.a == 0)
        fw_wr8(f, PROS_SEG_COUNT + 1, (uint8_t)(fw_rd8(f, PROS_SEG_COUNT + 1) - 1));
    fw_wr8(f, PROS_SEG_COUNT, (uint8_t)(fw_rd8(f, PROS_SEG_COUNT) - 1));
    *x = r.x;
    *y = r.y;
    return r.a;
}
