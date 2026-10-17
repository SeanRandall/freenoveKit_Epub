#ifndef FW_PAR_START_H
#define FW_PAR_START_H
/* The playback set-up's top level (ROM $B3A4-$B41C, $BAF2-$BB73, $BCF3-$BD06 and $C0D2-$C0FE), decompiled (M4.4a
   spec 2.1, 2.2): the voice set-up, the utterance start, and the per-segment step that the frame loop par_play
   (C since M4.4b) calls. Each routine takes the registers at entry in r and leaves in r the A, X and Y that the
   ROM leaves at its RTS. r->p is not used: no caller reads a flag. */
#include "fw.h"

/* $C0D2: for P10..P1 (Y = 10..1): par_sequential = 0 and par_end_val = par_seg_target. par_end_wait =
   par_frames_left - par_end_dur, or 0 when that 8-bit difference is negative (BPL, signed). When par_end_wait -
   par_start_dur is not negative (BMI, signed), par_steady = it and par_sequential = 1; otherwise par_steady keeps
   its old value. r in: not read. r out: A = the last difference or 1, X unchanged, Y = 0. */
void par_plan_tracks(fw *f, fw_regs *r);

/* $BCF3: par_row_ptr = par_params_ptr, then par_read_segment, par_boundary_pass and par_plan_tracks (all C), with
   the registers flowing from one to the next: par_read_segment's X = 0 is the running index of the boundary pass's
   stores. r in: not read. r out: as par_plan_tracks leaves them. */
void par_next_segment(fw *f, fw_regs *r);

/* $BB35: par_seg_index = 0 and par_frames_left = 1. For X = 10..1, par_next_start_dur[X] = 0 and frm_snap[X - 1] =
   frm_rest_frame[frm_snapshot_map[X - 1]]. par_boundary[1..4] = $20 and par_boundary[5..10] = 1. A, X and Y are
   preserved (PHA / PLA), so r is left as it is. */
void par_reset_tracks(fw *f, fw_regs *r);

/* $BAF2: par_reset_tracks and pros_reset_seg_ptrs. pros_play_target = pros_seg_pitch[0], scaled by
   pros_scale_target (no microprosody). par_next_phoneme = par_cur_phoneme = pros_seg_phoneme[0], and par_next_rank
   = par_cur_rank = its rank, (par_rank_ptr),Y. cmd_ev_pos = $FFFF, or cmd_ev_positions[0] when cmd_ev_count is not
   0. cmd_ev_next = pros_pitch_err = 0. r in: X passes through (nothing here writes it). r out: A = 0, X unchanged,
   Y = pros_seg_phoneme[0]. */
void par_play_start(fw *f, fw_regs *r);

/* $B3A4: the voice set-up for the voice table at par_table_ptr with par_count (N) phonemes. par_row_len = N.
   par_table_copy (never read), par_rank_ptr and par_row_ptr = the table. Two par_next_row calls give par_row2_ptr
   (never read) = table + N and par_params_ptr = table + 2N. par_param_stride = 5N in 16 bits. Then the rest frames:
   with isr_play_ptr = isr_frame_a, isr_fill_ptr = isr_frame_b and isr_swap_phase = 2, frm_build_rest_frame,
   frm_copy_rest_frame and frm_clamp_mark_space; with the two buffers swapped and isr_swap_phase = 1,
   frm_copy_rest_frame and frm_clamp_mark_space again. These are the C frame builder's (M4.5), called directly with
   the registers flowing from one to the next as the ROM's do.
   r in: Y passes through to frm_build_rest_frame, which does not read it (LDY #$14 first). r out: the
   registers the last call leaves: A = Y = $0B (frm_clamp_mark_space) and X = 9 (frm_build_rest_frame's). */
void par_setup_voice(fw *f, fw_regs *r);

#endif
