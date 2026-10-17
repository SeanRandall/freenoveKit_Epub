#include "pros_decl.h"

uint8_t pros_declination_step(fw *f)
{
    uint8_t x = fw_rd8(f, PROS_POS);
    uint8_t len = fw_rd8(f, (uint16_t)(PROS_SYM_LENGTHEN + x)) & 0x7F;
    fw_wr8(f, PROS_PHRASE_LEN, len);
    if (len == 0)
        fw_wr8(f, PROS_PHRASE_LEN, ++len);                     /* $D41A INC pros_phrase_len */
    uint8_t y = 0;
    uint8_t a = (uint8_t)(fw_rd8(f, PROS_BASELINE) - fw_rd8(f, PROS_BASELINE_END));   /* SEC; SBC */
    while (a >= len) {                                         /* CMP pros_phrase_len; BCC */
        y++;
        a = (uint8_t)(a - len);                                /* INY; SEC; SBC pros_phrase_len */
    }
    if (y >= PROS_DECL_STEP_MAX)
        y = PROS_DECL_STEP_MAX;                                /* CPY #$19; BCC; LDY #$19 */
    fw_wr8(f, PROS_DECL_STEP, y);
    return y;                                                  /* TYA */
}

uint8_t pros_declination(fw *f)
{
    uint8_t end = fw_rd8(f, PROS_BASELINE_END), base = fw_rd8(f, PROS_BASELINE);
    if (end <= base)                                           /* CMP; BCC / BEQ into the step */
        return pros_declination_step(f);
    fw_wr8(f, PROS_BASELINE_END, base);                        /* PHA; LDA; STA pros_baseline_end; PLA; STA */
    fw_wr8(f, PROS_BASELINE, end);
    pros_declination_step(f);
    fw_wr8(f, PROS_DECL_STEP, (uint8_t)(0 - fw_rd8(f, PROS_DECL_STEP)));   /* LDA #0; SEC; SBC pros_decl_step */
    uint8_t a = fw_rd8(f, PROS_BASELINE_END);                  /* the swap back leaves A = pros_baseline */
    fw_wr8(f, PROS_BASELINE_END, fw_rd8(f, PROS_BASELINE));
    fw_wr8(f, PROS_BASELINE, a);
    return a;
}
