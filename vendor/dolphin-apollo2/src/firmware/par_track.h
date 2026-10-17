#ifndef FW_PAR_TRACK_H
#define FW_PAR_TRACK_H
/* The tracks of the frame loop (ROM $C0FF-$C130 and $C14C-$C29B), decompiled (M4.4b spec 2.2, 2.6): once per frame
   par_play calls par_step_tracks, which steps the ten parameter tracks and the pitch track by util_interp_step and
   writes the frame's parameter bytes b0-b10 into the fill buffer (isr_fill_ptr) through par_slot_map. Each routine
   takes the registers at entry in r and leaves in r the A, X and Y that the ROM leaves at its RTS. r->p is not used:
   none of the six reads an entry flag, and no caller reads a flag after any of them. */
#include "fw.h"

/* $C1A1: one interpolation step. A = the target, Y = the steps left (the callers also store them in
   util_interp_steps, and the target in util_interp_target), util_interp_cur = the current value.
   - Y = 1: A unchanged (the target). Y = 2: (A + util_interp_cur) >> 1 with the carry as bit 7 (CLC; ADC; ROR).
   - Otherwise d = A - util_interp_cur (SEC; SBC). d = 0: A = util_interp_target.
   - d without a borrow: the quotient q by SBC util_interp_steps until one borrows; util_interp_steps is halved in
     RAM (LSR) and the remainder plus that half with bit 7 set takes one off q (BPL).
   - d with a borrow: q (negative) by ADC util_interp_steps until one carries; unless that sum is 0, util_interp_steps
     is halved in RAM and q gains one when the sum minus that half is above 0 (BMI, BEQ).
   - util_interp_quot = q, A = util_interp_cur + q (8 bits).
   util_interp_steps = 0 with d not 0 makes the ROM loop forever (SBC #0 with the carry set, ADC #0 with it clear):
   the C reports a fault at ROM_UTIL_INTERP_STEP instead (fw_fault) and returns with nothing written.
   util_interp_step writes nothing, but without a trap its C callers (track_step) go on to store the A it leaves;
   every thunk sets the trap.
   r in: A, Y. r out: A = the value; X unchanged; Y = 0 (Y = 1 or 2), Y - 2 (d = 0), q (otherwise). */
void util_interp_step(fw *f, fw_regs *r);

/* $C24A: the start track of par_track_overlap, X = the parameter: par_seg_target[X] once par_start_dur[X] is 0;
   otherwise util_interp_steps = par_start_dur[X], which is decremented, and par_start_val[X] takes one step towards
   par_seg_target[X]. r in: X. r out: A = the track's value, X unchanged, Y = 0 (no step) or util_interp_step's Y. */
void par_track_start(fw *f, fw_regs *r);

/* $C26C: the end track of par_track_overlap, X = the parameter. A negative par_end_wait[X] is zeroed first ($C271,
   unreachable: par_plan_tracks stores 0-127). While par_end_wait[X] is not 0 it is decremented and A =
   par_seg_target[X] (Y = the count before the DEC); at 0, util_interp_steps = par_end_dur[X], which is decremented
   (wrapping past 0), and par_end_val[X] takes one step towards par_boundary[X] (Y = util_interp_step's).
   r in: X. r out: A = the track's value, X unchanged, Y as above. */
void par_track_end(fw *f, fw_regs *r);

/* $C14C: X = the parameter, its transitions apart (par_sequential[X] set): while par_start_dur[X] is not 0
   (decremented), par_start_val[X] steps towards par_seg_target[X]; then while par_steady[X] is not 0 (decremented),
   par_seg_target[X] holds; then par_seg_target[X] itself steps towards par_boundary[X] over par_end_dur[X]
   (decremented, wrapping past 0). The value goes to frame byte par_slot_map[X - 1] through (isr_fill_ptr),Y.
   r in: X. r out: A = the value written, X unchanged, Y = the frame byte's index. */
void par_track_sequential(fw *f, fw_regs *r);

/* $C1F2: X = the parameter, its transitions overlapping: par_start_track = par_track_start, par_end_track =
   par_track_end; par_line_val[X] steps towards par_boundary[X] over par_frames_left; frame byte par_slot_map[X - 1]
   = the median of the three, ordered by CMP and BPL / BMI: the sign of the 8-bit difference, not a true signed
   comparison. r in: X. r out: A = the value written, X unchanged, Y = the frame byte's index. */
void par_track_overlap(fw *f, fw_regs *r);

/* $C0FF: for X = 10 down to 1, par_track_sequential when par_sequential[X] is not 0, else par_track_overlap; then the
   pitch track: pros_play_pitch steps towards pros_play_target over par_frames_left, and frame byte b10 =
   pros_play_pitch >> 2. r in: not read (LDX #$0A first; A and Y are loaded before they are read). r out: A =
   pros_play_pitch >> 2, X = 0, Y = 10. */
void par_step_tracks(fw *f, fw_regs *r);

#endif
