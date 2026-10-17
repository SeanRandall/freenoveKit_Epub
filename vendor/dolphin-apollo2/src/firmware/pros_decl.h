#ifndef FW_PROS_DECL_H
#define FW_PROS_DECL_H
/* The declination of a phrase (ROM $D3DD-$D43D), decompiled (M4.3a spec 2.2, 2.7): the step by which the pitch
   baseline falls per phonetic symbol, from pros_baseline to pros_baseline_end over the phrase. */
#include "fw.h"

#define PROS_DECL_STEP_MAX  25   /* CPY #$19: the step is capped at 25 */

/* $D40D: pros_phrase_len = pros_sym_lengthen[pros_pos] & $7F, then 1 if that is 0 (stored back: the next phrase's
   count starts from it); pros_decl_step = (pros_baseline - pros_baseline_end, 8 bits) / pros_phrase_len by
   repeated subtraction, at most 25. Returns A = Y = pros_decl_step; the ROM leaves X = pros_pos. */
uint8_t pros_declination_step(fw *f);
/* $D3DD: pros_declination_step when pros_baseline_end <= pros_baseline (falling or level: a tail-branch, so A, X
   and Y are the step's). A rising baseline swaps pros_baseline and pros_baseline_end, takes the step, negates
   pros_decl_step (0 - step) and swaps back; A is then the restored pros_baseline, Y the step before the negation,
   X = pros_pos. Returns A. */
uint8_t pros_declination(fw *f);

/* The Y the ROM pros_declination leaves, from the RAM it leaves: pros_decl_step, or 0 - pros_decl_step (the step
   before its negation) on the rising path, which leaves pros_baseline_end > pros_baseline as it found them. */
static inline uint8_t pros_declination_y(const fw *f)
{
    uint8_t step = fw_rd8(f, PROS_DECL_STEP);
    return fw_rd8(f, PROS_BASELINE_END) > fw_rd8(f, PROS_BASELINE) ? (uint8_t)(0 - step) : step;
}

#endif
