#ifndef FW_PROS_GEN_H
#define FW_PROS_GEN_H
/* The phrase generator of the segment generator (ROM $D129, with the phoneme decoder ph_decode_symbol at $D31B),
   decompiled (M4.3a spec 2.2). */
#include "fw.h"

/* $D129: walks tn_phonetic from position 0 to tn_phon_len. Per position: isr_par_ready; the command events due
   there get the current segment index; a space or punctuation ends the word (punctuation with @D adds two phrase
   pauses); a phonetic symbol gets its pitch target pros_target. Then the symbol's phoneme list appends one segment
   per (phoneme, max, min) triple, or a word pause (@Q) for the space. At the end two pauses (4 frames, then $80:
   the first pause leaves A = $80) and pros_seg_count back to the index of the last segment.
   Returns the A the ROM leaves (pros_seg_count's low byte before that decrement); *x = pros_pos (tn_phon_len),
   *y = the Y the second pause leaves (pros_seg_count's low byte before it). */
uint8_t pros_phrase_gen(fw *f, uint8_t *x, uint8_t *y);

#endif
