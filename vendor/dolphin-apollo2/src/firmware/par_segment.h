#ifndef FW_PAR_SEGMENT_H
#define FW_PAR_SEGMENT_H
/* The segment reader of the playback set-up and its leaves (ROM $BD07-$BDD2 and $BFC3-$C038), decompiled (M4.4a
   spec 2.2). Every routine takes the registers the ROM has at its entry in r (r->p is not read: no routine here
   reads an entry flag) and returns in r the A, X and Y the ROM leaves at its RTS; a register the routine does not
   touch is left as it came in. */
#include "fw.h"

#define PAR_CLAMP_MIN      1      /* par_clamp: LDA #1 */
#define PAR_CLAMP_MAX      0x3F   /* par_clamp: LDA #$3F */
#define PAR_VOICING_FLOOR  2      /* par_scale_voicing: LDA #2, at @B0, and for 0 at @B1-@B3 */

/* $BD07: par_row_ptr += par_row_len, 16 bits (the next row of N bytes). A = the new low byte; X and Y untouched. */
void par_next_row(fw *f, fw_regs *r);
/* $BD14: par_row_ptr += par_param_stride, 16 bits (the same row of the next parameter, 5N bytes on). A = the new
   high byte; X and Y untouched. */
void par_next_param(fw *f, fw_regs *r);
/* $BD24: A clamped by the N flag of two 8-bit differences (CMP; BPL): 1 when A - 1 has bit 7 set (A = 0 or
   $81-$FF), $3F when A - $40 has bit 7 clear (A = $40-$80), else A (1-$3F). X and Y untouched. */
void par_clamp(fw *f, fw_regs *r);
/* $BFC3: a voicing value v = A scaled by @B (cmd_voicing). 1 and 8 pass at every @B. Otherwise @B0 gives 2;
   @B1 v/4; @B2 v/16 + v/4 + 1; @B3 v/8 + v/4; @B4 v/16 + v/8 + v/4; @B5 v/2; @B6 v/8 + v/2; @B7 v/4 + v/2;
   @B8 and above v. Each term truncates; v = 0 gives 2 at @B1-@B3 (0 at @B4-@B7). FW_TMP_76 is scratch at @B2,
   @B3 (not for v = 0), @B4, @B6 and @B7. A = the result; X and Y untouched. */
void par_scale_voicing(fw *f, fw_regs *r);

/* $BD33: the next segment (M4.4a spec 2.2). The playing segment becomes the current one (par_cur_rank,
   par_cur_phoneme = the next ones). par_seg_index += 1; when its low byte wraps to 0, its high byte and the high
   bytes of pros_phoneme_ptr, pros_pitch_ptr, pros_duration_ptr and pros_flags_ptr move on one page, in that order.
   At the index (Y = its low byte, through the pointers, which point into ROM during a @T effect):
   par_next_phoneme, and pros_play_target, scaled by pros_scale_target and moved by pros_microprosody after
   pros_play_pitch takes the old target. par_frames_left = par_seg_target[0] = the duration at index - 1 (at a low
   byte of 0, pros_duration_ptr+1 is decremented for the read and incremented after it, $BD6E/$BD7E).
   par_seg_target[1..10] = par_cur_phoneme's ten targets from the rows at par_row_ptr (which the caller set to
   par_params_ptr), each plus its @u offset cmd_formant_ofs unless it is >= $80 or < 3, then par_clamp, and P10
   scaled by par_scale_voicing; par_row_ptr moves on one parameter block (par_next_param) per target and is then
   set back to par_params_ptr. For P1..P10, par_start_dur = par_next_start_dur and par_start_val = par_line_val =
   par_boundary. pros_seg_count -= 1 in 16 bits.
   r in: not read (X is set to 0 first, A and Y are loaded). r out: A = par_boundary[1], X = 0 (which the boundary
   pass's stores index with), Y = 0, as the ROM leaves them at its RTS ($BDD2). */
void par_read_segment(fw *f, fw_regs *r);  /* $BD33 */

#endif
