#include "par_segment.h"
#include "pros_play.h"
#include "fw_ops.h"

/* par_scale_voicing, one helper per @B block that keeps a term in FW_TMP_76; v is not 0 at @B2 and @B3 (their
   BEQ $BFD1 gives 2 first), and may be 0 at @B4, @B6 and @B7. */

/* $BFE5-$BFEE (@B2): v/4 in FW_TMP_76, then v/16 + FW_TMP_76 + 1 (SEC; ADC). */
static uint8_t voicing_b2(fw *f, uint8_t v)
{
    fw_wr8(f, FW_TMP_76, (uint8_t)(v >> 2));
    uint8_t c = 1;                                           /* SEC */
    return fw_adc((uint8_t)(v >> 4), fw_rd8(f, FW_TMP_76), &c);
}

/* $BFF6-$BFFE (@B3): v/4 in FW_TMP_76, then v/8 + FW_TMP_76 (CLC; ADC). */
static uint8_t voicing_b3(fw *f, uint8_t v)
{
    fw_wr8(f, FW_TMP_76, (uint8_t)(v >> 2));
    uint8_t c = 0;                                           /* CLC */
    return fw_adc((uint8_t)(v >> 3), fw_rd8(f, FW_TMP_76), &c);
}

/* $C004-$C014 (@B4): v/4 in FW_TMP_76; v/8 (PHA) + FW_TMP_76 back into FW_TMP_76; then v/16 (PLA; LSR) +
   FW_TMP_76. */
static uint8_t voicing_b4(fw *f, uint8_t v)
{
    fw_wr8(f, FW_TMP_76, (uint8_t)(v >> 2));
    uint8_t c = 0;                                           /* CLC */
    fw_wr8(f, FW_TMP_76, fw_adc((uint8_t)(v >> 3), fw_rd8(f, FW_TMP_76), &c));
    c = 0;                                                   /* CLC */
    return fw_adc((uint8_t)(v >> 4), fw_rd8(f, FW_TMP_76), &c);
}

/* $C021-$C029 (@B6): v/2 in FW_TMP_76, then v/8 + FW_TMP_76. */
static uint8_t voicing_b6(fw *f, uint8_t v)
{
    fw_wr8(f, FW_TMP_76, (uint8_t)(v >> 1));
    uint8_t c = 0;                                           /* CLC */
    return fw_adc((uint8_t)(v >> 3), fw_rd8(f, FW_TMP_76), &c);
}

/* $C02F-$C036 (@B7): v/2 in FW_TMP_76, then v/4 + FW_TMP_76. */
static uint8_t voicing_b7(fw *f, uint8_t v)
{
    fw_wr8(f, FW_TMP_76, (uint8_t)(v >> 1));
    uint8_t c = 0;                                           /* CLC */
    return fw_adc((uint8_t)(v >> 2), fw_rd8(f, FW_TMP_76), &c);
}

void par_next_row(fw *f, fw_regs *r)
{
    uint8_t c = 0;                                           /* CLC */
    r->a = fw_adc(fw_rd8(f, PAR_ROW_PTR), fw_rd8(f, PAR_ROW_LEN), &c);
    fw_wr8(f, PAR_ROW_PTR, r->a);
    if (c)                                                   /* BCC $BD13 */
        fw_inc8(f, PAR_ROW_PTR + 1);
}

void par_next_param(fw *f, fw_regs *r)
{
    uint8_t c = 0;                                           /* CLC */
    fw_wr8(f, PAR_ROW_PTR, fw_adc(fw_rd8(f, PAR_ROW_PTR), fw_rd8(f, PAR_PARAM_STRIDE), &c));
    r->a = fw_adc(fw_rd8(f, PAR_ROW_PTR + 1), fw_rd8(f, PAR_PARAM_STRIDE + 1), &c);
    fw_wr8(f, PAR_ROW_PTR + 1, r->a);
}

void par_clamp(fw *f, fw_regs *r)
{
    (void)f;
    if ((uint8_t)(r->a - 1) & 0x80)                          /* CMP #1; BPL $BD2B */
        r->a = PAR_CLAMP_MIN;
    else if (!((uint8_t)(r->a - 0x40) & 0x80))               /* CMP #$40; BPL $BD30 */
        r->a = PAR_CLAMP_MAX;
}

void par_scale_voicing(fw *f, fw_regs *r)
{
    uint8_t v = r->a;
    if (v == 8 || v == 1)                                    /* $BFC3-$BFC9: BEQ $BFD3 */
        return;
    switch (fw_rd8(f, CMD_VOICING)) {                        /* PHA; LDA cmd_voicing; CMP #n; BNE; PLA */
    case 0:                                                  /* $BFD0-$BFD3 */
        r->a = PAR_VOICING_FLOOR;
        break;
    case 1:                                                  /* $BFD8-$BFDD */
        r->a = v == 0 ? PAR_VOICING_FLOOR : (uint8_t)(v >> 2);
        break;
    case 2:                                                  /* $BFE2-$BFEE */
        r->a = v == 0 ? PAR_VOICING_FLOOR : voicing_b2(f, v);
        break;
    case 3:                                                  /* $BFF3-$BFFE */
        r->a = v == 0 ? PAR_VOICING_FLOOR : voicing_b3(f, v);
        break;
    case 4:                                                  /* $C003-$C014: no test for 0 */
        r->a = voicing_b4(f, v);
        break;
    case 5:                                                  /* $C019-$C01B */
        r->a = (uint8_t)(v >> 1);
        break;
    case 6:                                                  /* $C020-$C029 */
        r->a = voicing_b6(f, v);
        break;
    case 7:                                                  /* $C02E-$C036 */
        r->a = voicing_b7(f, v);
        break;
    default:                                                 /* $C037-$C038: PLA, v unchanged */
        break;
    }
}

/* ---- par_read_segment $BD33 (M4.4a Task 6) ---- */

/* $BD33-$BD50: X = 0; the playing segment becomes the current one (rank, then phoneme, through Y); par_seg_index
   += 1, and when its low byte wraps to 0, its high byte and the four playback pointers' high bytes, in the ROM's
   order (phoneme, pitch, duration, flags). */
static void advance(fw *f, fw_regs *r)
{
    r->x = 0;
    r->y = fw_rd8(f, PAR_NEXT_RANK);
    fw_wr8(f, PAR_CUR_RANK, r->y);
    r->y = fw_rd8(f, PAR_NEXT_PHONEME);
    fw_wr8(f, PAR_CUR_PHONEME, r->y);
    fw_inc8(f, PAR_SEG_INDEX);
    if (fw_rd8(f, PAR_SEG_INDEX) != 0)
        return;
    fw_inc8(f, PAR_SEG_INDEX + 1);
    fw_inc8(f, PROS_PHONEME_PTR + 1);
    fw_inc8(f, PROS_PITCH_PTR + 1);
    fw_inc8(f, PROS_DURATION_PTR + 1);
    fw_inc8(f, PROS_FLAGS_PTR + 1);
}

/* $BD51-$BD69: Y = the index's low byte; pros_play_pitch = the old pros_play_target; par_next_phoneme and
   pros_play_target from (pros_phoneme_ptr),Y and (pros_pitch_ptr),Y; then pros_scale_target and
   pros_microprosody, which keep X and Y. */
static void read_next(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_SEG_INDEX);
    r->a = fw_rd8(f, PROS_PLAY_TARGET);
    fw_wr8(f, PROS_PLAY_PITCH, r->a);
    r->a = fw_ind_y(f, PROS_PHONEME_PTR, r->y);
    fw_wr8(f, PAR_NEXT_PHONEME, r->a);
    r->a = fw_ind_y(f, PROS_PITCH_PTR, r->y);
    fw_wr8(f, PROS_PLAY_TARGET, r->a);
    pros_scale_target(f, r);
    pros_microprosody(f, r);
}

/* $BD6A-$BD7F: the duration at index - 1: at Y = 0 the DEC of pros_duration_ptr+1 turns the DEY's wrap to $FF into
   the byte before the page; (pros_duration_ptr),Y goes to par_frames_left and to par_seg_target[X] (X = 0: never
   read); INX; at Y = $FF the INC puts the pointer back. */
static void read_duration(fw *f, fw_regs *r)
{
    if (r->y == 0)
        fw_wr8(f, PROS_DURATION_PTR + 1, (uint8_t)(fw_rd8(f, PROS_DURATION_PTR + 1) - 1));
    r->y--;
    r->a = fw_ind_y(f, PROS_DURATION_PTR, r->y);
    fw_wr8(f, PAR_FRAMES_LEFT, r->a);
    fw_wr8(f, (uint16_t)(PAR_SEG_TARGET + r->x), r->a);
    r->x++;
    if (r->y == 0xFF)
        fw_inc8(f, PROS_DURATION_PTR + 1);
}

/* $BD80-$BDA3: Y = par_cur_phoneme; for X = 1..10: A = (par_row_ptr),Y, the target; unless it is >= $80 (BMI) or
   < 3 (CMP #3 / BCC), CLC / ADC cmd_formant_ofs[X - 1] ($2013,X; 8 bits); par_clamp; at X = 10 (P10)
   par_scale_voicing; par_seg_target[X] = A; INX; par_next_param, the same row of the next parameter. */
static void read_targets(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_CUR_PHONEME);
    do {
        r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);
        if (!(r->a & 0x80) && r->a >= 3)
            r->a = (uint8_t)(r->a + fw_rd8(f, (uint16_t)(CMD_FORMANT_OFS - 1 + r->x)));
        par_clamp(f, r);
        if (r->x == 10)
            par_scale_voicing(f, r);
        fw_wr8(f, (uint16_t)(PAR_SEG_TARGET + r->x), r->a);
        r->x++;
        par_next_param(f, r);
    } while (r->x != 11);
}

/* $BDA4-$BDC1: par_row_ptr = par_params_ptr; for Y = 10..1 the tracks start from the boundary: par_start_dur[Y] =
   par_next_start_dur[Y], par_start_val[Y] = par_line_val[Y] = par_boundary[Y]. Leaves A = par_boundary[1], Y = 0. */
static void start_tracks(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PAR_PARAMS_PTR);
    fw_wr8(f, PAR_ROW_PTR, r->a);
    r->a = fw_rd8(f, PAR_PARAMS_PTR + 1);
    fw_wr8(f, PAR_ROW_PTR + 1, r->a);
    for (r->y = 10; r->y != 0; r->y--) {
        r->a = fw_rd8(f, (uint16_t)(PAR_NEXT_START_DUR + r->y));
        fw_wr8(f, (uint16_t)(PAR_START_DUR + r->y), r->a);
        r->a = fw_rd8(f, (uint16_t)(PAR_BOUNDARY + r->y));
        fw_wr8(f, (uint16_t)(PAR_START_VAL + r->y), r->a);
        fw_wr8(f, (uint16_t)(PAR_LINE_VAL + r->y), r->a);
    }
}

/* $BDC2-$BDD2: pros_seg_count -= 1 in 16 bits (LDX low / DEX / CPX #$FF / DEC high / STX low); LDX #0. */
static void count_down(fw *f, fw_regs *r)
{
    r->x = (uint8_t)(fw_rd8(f, PROS_SEG_COUNT) - 1);
    if (r->x == 0xFF)
        fw_wr8(f, PROS_SEG_COUNT + 1, (uint8_t)(fw_rd8(f, PROS_SEG_COUNT + 1) - 1));
    fw_wr8(f, PROS_SEG_COUNT, r->x);
    r->x = 0;
}

void par_read_segment(fw *f, fw_regs *r)
{
    advance(f, r);
    read_next(f, r);
    read_duration(f, r);
    read_targets(f, r);
    start_tracks(f, r);
    count_down(f, r);
}
