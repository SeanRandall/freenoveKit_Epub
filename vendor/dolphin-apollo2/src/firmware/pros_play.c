#include "pros_play.h"
#include "fw_ops.h"

/* $BDD7-$BDF8: d = |pros_play_target - $80| (SEC; SBC #$80; BPL, else EOR #$FF; CLC; ADC #1); FW_PROS_DEV_FRAC =
   FW_PROS_SCALE_FRAC = 0; then twice LSR A / ROR FW_PROS_DEV_FRAC, and FW_PROS_DEV_QUARTER = A. Returns d (TAY). */
static uint8_t split_deviation(fw *f)
{
    uint8_t d = (uint8_t)(fw_rd8(f, PROS_PLAY_TARGET) - 0x80);
    if (d & 0x80)
        d = (uint8_t)((d ^ 0xFF) + 1);
    fw_wr8(f, FW_PROS_DEV_FRAC, 0);
    fw_wr8(f, FW_PROS_SCALE_FRAC, 0);
    uint8_t a = d;
    for (unsigned i = 0; i < 2; i++) {                       /* LDX #2; DEX; BNE */
        fw_wr8(f, FW_PROS_DEV_FRAC, (uint8_t)((a & 1) << 7 | fw_rd8(f, FW_PROS_DEV_FRAC) >> 1));
        a >>= 1;
    }
    fw_wr8(f, FW_PROS_DEV_QUARTER, a);
    return d;
}

/* $BDF9-$BE06: R = cmd_source_prosody & 7; when not below cmd_speed_limit (CMP; BCC, unsigned), cmd_speed_limit & 7.
   *zero = the Z flag the BEQ at $BE06 tests: that of the AND on the limited path, that of the CMP (clear, since
   R < cmd_speed_limit) otherwise. */
static uint8_t limit_r(fw *f, bool *zero)
{
    uint8_t R = fw_rd8(f, CMD_SOURCE_PROSODY) & 7;
    uint8_t limit = fw_rd8(f, CMD_SPEED_LIMIT);
    if (R < limit) {                                         /* BCC $BE06 */
        *zero = false;
        return R;
    }
    R = limit & 7;
    *zero = R == 0;
    return R;
}

/* $BE08-$BE30, $BE58: s. Y = d when R >= 4 (CMP #4; BPL: the N flag of R - 4), else 0. k = R & 3; k = 0 gives s = Y
   ($BE58 TYA). Else FW_PROS_SCALE_DEV = Y, then k times FW_PROS_SCALE_FRAC += FW_PROS_DEV_FRAC (CLC) and
   FW_PROS_SCALE_DEV += FW_PROS_DEV_QUARTER plus that add's carry; ASL FW_PROS_SCALE_FRAC; ADC #0 rounds. */
static uint8_t scale_deviation(fw *f, uint8_t R, uint8_t d)
{
    uint8_t y = ((uint8_t)(R - 4) & 0x80) ? 0 : d;           /* BPL $BE0E; LDY #0 */
    uint8_t k = R & 3;
    if (k == 0)                                              /* AND #3; BEQ $BE58 */
        return y;
    fw_wr8(f, FW_PROS_SCALE_DEV, y);                         /* TAX; STY $1811 */
    uint8_t a = y, c;
    for (; k != 0; k--) {                                    /* $BE16-$BE2A */
        c = 0;                                               /* CLC */
        fw_wr8(f, FW_PROS_SCALE_FRAC, fw_adc(fw_rd8(f, FW_PROS_DEV_FRAC), fw_rd8(f, FW_PROS_SCALE_FRAC), &c));
        a = fw_adc(fw_rd8(f, FW_PROS_SCALE_DEV), fw_rd8(f, FW_PROS_DEV_QUARTER), &c);
        fw_wr8(f, FW_PROS_SCALE_DEV, a);
    }
    uint8_t frac = fw_rd8(f, FW_PROS_SCALE_FRAC);
    c = frac >> 7;                                           /* ASL $1812 */
    fw_wr8(f, FW_PROS_SCALE_FRAC, (uint8_t)(frac << 1));
    return fw_adc(a, 0, &c);                                 /* ADC #0 */
}

/* $BE31-$BE39, $BE4D, $BE5C-$BE67: FW_PROS_SCALE_DEV = s; for a target >= $80 (LDX pros_play_target; BPL) centre + s
   (CLC; ADC), $FF on a carry (ORA #$FF); below, centre - s (SEC; SBC), 4 on a borrow. */
static uint8_t around_centre(fw *f, uint8_t s)
{
    fw_wr8(f, FW_PROS_SCALE_DEV, s);
    uint8_t centre = fw_rd8(f, PROS_PITCH_CENTRE);
    if (fw_rd8(f, PROS_PLAY_TARGET) & 0x80) {
        unsigned sum = (unsigned)centre + s;
        return sum > 0xFF ? 0xFF : (uint8_t)sum;             /* BCS $BE4D */
    }
    return centre >= s ? (uint8_t)(centre - s) : PROS_TARGET_FLOOR;   /* BCS $BE3F */
}

/* $BE3F-$BE45: CMP #4; BPL: when A - 4 has bit 7 set (A = 0-3 or $84-$FF), ORA #4; pros_play_target = A. */
static void store_target(fw *f, uint8_t a)
{
    if ((uint8_t)(a - PROS_TARGET_FLOOR) & 0x80)
        a |= PROS_TARGET_FLOOR;
    fw_wr8(f, PROS_PLAY_TARGET, a);
}

/* $B99F-$B9A7: pros_play_target + 5 (CLC; ADC), stored when it does not carry ($B9A5 BCC $B9B1). Returns A. */
static uint8_t micro_raise(fw *f)
{
    unsigned a = fw_rd8(f, PROS_PLAY_TARGET) + PROS_MICRO_STEP;
    if (a <= 0xFF)
        fw_wr8(f, PROS_PLAY_TARGET, (uint8_t)a);             /* $B9B1 */
    return (uint8_t)a;
}

/* $B9A8-$B9B0: pros_play_target - 5 (SEC; SBC), stored when it does not borrow ($B9AE BCS $B9B1). Returns A. */
static uint8_t micro_lower(fw *f)
{
    uint8_t t = fw_rd8(f, PROS_PLAY_TARGET);
    uint8_t a = (uint8_t)(t - PROS_MICRO_STEP);
    if (t >= PROS_MICRO_STEP)
        fw_wr8(f, PROS_PLAY_TARGET, a);                      /* $B9B1 */
    return a;
}

void pros_scale_target(fw *f, fw_regs *r)
{
    uint8_t d = split_deviation(f);                          /* after TYA; PHA; TXA; PHA */
    bool zero;
    uint8_t R = limit_r(f, &zero);
    uint8_t a;
    if (zero)                                                /* BEQ $BE52 */
        a = fw_rd8(f, PROS_PITCH_CENTRE);
    else
        a = around_centre(f, scale_deviation(f, R, d));
    store_target(f, a);
    r->a = r->y;                                             /* PLA; TAX; PLA; TAY: X and Y restored, A = the entry Y */
}

void pros_microprosody(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, PAR_NEXT_PHONEME);
    switch (r->a) {                                          /* $B983-$B99E, the ROM's order */
    case 0x15: case 0x18: case 0x20: case 0x41:
        r->a = micro_raise(f);
        break;
    case 0x1F: case 0x0D:
        r->a = micro_lower(f);
        break;
    default:                                                 /* $B99E RTS */
        break;
    }
}
