#include "par_boundary.h"
#include "par_segment.h"
#include "fw_ops.h"
#include "data/fw_data.h"

void par_scale_duration(fw *f, fw_regs *r)
{
    if (r->a == 0)                                           /* CMP #0; BEQ $C2B3 */
        return;
    fw_wr8(f, FW_TMP_78, r->y);                              /* STY $78 */
    if (fw_rd8(f, CMD_SPEED) >= PAR_FAST_SPEED) {            /* LDY cmd_speed; CPY #5; BCC $C2B1 */
        uint8_t i = r->a < PAR_SPEED_MAP_LAST ? r->a : PAR_SPEED_MAP_LAST;   /* CMP #$1F; BCC $C2AE; LDA #$1F */
        r->a = fw_ind_y(f, CMD_SPEED_MAP, i);                /* TAY; LDA ($3A),Y */
    }
    r->y = fw_rd8(f, FW_TMP_78);                             /* LDY $78 */
}

/* ---- The boundary pass (M4.4a Task 5) ---- */

#define PAR_PARAMS        10     /* P1..P10: CMP #$0A, LDY #$0A */
#define PAR_P10_INDEX     9      /* CPX #9: the voicing parameter P10, scaled by @B */
#define PAR_RANK_JUMP     29     /* CMP #$1D: two ranks from 29 up (the pause 31, rank[$FF] 32) have no transitions */
#define PAR_PROP_WHOLE    8      /* CMP #8: the whole of target + bias */
#define PAR_BOUNDARY_MAX  0x3F   /* CMP #$40: a boundary value from $40 up becomes PAR_BOUNDARY_MIN */
#define PAR_BOUNDARY_MIN  1      /* LDA #1 */

/* $BEB9-$BEC8: when par_start_dur[p + 1] is 0, par_trans_dur is cut to par_frames_left if that is lower (BCS:
   unsigned; equal keeps it). */
static void cut_trans_dur(fw *f, uint8_t x)
{
    if (fw_rd8(f, (uint16_t)(PAR_START_DUR + 1 + x)) != 0)
        return;
    uint8_t left = fw_rd8(f, PAR_FRAMES_LEFT);
    if (left < fw_rd8(f, PAR_TRANS_DUR))
        fw_wr8(f, PAR_TRANS_DUR, left);
}

/* $BEC9-$BED9: when par_trans_dur is (still) 0, par_start_dur[p + 1] is cut to par_frames_left if that is lower. */
static void cut_start_dur(fw *f, uint8_t x)
{
    if (fw_rd8(f, PAR_TRANS_DUR) != 0)
        return;
    uint8_t left = fw_rd8(f, PAR_FRAMES_LEFT);
    if (left < fw_rd8(f, (uint16_t)(PAR_START_DUR + 1 + x)))
        fw_wr8(f, (uint16_t)(PAR_START_DUR + 1 + x), left);
}

void par_store_boundary(fw *f, fw_regs *r)
{
    fw_wr8(f, (uint16_t)(PAR_BOUNDARY + 1 + r->x), r->a);     /* $BEB5 STA $16ED,X */
    cut_trans_dur(f, r->x);
    cut_start_dur(f, r->x);
    r->a = fw_rd8(f, PAR_TRANS_DUR);                           /* $BEDA-$BEE0 */
    fw_wr8(f, (uint16_t)(PAR_END_DUR + 1 + r->x), r->a);
    r->x++;
}

/* $BF6C-$BF7B: par_dominated_target = par_clamp(target[par_target_phoneme] + par_ofs) (CLC; 8 bits), then the next
   row (the fixed contributions). */
static void dominated_target(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_TARGET_PHONEME);
    r->a = (uint8_t)(fw_ind_y(f, PAR_ROW_PTR, r->y) + fw_rd8(f, PAR_OFS));
    par_clamp(f, r);
    fw_wr8(f, PAR_DOMINATED_TARGET, r->a);
    par_next_row(f, r);
}

/* $BF7E-$BF8D: par_fixed = the dominant phoneme's fixed contribution, scaled by @B for P10, then the next row (the
   proportions). Y is left on par_dominant_phoneme for the rows after. */
static void fixed_contribution(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_DOMINANT_PHONEME);
    r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);
    if (r->x == PAR_P10_INDEX)
        par_scale_voicing(f, r);
    fw_wr8(f, PAR_FIXED, r->a);
    par_next_row(f, r);
}

/* $BF9D-$BFBE: with X saved (TXA; PHA ... PLA; TAX): A = 0 (AND #0), then par_ofs added par_ofs_weight times (LDX;
   BEQ: none when the weight is 0; CLC before each ADC, so 8 bits), >> 3 (PHP; LSR x3; PLP), with bits 7-5 set when
   the last add carried (BCC; ORA #$E0), added to par_fixed (CLC). A is left = X (PLA). */
static void weighted_offset(fw *f, fw_regs *r)
{
    uint8_t n = fw_rd8(f, PAR_OFS_WEIGHT);
    if (n != 0) {
        uint8_t a = 0, c = 0;
        do {
            c = 0;
            a = fw_adc(a, fw_rd8(f, PAR_OFS), &c);
        } while (--n != 0);
        a = (uint8_t)(a >> 3);
        if (c)
            a |= 0xE0;
        c = 0;
        fw_wr8(f, PAR_FIXED, fw_adc(a, fw_rd8(f, PAR_FIXED), &c));
    }
    r->a = r->x;
}

void par_read_dominance(fw *f, fw_regs *r)
{
    dominated_target(f, r);
    fixed_contribution(f, r);
    r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);                     /* $BF90-$BF9A: the proportion */
    fw_wr8(f, PAR_PROP, r->a);
    r->a = (uint8_t)((uint8_t)(r->a - 9) ^ 0xFF);              /* SEC; SBC #9; EOR #$FF: 8 - prop, mod 256 */
    fw_wr8(f, PAR_OFS_WEIGHT, r->a);
    weighted_offset(f, r);
    par_next_row(f, r);                                        /* $BFBF: row 3, the external durations */
}

/* $C039-$C058: the sums 0; x2 = t << 1 and half = x2 >> 2 (ASL; LSR; LSR: bit 7 is lost) of the dominated target t
   and of the bias b = par_target_bias[X] (LDA $B7A7,X). */
static void halves_setup(fw *f, uint8_t x)
{
    fw_wr8(f, PAR_BIAS_SUM, 0);
    fw_wr8(f, PAR_TARGET_SUM, 0);
    uint8_t t2 = (uint8_t)(fw_rd8(f, PAR_DOMINATED_TARGET) << 1);
    fw_wr8(f, PAR_TARGET_X2, t2);
    fw_wr8(f, PAR_TARGET_HALF, (uint8_t)(t2 >> 2));
    uint8_t b2 = (uint8_t)(fw_rom8(f, (uint16_t)(PAR_TARGET_BIAS + x)) << 1);
    fw_wr8(f, PAR_BIAS_X2, b2);
    fw_wr8(f, PAR_BIAS_HALF, (uint8_t)(b2 >> 2));
}

/* $C066-$C09D, proportion & 3 = k not 0: from 0, or from x2 when the proportion has bit 2, k halves added with the
   carry chained from add to add (one CLC before each loop): the target's sum into par_target_sum, the bias's left in
   A (par_bias_sum keeps 0 or x2); then (bias + target) as 9 bits (ROR takes the carry), >> 2 (LSR), rounded by the
   bit shifted out (ADC #0). Y counts down to 0. */
static uint8_t halves(fw *f, fw_regs *r, uint8_t prop)
{
    uint8_t k = prop & 3, a, c;
    r->y = k;                                                  /* TAY */
    if (prop & 4) {
        fw_wr8(f, PAR_TARGET_SUM, fw_rd8(f, PAR_TARGET_X2));
        fw_wr8(f, PAR_BIAS_SUM, fw_rd8(f, PAR_BIAS_X2));
    }
    fw_wr8(f, PAR_PROP_LOW, k);
    a = fw_rd8(f, PAR_TARGET_SUM);
    c = 0;
    do
        a = fw_adc(a, fw_rd8(f, PAR_TARGET_HALF), &c);
    while (--r->y != 0);
    fw_wr8(f, PAR_TARGET_SUM, a);
    r->y = fw_rd8(f, PAR_PROP_LOW);
    a = fw_rd8(f, PAR_BIAS_SUM);
    c = 0;
    do
        a = fw_adc(a, fw_rd8(f, PAR_BIAS_HALF), &c);
    while (--r->y != 0);
    c = 0;
    a = fw_adc(a, fw_rd8(f, PAR_TARGET_SUM), &c);
    a = (uint8_t)(c << 7 | a >> 1);                            /* ROR A */
    c = a & 1;                                                 /* LSR A */
    a = (uint8_t)(a >> 1);
    return fw_adc(a, 0, &c);                                   /* ADC #0 */
}

/* $C0B2-$C0B9, proportion 8: target + bias (CLC; 8 bits, the carry lost before $C09F's CLC). */
static uint8_t sum_whole(fw *f, uint8_t x)
{
    return (uint8_t)(fw_rd8(f, PAR_DOMINATED_TARGET) + fw_rom8(f, (uint16_t)(PAR_TARGET_BIAS + x)));
}

/* $C0C5-$C0CF, proportion & 3 = 0, a multiple of 4 other than 0 and 8 (4, 12, 16, ...; 0 and 8 take their own
   paths first): bias + target (CLC; 8 bits) >> 1 (LSR), rounded (ADC #0). */
static uint8_t sum_half(fw *f, uint8_t x)
{
    uint8_t s = (uint8_t)(fw_rom8(f, (uint16_t)(PAR_TARGET_BIAS + x)) + fw_rd8(f, PAR_DOMINATED_TARGET));
    return (uint8_t)((s >> 1) + (s & 1));
}

/* $C09F-$C0AD: + par_fixed (CLC; 8 bits); 0 becomes 1, and so does $40 or more (CMP #$40; BCC: unsigned). */
static uint8_t limit(fw *f, uint8_t a)
{
    a = (uint8_t)(a + fw_rd8(f, PAR_FIXED));
    if (a == 0)
        a = PAR_BOUNDARY_MIN;
    if (a > PAR_BOUNDARY_MAX)
        a = PAR_BOUNDARY_MIN;
    return a;
}

void par_boundary_value(fw *f, fw_regs *r)
{
    halves_setup(f, r->x);
    uint8_t prop = fw_rd8(f, PAR_PROP);
    if (prop == 0) {                                           /* $C0BC: clamped, no 0 / $40 test */
        r->a = fw_rd8(f, PAR_FIXED);
        par_clamp(f, r);
    } else {
        if (prop == PAR_PROP_WHOLE)
            r->a = sum_whole(f, r->x);
        else if ((prop & 3) == 0)
            r->a = sum_half(f, r->x);
        else
            r->a = halves(f, r, prop);
        r->a = limit(f, r->a);
    }
    par_store_boundary(f, r);                                  /* $C0AE */
}

/* STA $171D,X (X = p: par_next_start_dur[p + 1]) or STA par_trans_dur: where a scaled duration goes. */
static void store_duration(fw *f, const fw_regs *r, bool start)
{
    if (start)
        fw_wr8(f, (uint16_t)(PAR_NEXT_START_DUR + 1 + r->x), r->a);
    else
        fw_wr8(f, PAR_TRANS_DUR, r->a);
}

/* $BEEE-$BF26 (cur_dom) and $BF33-$BF6B (next_dom): par_param_index = 0 (AND #0), then per parameter p, with the
   running X saved (TXA; PHA): X = p, par_ofs = cmd_formant_ofs[p], par_read_dominance (Y = the dominant phoneme), its
   duration rows 3 and 4 through par_scale_duration, par_next_row after each (row 3 into par_next_start_dur[p + 1]
   for cur_dom, into par_trans_dur for next_dom; row 4 the other), X restored (PLA; TAX), par_boundary_value; until
   par_param_index is 10. */
static void dominance_loop(fw *f, fw_regs *r, bool next_dom)
{
    r->a = 0;
    fw_wr8(f, PAR_PARAM_INDEX, r->a);
    do {
        uint8_t running = r->x;
        r->x = fw_rd8(f, PAR_PARAM_INDEX);
        r->a = fw_rd8(f, (uint16_t)(CMD_FORMANT_OFS + r->x));
        fw_wr8(f, PAR_OFS, r->a);
        par_read_dominance(f, r);
        r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);                 /* row 3: the external duration */
        par_scale_duration(f, r);
        store_duration(f, r, !next_dom);
        par_next_row(f, r);
        r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);                 /* row 4: the internal duration */
        par_scale_duration(f, r);
        store_duration(f, r, next_dom);
        par_next_row(f, r);
        r->a = running;
        r->x = running;
        par_boundary_value(f, r);
        fw_inc8(f, PAR_PARAM_INDEX);
        r->a = fw_rd8(f, PAR_PARAM_INDEX);
    } while (r->a != PAR_PARAMS);
}

void par_boundary_cur_dom(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_NEXT_PHONEME);                        /* $BEE2-$BEED */
    fw_wr8(f, PAR_TARGET_PHONEME, r->y);
    r->y = fw_rd8(f, PAR_CUR_PHONEME);
    fw_wr8(f, PAR_DOMINANT_PHONEME, r->y);
    dominance_loop(f, r, false);
}

void par_boundary_next_dom(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_CUR_PHONEME);                         /* $BF27-$BF32 */
    fw_wr8(f, PAR_TARGET_PHONEME, r->y);
    r->y = fw_rd8(f, PAR_NEXT_PHONEME);
    fw_wr8(f, PAR_DOMINANT_PHONEME, r->y);
    dominance_loop(f, r, true);
}

/* $BE94-$BEA4: par_param_index = 10, par_next_start_dur[10..1] = 0 (STA $171C,Y), par_trans_dur = 0, Y = the next
   phoneme. */
static void jump_setup(fw *f, fw_regs *r)
{
    r->y = PAR_PARAMS;
    fw_wr8(f, PAR_PARAM_INDEX, r->y);
    r->a = 0;
    do
        fw_wr8(f, (uint16_t)(PAR_NEXT_START_DUR + r->y), r->a);
    while (--r->y != 0);
    fw_wr8(f, PAR_TRANS_DUR, r->a);
    r->y = fw_rd8(f, PAR_NEXT_PHONEME);
}

void par_boundary_jump(fw *f, fw_regs *r)
{
    jump_setup(f, r);
    do {                                                       /* $BEA7-$BEB2 */
        r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);
        par_store_boundary(f, r);
        par_next_param(f, r);
        fw_wr8(f, PAR_PARAM_INDEX, (uint8_t)(fw_rd8(f, PAR_PARAM_INDEX) - 1));
    } while (fw_rd8(f, PAR_PARAM_INDEX) != 0);
}

void par_boundary_pass(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_NEXT_PHONEME);
    r->a = fw_ind_y(f, PAR_RANK_PTR, r->y);
    fw_wr8(f, PAR_NEXT_RANK, r->a);
    if (r->a >= PAR_RANK_JUMP) {
        r->a = fw_rd8(f, PAR_CUR_RANK);
        if (r->a >= PAR_RANK_JUMP) {                           /* $BE7F */
            par_boundary_jump(f, r);
            return;
        }
    }
    r->a = fw_rd8(f, PAR_NEXT_RANK);                           /* $BE83 */
    if (r->a >= fw_rd8(f, PAR_CUR_RANK))
        par_boundary_next_dom(f, r);
    else
        par_boundary_cur_dom(f, r);
}
