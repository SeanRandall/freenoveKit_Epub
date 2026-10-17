#ifndef FW_PH_EXPAND_H
#define FW_PH_EXPAND_H
/* Phoneme expansion helpers of the segment generator (M4.3a spec 2.2), decompiled. */
#include "fw.h"

/* $D43E: the number of ph_dur_lengthen steps for a half lengthening count a = (count + 1) / 2:
   ph_lengthen_steps[a] ($D44A,a) below 31, else a >> 1. Entry 0 is $D44A, the routine's own RTS opcode ($60),
   so a = 0 gives 96 steps. The ROM returns it in Y (and A). */
uint8_t ph_lengthen_count(fw *f, uint8_t a);

#endif
