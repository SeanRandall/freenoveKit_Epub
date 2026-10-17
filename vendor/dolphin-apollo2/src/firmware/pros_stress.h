#ifndef FW_PROS_STRESS_H
#define FW_PROS_STRESS_H
/* The stress pass of the prosody (ROM $CABD-$CEEB), decompiled (M4.3b spec 2.2): the first of the two passes over
   tn_phonetic that pros_make_segments runs before the phrase generator. */
#include "fw.h"

/* $CABD: clears the pass state; cmd_ev_next = 0 and cmd_ev_pos = $FF in its low byte, or, when cmd_ev_count is not
   0, both bytes of the first event's position and $6B = $FF. Then isr_par_ready ($CB1F) before each INC pros_pos
   and the end test, so tn_phon_len + 1 calls (the last one, after position tn_phon_len - 1, exits), and per
   position from 0 to tn_phon_len - 1: the position's flags and counts cleared; the next symbol and the next
   phonetic symbol looked up; the markers ~ | } { ` x; the stress toggle and the stress flag; the shortening and
   lengthening counts from the context (8 bits each); a word's syllable count stored at the space or
   punctuation that ends it. tn_phonetic is rewritten in place: | before a vowel becomes R, the first of two equal
   consonants becomes |, a ? after a backquote becomes a full stop.
   r in: the registers at entry (Y reaches isr_par_ready at the first position; A and X are written first).
   r out: the A, X and Y the ROM leaves at its RTS ($CB2C): A and Y as at the last isr_par_ready, X = pros_pos
   (tn_phon_len). r->p is not used. */
void pros_mark_stress(fw *f, fw_regs *r);

/* $CC60-$CCD7, one block of pros_mark_stress, public only for its unit test (M4.3b spec 4.4: the carry into $CCD5,
   which pros_mark_stress itself never sets, because it clears the position's counts first). At pros_pos, from
   pros_features: the stress flag, then the counts from the stress state. The ADC #$0C at $CCD5 has no CLC: it
   adds the carry out of the ADC #3 at $CCB2. */
void pros_stress_counts(fw *f);

#endif
