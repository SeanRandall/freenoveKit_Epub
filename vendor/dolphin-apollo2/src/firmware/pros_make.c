#include "pros_make.h"
#include "pros_gen.h"
#include "pros_phrase.h"
#include "pros_seg.h"
#include "pros_stress.h"

void pros_make_segments(fw *f, fw_regs *r)
{
    pros_load_params(f, PROS_PARAM_SET_A);              /* $CA92-$CAAD: a statement's parameters (@R A-E) */
    pros_reset_seg_ptrs(f);                             /* $CAB0 */
    r->a = (uint8_t)(PROS_SEG_FLAGS >> 8);              /* its last LDA #$0E; X and Y untouched */
    pros_mark_stress(f, r);                             /* $CAB3: the entry Y reaches its first isr_par_ready */
    pros_mark_phrases(f, r);                            /* $CAB6: pros_mark_stress's Y reaches its first one */
    r->a = pros_phrase_gen(f, &r->x, &r->y);            /* $CAB9 (it reads no register), then RTS $CABC */
}
