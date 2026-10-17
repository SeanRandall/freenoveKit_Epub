#include "frm_build.h"
#include "fw_ops.h"
#include "par_segment.h"
#include "data/fw_data.h"

/* ---- frm_build_rest_frame $B41D ---- */

/* $B41D-$B426: LDA $B782,Y / STA $1FFF,Y for Y = 20 down to 1 (DEY / BNE): frm_rest_frame = frm_template.
   Leaves A = frm_template[0] and Y = 0. */
static void copy_template(fw *f, fw_regs *r)
{
    r->y = FRAME_SIZE;
    do {
        r->a = fw_rom8(f, (uint16_t)(FRM_TEMPLATE - 1 + r->y));
        fw_wr8(f, (uint16_t)(FRM_REST_FRAME - 1 + r->y), r->a);
        r->y--;
    } while (r->y != 0);
}

/* $B428-$B438: Y = the pause phoneme; for X = 1, 3, 5, 7 (INX / INX, CPX #8 / BCC): frm_rest_frame[X] =
   (par_row_ptr),Y, then par_next_param (the same row of the next parameter). Leaves X = 9. */
static void pause_formants(fw *f, fw_regs *r)
{
    r->y = FRM_PAUSE_PHONEME;
    r->x = FRAME_FN;
    do {
        r->a = fw_ind_y(f, PAR_ROW_PTR, r->y);
        fw_wr8(f, (uint16_t)(FRM_REST_FRAME + r->x), r->a);
        par_next_param(f, r);
        r->x += 2;
    } while (r->x < FRAME_F3 + 1);
}

void frm_build_rest_frame(fw *f, fw_regs *r)
{
    copy_template(f, r);
    pause_formants(f, r);
    r->a = fw_rd8(f, PAR_PARAMS_PTR);          /* $B43A-$B444: par_row_ptr = par_params_ptr */
    fw_wr8(f, PAR_ROW_PTR, r->a);
    r->a = fw_rd8(f, PAR_PARAMS_PTR + 1);
    fw_wr8(f, PAR_ROW_PTR + 1, r->a);
}

/* ---- frm_copy_rest_frame $B445 ---- */

void frm_copy_rest_frame(fw *f, fw_regs *r)
{
    r->y = FRAME_SIZE - 1;                     /* LDY #$13 */
    do {
        r->a = fw_rd8(f, (uint16_t)(FRM_REST_FRAME + r->y));
        fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
        r->y--;
    } while (!(r->y & 0x80));                  /* DEY / BPL */
}

/* ---- frm_snapshot $B917 ---- */

/* $B917-$B920: LDA $16CC,Y / STA $16DC,Y for Y = 10 down to 1: frm_snap_prev = frm_snap. */
static void save_snapshot(fw *f, fw_regs *r)
{
    r->y = 10;
    do {
        r->a = fw_rd8(f, (uint16_t)(FRM_SNAP - 1 + r->y));
        fw_wr8(f, (uint16_t)(FRM_SNAP_PREV - 1 + r->y), r->a);
        r->y--;
    } while (r->y != 0);
}

/* $B922-$B92D: for X = 10 down to 1: Y = frm_snapshot_map[X - 1] ($B772,X), frm_snap[X - 1] ($16CC,X) = the
   producer frame's byte Y. */
static void take_snapshot(fw *f, fw_regs *r)
{
    r->x = 10;
    do {
        r->y = fw_rom8(f, (uint16_t)(FRM_SNAPSHOT_MAP - 1 + r->x));
        r->a = fw_ind_y(f, ISR_FILL_PTR, r->y);
        fw_wr8(f, (uint16_t)(FRM_SNAP - 1 + r->x), r->a);
        r->x--;
    } while (r->x != 0);
}

void frm_snapshot(fw *f, fw_regs *r)
{
    save_snapshot(f, r);
    take_snapshot(f, r);
}

/* ---- frm_clamp_mark_space $B930 ---- */

/* $B930-$B93A: Y = 10, A = b10; INY; ASL A: FW_TMP_75 = 2c; b11 = @M. */
static void mark_space(fw *f, fw_regs *r)
{
    r->y = FRAME_PITCH;
    r->a = fw_ind_y(f, ISR_FILL_PTR, r->y);
    r->y++;
    r->a = (uint8_t)(r->a << 1);
    fw_wr8(f, FW_TMP_75, r->a);
    r->a = fw_rd8(f, CMD_MARK_SPACE);
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

/* $B93C-$B945: A = 160 - 2c (SEC / SBC, 8 bits); CMP b11 / BCS: written when A < b11 (unsigned). */
static void upper_cap(fw *f, fw_regs *r)
{
    r->a = (uint8_t)(FRM_MARK_SPACE_CAP - fw_rd8(f, FW_TMP_75));
    if (r->a < fw_ind_y(f, ISR_FILL_PTR, r->y))
        fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

/* $B947-$B952: A = 47 - 2c (SEC / SBC, 8 bits); BMI: skipped when bit 7 is set; CMP b11 / BCC: written when
   A >= b11 (unsigned). */
static void lower_bound(fw *f, fw_regs *r)
{
    r->a = (uint8_t)(FRM_MARK_SPACE_BASE - fw_rd8(f, FW_TMP_75));
    if (!(r->a & 0x80) && r->a >= fw_ind_y(f, ISR_FILL_PTR, r->y))
        fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

/* $B954-$B95A: A = 11; CMP b11 / BCC: written when 11 >= b11. */
static void floor_11(fw *f, fw_regs *r)
{
    r->a = FRM_MARK_SPACE_FLOOR;
    if (r->a >= fw_ind_y(f, ISR_FILL_PTR, r->y))
        fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

void frm_clamp_mark_space(fw *f, fw_regs *r)
{
    mark_space(f, r);
    upper_cap(f, r);
    lower_bound(f, r);
    floor_11(f, r);
}

/* ---- frm_raise_formants $B95D ---- */

/* LDY #n; CLC; ADC (fill),Y; JSR par_clamp; STA (fill),Y: frame byte n += A, clamped to 1-63 ($B96B-$B973 for F1,
   $B978-$B980 for F2). */
static void raise(fw *f, fw_regs *r, uint8_t n)
{
    r->y = n;
    r->a = (uint8_t)(r->a + fw_ind_y(f, ISR_FILL_PTR, r->y));
    par_clamp(f, r);
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

void frm_raise_formants(fw *f, fw_regs *r)
{
    r->y = FRAME_PITCH;                        /* $B95D-$B966: SEC; SBC #$20; BPL, else RTS */
    r->a = (uint8_t)(fw_ind_y(f, ISR_FILL_PTR, r->y) - FRM_RAISE_PITCH);
    if (r->a & 0x80)
        return;
    r->a >>= 2;                                /* $B967-$B969: LSR, LSR, STA $75 */
    fw_wr8(f, FW_TMP_75, r->a);
    raise(f, r, FRAME_F1);
    r->a = (uint8_t)(fw_rd8(f, FW_TMP_75) >> 1);   /* $B975-$B977: LDA $75; LSR */
    raise(f, r, FRAME_F2);
}

/* ---- frm_transition_flags $BA3F ---- */

/* CMP #k: the N flag, bit 7 of the 8-bit d - k (BPL / BMI). */
static bool minus(uint8_t d, uint8_t k) { return ((uint8_t)(d - k) & 0x80) != 0; }

/* LDA $16DD,Y; SEC; SBC $16CD,Y: frm_snap_prev[i] - frm_snap[i], 8 bits. */
static uint8_t moved(fw *f, uint8_t i)
{
    return (uint8_t)(fw_rd8(f, (uint16_t)(FRM_SNAP_PREV + i)) - fw_rd8(f, (uint16_t)(FRM_SNAP + i)));
}

/* LDA frm_flags; ORA #bit; STA frm_flags. */
static void set_flag(fw *f, uint8_t bit) { fw_wr8(f, FRM_FLAGS, (uint8_t)(fw_rd8(f, FRM_FLAGS) | bit)); }

/* $BA44-$BA74: the frequencies (Y = 9 down to 6, CMP #$10 / BPL, CMP #$F1 / BMI; DEY / CPY #5), then the
   amplitudes (Y = 5 down to 1, CMP #$0C / BPL, CMP #$F5 / BMI; DEY / BNE); the first hit sets FRM_FLAG_JUMP. */
static void jump_flag(fw *f)
{
    uint8_t i = 9;
    for (; i != 5; i--)
        if (!minus(moved(f, i), 0x10) || minus(moved(f, i), 0xF1)) {
            set_flag(f, FRM_FLAG_JUMP);
            return;
        }
    for (; i != 0; i--)
        if (!minus(moved(f, i), 0x0C) || minus(moved(f, i), 0xF5)) {
            set_flag(f, FRM_FLAG_JUMP);
            return;
        }
}

/* $BA77-$BAA4: for Y = 5 down to 1: CMP #1 / BEQ on frm_snap[i] picks the limit, 19 ($13) at the silent value 1,
   else 28 ($1C); CMP / BPL: the first hit sets FRM_FLAG_FALL. */
static void fall_flag(fw *f)
{
    for (uint8_t i = 5; i != 0; i--) {
        uint8_t k = fw_rd8(f, (uint16_t)(FRM_SNAP + i)) == 1 ? 0x13 : 0x1C;
        if (!minus(moved(f, i), k)) {
            set_flag(f, FRM_FLAG_FALL);
            return;
        }
    }
}

/* $BAA7-$BAC7: LDM #0,$75; for Y = 5 down to 1: frm_snap[i] - frm_snap_prev[i] (SEC; SBC); CMP #$0C / BMI, else
   INC $75. Then LDA $75; CMP #2 / BMI, else FRM_FLAG_RISES. */
static void rise_flag(fw *f)
{
    fw_wr8(f, FW_TMP_75, 0);
    for (uint8_t i = 5; i != 0; i--) {
        uint8_t rise = (uint8_t)(fw_rd8(f, (uint16_t)(FRM_SNAP + i)) - fw_rd8(f, (uint16_t)(FRM_SNAP_PREV + i)));
        if (!minus(rise, 0x0C))
            fw_inc8(f, FW_TMP_75);
    }
    if (!minus(fw_rd8(f, FW_TMP_75), 2))
        set_flag(f, FRM_FLAG_RISES);
}

/* $BACA-$BAE1: the pitch code (frm_snap_prev[0] - frm_snap[0]): CMP #7 / BPL, CMP #$FA / BMI: FRM_FLAG_PITCH. */
static void pitch_flag(fw *f)
{
    if (!minus(moved(f, 0), 0x07) || minus(moved(f, 0), 0xFA))
        set_flag(f, FRM_FLAG_PITCH);
}

void frm_transition_flags(fw *f, fw_regs *r)
{
    r->a = 0;                                  /* $BA3F-$BA41 */
    fw_wr8(f, FRM_FLAGS, r->a);
    jump_flag(f);
    fall_flag(f);
    rise_flag(f);
    pitch_flag(f);
    r->a = fw_rd8(f, FRM_FLAGS);               /* $BAE4-$BAE9: b13 = frm_flags */
    r->y = FRAME_FLAGS;
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

/* ---- frm_finish_frame $B9B5 ---- */

/* $B9C3-$B9D7: Y = 16, X = frm_source_index: b16 = frm_source_codes,X; INY, X = frm_filter_index: b17 =
   frm_filter_codes,X; DEY / DEY: Y = 15. */
static void source_filter(fw *f, fw_regs *r)
{
    r->y = FRAME_SOURCE;
    r->x = fw_rd8(f, FRM_SOURCE_INDEX);
    r->a = fw_rom8(f, (uint16_t)(FRM_SOURCE_CODES + r->x));
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
    r->y++;
    r->x = fw_rd8(f, FRM_FILTER_INDEX);
    r->a = fw_rom8(f, (uint16_t)(FRM_FILTER_CODES + r->x));
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
    r->y -= 2;
}

/* $B9D8-$B9E9: X = 5, LDA frm_snap,X; DEX; SEC; SBC frm_snap,X; SEC; SBC #4: d = ALF - A1 - 4 (8 bits). BEQ / BMI:
   false when d is 0 or has bit 7 set; else frm_level_diff = d. */
static bool level_diff(fw *f, fw_regs *r)
{
    r->x = 5;
    r->a = fw_rd8(f, (uint16_t)(FRM_SNAP + r->x));
    r->x--;
    r->a = (uint8_t)(r->a - fw_rd8(f, (uint16_t)(FRM_SNAP + r->x)));
    r->a = (uint8_t)(r->a - FRM_LEVEL_MARGIN);
    if (r->a == 0 || (r->a & 0x80))
        return false;
    fw_wr8(f, FRM_LEVEL_DIFF, r->a);
    return true;
}

/* $B9EC-$B9FB: X = 7, LDA frm_snap,X; SEC; SBC #$21: e = F1 - 33 (8 bits). BEQ / BMI: false when e is 0 or has
   bit 7 set; else frm_f1_excess = e and X = e (TAX). */
static bool f1_excess(fw *f, fw_regs *r)
{
    r->x = 7;
    r->a = (uint8_t)(fw_rd8(f, (uint16_t)(FRM_SNAP + r->x)) - FRM_F1_BASE);
    if (r->a == 0 || (r->a & 0x80))
        return false;
    fw_wr8(f, FRM_F1_EXCESS, r->a);
    r->x = r->a;
    return true;
}

/* $BA04-$BA17: A = 0; for X = 8 down to 1: LSR frm_f1_excess, BCC, else CLC / ADC frm_level_diff (8 bits, the
   carry out lost); ASL frm_level_diff. A = d * e (8 bits); both bytes end at 0. */
static void multiply(fw *f, fw_regs *r)
{
    r->a = 0;
    for (r->x = 8; r->x != 0; r->x--) {
        uint8_t e = fw_rd8(f, FRM_F1_EXCESS);
        fw_wr8(f, FRM_F1_EXCESS, (uint8_t)(e >> 1));
        if (e & 1)
            r->a = (uint8_t)(r->a + fw_rd8(f, FRM_LEVEL_DIFF));
        fw_wr8(f, FRM_LEVEL_DIFF, (uint8_t)(fw_rd8(f, FRM_LEVEL_DIFF) << 1));
    }
}

/* $B9FC-$BA26: LDA frm_product_limit,X (X = e); CMP frm_level_diff / BMI: the index FRM_PRODUCT_SATURATED when the
   8-bit difference has bit 7 set, else the product. TAX: frm_b14 = frm_b14_15_lo,X; A = frm_b14_15_hi,X. */
static void product_word(fw *f, fw_regs *r)
{
    r->a = fw_rom8(f, (uint16_t)(FRM_PRODUCT_LIMIT + r->x));
    if (minus(r->a, fw_rd8(f, FRM_LEVEL_DIFF)))
        r->a = FRM_PRODUCT_SATURATED;
    else
        multiply(f, r);
    r->x = r->a;
    r->a = fw_rom8(f, (uint16_t)(FRM_B14_15_LO + r->x));
    fw_wr8(f, FRM_B14, r->a);
    r->a = fw_rom8(f, (uint16_t)(FRM_B14_15_HI + r->x));
}

/* $B9D8-$BA34: the b14-b15 word, or 0 ($BA29-$BA2B: LDA #0; STA frm_b14); STA (fill),Y: b15 (Y = 15); DEY;
   LDA frm_b14; STA (fill),Y: b14. */
static void b14_15(fw *f, fw_regs *r)
{
    if (level_diff(f, r) && f1_excess(f, r)) {
        product_word(f, r);
    } else {
        r->a = 0;
        fw_wr8(f, FRM_B14, r->a);
    }
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
    r->y--;
    r->a = fw_rd8(f, FRM_B14);
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

void frm_finish_frame(fw *f, fw_regs *r)
{
    fw_regs s = *r;                            /* $B9B5-$B9B9: PHA / TXA / PHA / TYA / PHA */
    frm_snapshot(f, &s);
    frm_clamp_mark_space(f, &s);
    frm_raise_formants(f, &s);
    source_filter(f, &s);
    b14_15(f, &s);
    frm_transition_flags(f, &s);               /* $BA36, then PLA / TAY / PLA / TAX / PLA: r as it came in */
}
