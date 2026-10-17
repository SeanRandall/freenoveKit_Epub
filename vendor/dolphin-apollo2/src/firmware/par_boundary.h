#ifndef FW_PAR_BOUNDARY_H
#define FW_PAR_BOUNDARY_H
/* The boundary pass of the playback set-up (ROM $BE6A-$C0D1 and $C29C-$C2B3), decompiled (M4.4a spec 2.2): at each
   segment, the boundary value and the transition durations of each parameter, by the dominance rules. r in = the
   registers at entry (r->p is not read); r out = the A, X and Y the ROM leaves at its RTS. */
#include "fw.h"

#define PAR_FAST_SPEED      5      /* par_scale_duration: CPY #5, the first speed (cmd_speed) that maps durations */
#define PAR_SPEED_MAP_LAST  0x1F   /* par_scale_duration: CMP #$1F; LDA #$1F, the last index of a 32-byte map */

/* $C29C: a transition duration A. 0 passes, and FW_TMP_78 is not written. Otherwise FW_TMP_78 = Y; at cmd_speed >= 5,
   A = cmd_speed_map[min(A, 31)] (through the pointer at $3A); slower speeds leave A. Y is restored from FW_TMP_78,
   X untouched. */
void par_scale_duration(fw *f, fw_regs *r);

/* The boundary pass ($BE6A-$C0D1, M4.4a spec 2.2), run once per segment by par_next_segment after
   par_read_segment. The running X: the store index of par_store_boundary and par_boundary_value (par_boundary,
   par_start_dur, par_end_dur, par_target_bias, each + X) is the X the pass is entered with, incremented once per
   parameter. par_read_segment leaves it 0 ($BDD0), so in the ROM it always runs 0..9 next to par_param_index; the
   C takes it from r->x and never assumes 0. */

/* $BEB5: X = the store index p: par_boundary[p + 1] = A; when par_start_dur[p + 1] is 0, par_trans_dur is cut to
   par_frames_left if that is smaller; then, when par_trans_dur is 0, par_start_dur[p + 1] is cut to par_frames_left
   if that is smaller (both compares unsigned); par_end_dur[p + 1] = par_trans_dur.
   r in: A, X. r out: A = par_trans_dur, X + 1 (8 bits), Y untouched. */
void par_store_boundary(fw *f, fw_regs *r);
/* $BF6C: X = the parameter p (CPX #9: P10 is scaled by @B), par_ofs = its @u offset, par_row_ptr on the parameter's
   target row: par_dominated_target = par_clamp(target[par_target_phoneme] + par_ofs); par_fixed = the fixed
   contribution of par_dominant_phoneme (P10: par_scale_voicing), plus the weighted offset when
   par_ofs_weight = 8 - proportion (mod 256) is not 0: par_ofs added that many times in 8 bits, >> 3, with bits 7-5
   set when the last add carried; par_prop = the proportion. Three par_next_row calls leave par_row_ptr on the
   external-duration row (row 3). r in: X. r out: A = par_row_ptr's low byte, X = p, Y = par_dominant_phoneme. */
void par_read_dominance(fw *f, fw_regs *r);
/* $C039: X = the store index: the boundary value from par_fixed, par_prop (eighths of the way), par_dominated_target
   and par_target_bias[X]: proportion 0: par_clamp(par_fixed); 8: target + bias (8 bits); a multiple of 4: half of
   that 8-bit sum, rounded; otherwise from halves with chained carries, the 9-bit sum >> 2 rounded. Each but 0 adds
   par_fixed and turns 0 and $40 or more (unsigned) into 1. Stored by par_store_boundary. Writes the halves state
   (par_bias_sum ... par_target_sum). r in: X (Y kept unless the halves path). r out: A = par_trans_dur, X + 1,
   Y = 0 on the halves path (proportion & 3 not 0), else as on entry. */
void par_boundary_value(fw *f, fw_regs *r);
/* $BEE2: the current phoneme dominates (par_dominant_phoneme = par_cur_phoneme, par_target_phoneme =
   par_next_phoneme). For p = par_param_index 0..9: par_ofs = cmd_formant_ofs[p], par_read_dominance, the dominant's
   external duration (row 3) into par_next_start_dur[p + 1] and its internal duration (row 4) into par_trans_dur,
   each through par_scale_duration, then par_boundary_value at the running X, which the loop saves around its body.
   r in: X. r out: A = 10, X + 10, Y as par_boundary_value leaves it. */
void par_boundary_cur_dom(fw *f, fw_regs *r);
/* $BF27: as par_boundary_cur_dom with the next phoneme dominant (par_dominant_phoneme = par_next_phoneme,
   par_target_phoneme = par_cur_phoneme) and the two durations the other way round: row 3 into par_trans_dur,
   row 4 into par_next_start_dur[p + 1]. */
void par_boundary_next_dom(fw *f, fw_regs *r);
/* $BE94: no transitions: par_next_start_dur[1..10] = 0 and par_trans_dur = 0, then par_param_index counts 10 down to
   1: the next phoneme's raw target (no @u offset, no clamp) through par_store_boundary, par_next_param after each.
   par_row_ptr is left ten strides on. r in: X. r out: A = par_row_ptr's high byte, X + 10, Y = par_next_phoneme. */
void par_boundary_jump(fw *f, fw_regs *r);
/* $BE6A: par_next_rank = the next phoneme's rank (par_rank_ptr); both ranks 29 or more: par_boundary_jump; else
   par_boundary_next_dom when par_next_rank >= par_cur_rank, par_boundary_cur_dom when it is lower (unsigned).
   r in: X, the running store index. r out: as the callee leaves them (A going in: par_cur_rank into
   par_boundary_jump, par_next_rank into par_boundary_next_dom and par_boundary_cur_dom). */
void par_boundary_pass(fw *f, fw_regs *r);

#endif
