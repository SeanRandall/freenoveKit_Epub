#ifndef FW_PROS_SEG_H
#define FW_PROS_SEG_H
/* The segment appenders of the segment generator (ROM $C9B8-$CA91), decompiled (M4.3a spec 2.2, 2.5). A segment
   is one byte in each of the four arrays pros_seg_phoneme, pros_seg_duration, pros_seg_pitch and pros_seg_flags,
   stored through the page pointers pros_phoneme_ptr..pros_flags_ptr at Y = the low byte of the 16-bit index
   pros_seg_count. When that byte wraps to 0, the high byte and all four pointers move on one page. */
#include "fw.h"

#define PH_PAUSE_PHONEME    0x3B   /* the phoneme of every pause segment */
#define PROS_SEG_FLAGS_GEN  0x80   /* the flags byte of every generated segment (playback never reads it) */

/* $CA71: pros_phoneme_ptr = pros_seg_phoneme ($0200), pros_duration_ptr = pros_seg_duration ($0600),
   pros_pitch_ptr = pros_seg_pitch ($0A00), pros_flags_ptr = pros_seg_flags ($0E00). The ROM leaves A = $0E. */
void pros_reset_seg_ptrs(fw *f);
/* $CA3B: isr_par_ready (C since M4.6b) with A = a, X = x, Y = y first, then one segment: ph_phoneme, ph_duration, and
   pros_last_target = pros_target + cmd_pitch_step (8 bits) as its pitch; flags $80. A, X and Y are preserved
   (isr_par_ready preserves them; the ROM pushes A and Y after it and pulls them at $CA6D). */
void pros_append_segment(fw *f, uint8_t a, uint8_t x, uint8_t y);
/* $CA15: a pause segment of duration a: phoneme $3B (also into ph_phoneme), pitch pros_last_target, flags $80.
   Returns the Y it leaves, the old low byte of pros_seg_count; it leaves A = $80 (its last LDA) and X as it was. */
uint8_t ph_append_pause(fw *f, uint8_t a);
/* $C9B8: a pause segment after the punctuation pros_symbol, as ph_append_pause, of duration cmd_phrase_pause in
   full after . ? !, (cmd_phrase_pause >> 1) & $0F after , ; : and (cmd_phrase_pause >> 2) & $0F after any other
   symbol; a duration of 0 becomes 1 ($C9EC INC A). A, X and Y are preserved (PHA / TYA / PHA, and the exit
   JMP $CA6D into pros_append_segment's PLA / TAY / PLA / RTS). */
void ph_append_phrase_pause(fw *f);

#endif
