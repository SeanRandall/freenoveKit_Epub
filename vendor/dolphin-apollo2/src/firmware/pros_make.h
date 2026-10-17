#ifndef FW_PROS_MAKE_H
#define FW_PROS_MAKE_H
/* The prosody of tn_phonetic (ROM $CA92-$CABC, pros_make_segments), decompiled (M4.3b spec 2.1, 2.2). */
#include "fw.h"

/* $CA92: the statement's phrase parameters pros_params[0..4] (@R A-E) into pros_stress_param, pros_nucleus_param,
   pros_final_param, pros_baseline and pros_baseline_end (C, D, E, A, B: the ROM's order); then pros_reset_seg_ptrs,
   pros_mark_stress, pros_mark_phrases and pros_phrase_gen, all four C ("C calls C"), with the registers passed along
   as the ROM has them.
   r in: the registers at entry; only Y is read, by pros_mark_stress's first isr_par_ready ($CB1F; A is written by
   the copies and X by its TAX at $CAEC). r out: the A, X and Y the ROM leaves at its RTS ($CABC), which are
   pros_phrase_gen's. r->p is not used. */
void pros_make_segments(fw *f, fw_regs *r);

#endif
