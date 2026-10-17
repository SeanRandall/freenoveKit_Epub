#ifndef FW_PROS_PLAY_H
#define FW_PROS_PLAY_H
/* The pitch target at playback (ROM $BDD3-$BE69 and $B983-$B9B4), decompiled (M4.4a spec 2.2): each segment's
   pitch target, scaled around the @F centre by @R, then moved by the microprosody of the phoneme starting to play.
   r in = the registers at entry (r->p is not read); r out = the A, X and Y the ROM leaves at its RTS. */
#include "fw.h"

#define PROS_TARGET_FLOOR  4   /* pros_scale_target: LDA #4 on a borrow; CMP #4 / ORA #4 sets bit 2 below it */
#define PROS_MICRO_STEP    5   /* pros_microprosody: ADC #5 / SBC #5 */

/* $BDD3: pros_play_target scaled by R/4 around pros_pitch_centre. d = |pros_play_target - $80| ($80 for a target
   of 0) is split into FW_PROS_DEV_QUARTER = d >> 2 and FW_PROS_DEV_FRAC = (d & 3) << 6. R = cmd_source_prosody & 7,
   or cmd_speed_limit & 7 when R is not below cmd_speed_limit. When that limited R is 0, the result is the centre
   and FW_PROS_SCALE_DEV is not written; R = 0 below the limit goes on (the BEQ at $BE06 tests the Z of the CMP) and
   writes FW_PROS_SCALE_DEV = 0. Otherwise s = (d when R >= 4, else 0) + (R & 3) chained adds of the quarter and the
   fraction bits, rounded by bit 7 of FW_PROS_SCALE_FRAC; FW_PROS_SCALE_DEV = s. The result is centre + s ($FF on a
   carry) for a target >= $80, centre - s (4 on a borrow) below; a result of 0-3 or $84-$FF gets bit 2 (CMP #4;
   BPL; ORA #4); pros_play_target = the result. X and Y are preserved (PHA / PLA); A = the entry Y. */
void pros_scale_target(fw *f, fw_regs *r);
/* $B983: for par_next_phoneme $15, $18, $20 or $41, A = pros_play_target + 5; for $1F or $0D, A =
   pros_play_target - 5; stored only when the add does not carry or the subtract does not borrow (A keeps the
   wrapped value). Any other phoneme: A = the phoneme. X and Y untouched. */
void pros_microprosody(fw *f, fw_regs *r);

#endif
