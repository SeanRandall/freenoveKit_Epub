#include "par_start.h"
#include "frm_build.h"
#include "fw_ops.h"
#include "par_boundary.h"
#include "par_segment.h"
#include "pros_play.h"
#include "pros_seg.h"
#include "data/fw_data.h"

#define PAR_BOUNDARY_START_FORMANT  0x20   /* $BB5A: the start boundary of P1-P4 (FN, F1, F2, F3) */
#define PAR_BOUNDARY_START_OTHER    0x01   /* $BB64: the start boundary of P5-P10 */
#define ISR_SWAP_PHASE_FILL_B       2      /* $B401: isr_frame_b is the fill frame */
#define ISR_SWAP_PHASE_FILL_A       1      /* $B413: isr_frame_a is the fill frame */

/* ---- par_plan_tracks $C0D2 ---- */

/* $C0D4-$C0FC, one parameter (Y = 1..10): par_sequential[Y] = 0 and par_end_val[Y] = par_seg_target[Y]. SEC / SBC:
   par_end_wait[Y] = par_frames_left - par_end_dur[Y], or 0 when bit 7 is set (BPL). SEC / SBC again: when
   par_end_wait[Y] - par_start_dur[Y] has bit 7 clear (BMI), par_steady[Y] = it and par_sequential[Y] = 1;
   otherwise par_steady[Y] is not written. */
static void plan_track(fw *f, fw_regs *r)
{
    uint8_t y = r->y;
    r->a = 0;
    fw_wr8(f, (uint16_t)(PAR_SEQUENTIAL + y), r->a);
    r->a = fw_rd8(f, (uint16_t)(PAR_SEG_TARGET + y));
    fw_wr8(f, (uint16_t)(PAR_END_VAL + y), r->a);
    r->a = (uint8_t)(fw_rd8(f, PAR_FRAMES_LEFT) - fw_rd8(f, (uint16_t)(PAR_END_DUR + y)));
    if (r->a & 0x80)
        r->a = 0;
    fw_wr8(f, (uint16_t)(PAR_END_WAIT + y), r->a);
    r->a = (uint8_t)(r->a - fw_rd8(f, (uint16_t)(PAR_START_DUR + y)));
    if (r->a & 0x80)
        return;
    fw_wr8(f, (uint16_t)(PAR_STEADY + y), r->a);
    r->a = 1;
    fw_wr8(f, (uint16_t)(PAR_SEQUENTIAL + y), r->a);
}

void par_plan_tracks(fw *f, fw_regs *r)
{
    for (r->y = 10; r->y != 0; r->y--)   /* LDY #$0A ... DEY / BNE $C0D4 */
        plan_track(f, r);
}

/* ---- par_next_segment $BCF3 ---- */

void par_next_segment(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PAR_PARAMS_PTR);    /* $BCF3-$BCFC: par_row_ptr = par_params_ptr, P1's target row */
    fw_wr8(f, PAR_ROW_PTR, r->a);
    r->a = fw_rd8(f, PAR_PARAMS_PTR + 1);
    fw_wr8(f, PAR_ROW_PTR + 1, r->a);
    par_read_segment(f, r);              /* $BCFD: leaves X = 0 */
    par_boundary_pass(f, r);             /* $BD00: indexes its stores with that X */
    par_plan_tracks(f, r);               /* $BD03, then RTS $BD06 */
}

/* ---- par_reset_tracks $BB35 ---- */

/* $BB3A-$BB46: par_seg_index = 0 (both bytes) and par_frames_left = 1, so the first frame starts a segment. */
static void reset_index(fw *f)
{
    fw_wr8(f, PAR_SEG_INDEX, 0);
    fw_wr8(f, PAR_SEG_INDEX + 1, 0);
    fw_wr8(f, PAR_FRAMES_LEFT, 1);
}

/* $BB47-$BB59: for X = 10..1: par_next_start_dur[X] = 0 ($171C,X); Y = frm_snapshot_map[X - 1] ($B772,X);
   frm_snap[X - 1] ($16CC,X) = frm_rest_frame[Y]. */
static void reset_snapshot(fw *f)
{
    for (uint8_t x = 10; x != 0; x--) {
        fw_wr8(f, (uint16_t)(PAR_NEXT_START_DUR + x), 0);
        uint8_t y = fw_rom8(f, (uint16_t)(FRM_SNAPSHOT_MAP - 1 + x));
        fw_wr8(f, (uint16_t)(FRM_SNAP - 1 + x), fw_rd8(f, (uint16_t)(FRM_REST_FRAME + y)));
    }
}

/* $BB5A-$BB6D: par_boundary[4..1] = $20 ($16EC,Y), then par_boundary[10..5] = 1 ($16F0,Y for Y = 6..1). */
static void reset_boundaries(fw *f)
{
    for (uint8_t y = 4; y != 0; y--)
        fw_wr8(f, (uint16_t)(PAR_BOUNDARY + y), PAR_BOUNDARY_START_FORMANT);
    for (uint8_t y = 6; y != 0; y--)
        fw_wr8(f, (uint16_t)(PAR_BOUNDARY + 4 + y), PAR_BOUNDARY_START_OTHER);
}

void par_reset_tracks(fw *f, fw_regs *r)
{
    (void)r;                             /* PHA / TXA / PHA / TYA / PHA ... PLA / TAY / PLA / TAX / PLA */
    reset_index(f);
    reset_snapshot(f);
    reset_boundaries(f);
}

/* ---- par_play_start $BAF2 ---- */

/* $BAF8-$BB00: pros_play_target = pros_seg_pitch[0] ($0A00, absolute), then pros_scale_target (A = the entry Y;
   X and Y kept). */
static void first_pitch(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PROS_SEG_PITCH);
    fw_wr8(f, PROS_PLAY_TARGET, r->a);
    pros_scale_target(f, r);
}

/* $BB01-$BB12: par_next_phoneme = par_cur_phoneme = pros_seg_phoneme[0] ($0200, absolute); TAY; both ranks =
   (par_rank_ptr),Y, from the voice table's rank row. */
static void first_phoneme(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PROS_SEG_PHONEME);
    fw_wr8(f, PAR_NEXT_PHONEME, r->a);
    fw_wr8(f, PAR_CUR_PHONEME, r->a);
    r->y = r->a;
    r->a = fw_ind_y(f, PAR_RANK_PTR, r->y);
    fw_wr8(f, PAR_NEXT_RANK, r->a);
    fw_wr8(f, PAR_CUR_RANK, r->a);
}

/* $BB13-$BB34: cmd_ev_pos = $FFFF; when cmd_ev_count is not 0, cmd_ev_pos = cmd_ev_positions[0], low byte then
   high byte. Then A = 0: cmd_ev_next = 0 and pros_pitch_err = 0 (read only by the unreferenced $C131). */
static void first_event(fw *f, fw_regs *r)
{
    r->a = 0xFF;
    fw_wr8(f, CMD_EV_POS, r->a);
    fw_wr8(f, CMD_EV_POS + 1, r->a);
    r->a = fw_rd8(f, CMD_EV_COUNT);
    if (r->a != 0) {
        r->a = fw_rd8(f, CMD_EV_POSITIONS);
        fw_wr8(f, CMD_EV_POS, r->a);
        r->a = fw_rd8(f, CMD_EV_POSITIONS + 1);
        fw_wr8(f, CMD_EV_POS + 1, r->a);
    }
    r->a = 0;
    fw_wr8(f, CMD_EV_NEXT, r->a);
    fw_wr8(f, PROS_PITCH_ERR, r->a);
}

void par_play_start(fw *f, fw_regs *r)
{
    par_reset_tracks(f, r);              /* $BAF2: keeps A, X and Y */
    pros_reset_seg_ptrs(f);              /* $BAF5 */
    r->a = (uint8_t)(PROS_SEG_FLAGS >> 8);   /* its last LDA #$0E; X and Y untouched */
    first_pitch(f, r);
    first_phoneme(f, r);
    first_event(f, r);
}

/* ---- par_setup_voice $B3A4 ---- */

/* $B3A4-$B3BD: par_row_len = par_count. par_table_ptr's low byte, then its high byte, into par_table_copy,
   par_rank_ptr and par_row_ptr (each byte in the ROM's store order). */
static void voice_pointers(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PAR_COUNT);
    fw_wr8(f, PAR_ROW_LEN, r->a);
    r->a = fw_rd8(f, PAR_TABLE_PTR);
    fw_wr8(f, PAR_TABLE_COPY, r->a);
    fw_wr8(f, PAR_RANK_PTR, r->a);
    fw_wr8(f, PAR_ROW_PTR, r->a);
    r->a = fw_rd8(f, PAR_TABLE_PTR + 1);
    fw_wr8(f, PAR_TABLE_COPY + 1, r->a);
    fw_wr8(f, PAR_ROW_PTR + 1, r->a);
    fw_wr8(f, PAR_RANK_PTR + 1, r->a);
}

/* $B3BE-$B3D1: par_next_row (A = the new low byte): par_row2_ptr = table + N. par_next_row again: par_params_ptr =
   table + 2N, P1's target row. par_row_ptr is left there. */
static void row_pointers(fw *f, fw_regs *r)
{
    par_next_row(f, r);
    fw_wr8(f, PAR_ROW2_PTR, r->a);
    r->a = fw_rd8(f, PAR_ROW_PTR + 1);
    fw_wr8(f, PAR_ROW2_PTR + 1, r->a);
    par_next_row(f, r);
    fw_wr8(f, PAR_PARAMS_PTR, r->a);
    r->a = fw_rd8(f, PAR_ROW_PTR + 1);
    fw_wr8(f, PAR_PARAMS_PTR + 1, r->a);
}

/* $B3D2-$B3F4: par_param_stride = 5N. AND #0: both bytes 0 (the low byte's store is overwritten below). A = N, then
   X = 2 times ASL A / ROL the high byte: 4N, its two top bits shifted into the high byte. CLC / ADC N gives the low
   byte, and the high byte gets ADC #0 with that carry. Leaves A = the high byte and X = 0. */
static void param_stride(fw *f, fw_regs *r)
{
    r->a = 0;
    fw_wr8(f, PAR_PARAM_STRIDE, r->a);
    fw_wr8(f, PAR_PARAM_STRIDE + 1, r->a);
    r->a = fw_rd8(f, PAR_ROW_LEN);
    for (r->x = 2; r->x != 0; r->x--) {
        uint8_t c = r->a >> 7;
        r->a = (uint8_t)(r->a << 1);
        fw_wr8(f, PAR_PARAM_STRIDE + 1, (uint8_t)(fw_rd8(f, PAR_PARAM_STRIDE + 1) << 1 | c));
    }
    uint8_t c = 0;
    r->a = fw_adc(r->a, fw_rd8(f, PAR_ROW_LEN), &c);
    fw_wr8(f, PAR_PARAM_STRIDE, r->a);
    r->a = fw_adc(fw_rd8(f, PAR_PARAM_STRIDE + 1), 0, &c);
    fw_wr8(f, PAR_PARAM_STRIDE + 1, r->a);
}

/* $B3F5-$B40C: LDM isr_play_ptr = isr_frame_a (high byte first), isr_fill_ptr = isr_frame_b, isr_swap_phase = 2.
   Then the rest frame is built at frm_rest_frame, copied to the fill frame, and its b11 clamped (the C frame
   builder, M4.5). */
static void first_rest_frame(fw *f, fw_regs *r)
{
    fw_wr8(f, ISR_PLAY_PTR + 1, ISR_FRAME_A >> 8);
    fw_wr8(f, ISR_PLAY_PTR, ISR_FRAME_A & 0xFF);
    fw_wr8(f, ISR_FILL_PTR + 1, ISR_FRAME_B >> 8);
    fw_wr8(f, ISR_FILL_PTR, ISR_FRAME_B & 0xFF);
    fw_wr8(f, ISR_SWAP_PHASE, ISR_SWAP_PHASE_FILL_B);
    frm_build_rest_frame(f, r);                /* returns X = 9 */
    frm_copy_rest_frame(f, r);
    frm_clamp_mark_space(f, r);
}

/* $B40D-$B41B: LDM the low bytes swapped (isr_play_ptr = isr_frame_b, isr_fill_ptr = isr_frame_a) and
   isr_swap_phase = 1. Then the rest frame is copied to the other frame, and its b11 clamped. */
static void second_rest_frame(fw *f, fw_regs *r)
{
    fw_wr8(f, ISR_PLAY_PTR, ISR_FRAME_B & 0xFF);
    fw_wr8(f, ISR_FILL_PTR, ISR_FRAME_A & 0xFF);
    fw_wr8(f, ISR_SWAP_PHASE, ISR_SWAP_PHASE_FILL_A);
    frm_copy_rest_frame(f, r);
    frm_clamp_mark_space(f, r);                /* leaves A = Y = $0B */
}

void par_setup_voice(fw *f, fw_regs *r)
{
    voice_pointers(f, r);
    row_pointers(f, r);
    param_stride(f, r);
    first_rest_frame(f, r);
    second_rest_frame(f, r);
}
