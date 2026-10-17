#include "par_track.h"
#include "par_addr.h"
#include "data/fw_data.h"

/* ---- util_interp_step $C1A1 ---- */

/* $C1A8-$C1AD, Y = 2: CLC; ADC util_interp_cur; ROR A: the 9-bit sum halved, its carry into bit 7. */
static void average(fw *f, fw_regs *r)
{
    r->a = (uint8_t)(((unsigned)r->a + fw_rd8(f, UTIL_INTERP_CUR)) >> 1);
}

/* $C1BA-$C1C1, d without a borrow (A = d): LDY #0, then INY; SEC; SBC util_interp_steps; BCS: Y = the number of
   subtractions up to and including the one that borrows (8 bits), A = what that one leaves. With util_interp_steps =
   0 nothing ever borrows and the ROM loops forever: the C faults and returns false. */
static bool quotient_up(fw *f, fw_regs *r)
{
    uint8_t steps = fw_rd8(f, UTIL_INTERP_STEPS);
    if (steps == 0) {
        fw_fault(f, ROM_UTIL_INTERP_STEP);
        return false;
    }
    bool no_borrow;
    r->y = 0;
    do {
        r->y++;
        no_borrow = r->a >= steps;
        r->a = (uint8_t)(r->a - steps);
    } while (no_borrow);
    return true;
}

/* $C1C3-$C1CC: LSR util_interp_steps (the halved count stays in RAM); CLC; ADC util_interp_steps; BPL: bit 7 of the
   8-bit sum set takes one off Y. */
static void round_up(fw *f, fw_regs *r)
{
    uint8_t half = fw_rd8(f, UTIL_INTERP_STEPS) >> 1;
    fw_wr8(f, UTIL_INTERP_STEPS, half);
    r->a = (uint8_t)(r->a + half);
    if (r->a & 0x80)
        r->y--;
}

/* $C1D8-$C1DF, d with a borrow (A = d): LDY #0, then DEY; CLC; ADC util_interp_steps; BCC: Y = minus the number of
   additions up to and including the one that carries (8 bits), A = the 8-bit sum it leaves. With
   util_interp_steps = 0 nothing ever carries and the ROM loops forever: the C faults and returns false. */
static bool quotient_down(fw *f, fw_regs *r)
{
    uint8_t steps = fw_rd8(f, UTIL_INTERP_STEPS);
    if (steps == 0) {
        fw_fault(f, ROM_UTIL_INTERP_STEP);
        return false;
    }
    unsigned sum;
    r->y = 0;
    do {
        r->y--;
        sum = (unsigned)r->a + steps;
        r->a = (uint8_t)sum;
    } while (sum < 0x100);
    return true;
}

/* $C1E3-$C1EF, after the BEQ at $C1E1 (a sum of 0 skips this): LSR util_interp_steps (kept in RAM); SEC; SBC
   util_interp_steps; BMI, BEQ: Y + 1 (INY) when the 8-bit difference is above 0 with bit 7 clear. */
static void round_down(fw *f, fw_regs *r)
{
    uint8_t half = fw_rd8(f, UTIL_INTERP_STEPS) >> 1;
    fw_wr8(f, UTIL_INTERP_STEPS, half);
    r->a = (uint8_t)(r->a - half);
    if (!(r->a & 0x80) && r->a != 0)
        r->y++;
}

/* $C1CD-$C1D7: util_interp_quot = Y; LDA util_interp_cur; CLC; ADC util_interp_quot. */
static void add_quotient(fw *f, fw_regs *r)
{
    fw_wr8(f, UTIL_INTERP_QUOT, r->y);
    r->a = (uint8_t)(fw_rd8(f, UTIL_INTERP_CUR) + fw_rd8(f, UTIL_INTERP_QUOT));
}

void util_interp_step(fw *f, fw_regs *r)
{
    r->y--;                                        /* $C1A1 DEY; BNE: Y = 1 returns the target */
    if (r->y == 0)
        return;
    r->y--;                                        /* $C1A5 DEY; BNE */
    if (r->y == 0) {
        average(f, r);
        return;
    }
    uint8_t cur = fw_rd8(f, UTIL_INTERP_CUR);      /* $C1AE SEC; SBC util_interp_cur */
    bool up = r->a >= cur;
    r->a = (uint8_t)(r->a - cur);
    if (r->a == 0) {                               /* $C1B2 BNE; LDA util_interp_target */
        r->a = fw_rd8(f, UTIL_INTERP_TARGET);
        return;
    }
    if (up) {                                      /* $C1B8 BCC $C1D8 */
        if (!quotient_up(f, r))
            return;
        round_up(f, r);
    } else {
        if (!quotient_down(f, r))
            return;
        if (r->a != 0)                             /* $C1E1 BEQ $C1CD */
            round_down(f, r);
    }
    add_quotient(f, r);
}

/* ---- the steps of every track ---- */

/* One step of a track value at val towards the value at target, with Y = the steps (loaded by the caller):
   STY util_interp_steps; DEC dur (when dur is not 0: the overlap line and the pitch have no count); LDA val; STA
   util_interp_cur; LDA target; STA util_interp_target; JSR util_interp_step; STA val. The six copies are $C151-$C166,
   $C183-$C198, $C201-$C213, $C253-$C268, $C27C-$C291 and $C115-$C127. */
static void track_step(fw *f, fw_regs *r, uint16_t dur, uint16_t val, uint16_t target)
{
    fw_wr8(f, UTIL_INTERP_STEPS, r->y);
    if (dur != 0)
        fw_wr8(f, dur, (uint8_t)(fw_rd8(f, dur) - 1));
    r->a = fw_rd8(f, val);
    fw_wr8(f, UTIL_INTERP_CUR, r->a);
    r->a = fw_rd8(f, target);
    fw_wr8(f, UTIL_INTERP_TARGET, r->a);
    util_interp_step(f, r);
    fw_wr8(f, val, r->a);
}

/* LDY $B796,X; STA ($2C),Y: A into the fill frame's byte par_slot_map[X - 1] ($C169-$C16C, $C177 with $C17D,
   $C19B-$C19E; par_track_overlap loads Y at $C216 and stores on each of its paths). */
static void slot_store(fw *f, fw_regs *r)
{
    r->y = fw_rom8(f, (uint16_t)(PAR_SLOT_MAP - 1 + r->x));
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

/* ---- par_track_start $C24A, par_track_end $C26C ---- */

void par_track_start(fw *f, fw_regs *r)
{
    uint8_t x = r->x;
    r->y = fw_rd8(f, (uint16_t)(PAR_START_DUR + x));   /* $C24A LDY; BNE */
    if (r->y == 0) {
        r->a = fw_rd8(f, (uint16_t)(PAR_SEG_TARGET + x));   /* $C24F-$C252 */
        return;
    }
    track_step(f, r, (uint16_t)(PAR_START_DUR + x), (uint16_t)(PAR_START_VAL + x), (uint16_t)(PAR_SEG_TARGET + x));
}

/* $C271-$C276: LDY #0; TYA; STA par_end_wait,X: a negative count is zeroed, and the Z of the TYA takes the step
   path at $C277. */
static void zero_wait(fw *f, fw_regs *r)
{
    r->y = 0;
    r->a = r->y;
    fw_wr8(f, (uint16_t)(PAR_END_WAIT + r->x), r->a);
}

/* $C295-$C29B: DEC par_end_wait,X; LDA par_seg_target,X. */
static void end_wait(fw *f, fw_regs *r)
{
    uint16_t wait = (uint16_t)(PAR_END_WAIT + r->x);
    fw_wr8(f, wait, (uint8_t)(fw_rd8(f, wait) - 1));
    r->a = fw_rd8(f, (uint16_t)(PAR_SEG_TARGET + r->x));
}

void par_track_end(fw *f, fw_regs *r)
{
    uint8_t x = r->x;
    r->y = fw_rd8(f, (uint16_t)(PAR_END_WAIT + x));    /* $C26C LDY; BPL */
    if (r->y & 0x80)
        zero_wait(f, r);
    if (r->y != 0) {                                    /* $C277 BNE: the Z of the LDY, or of the TYA */
        end_wait(f, r);
        return;
    }
    r->y = fw_rd8(f, (uint16_t)(PAR_END_DUR + x));     /* $C279 */
    track_step(f, r, (uint16_t)(PAR_END_DUR + x), (uint16_t)(PAR_END_VAL + x), (uint16_t)(PAR_BOUNDARY + x));
}

/* ---- par_track_sequential $C14C ---- */

/* $C174-$C17F: DEC par_steady,X; then LDY $B796,X; LDA par_seg_target,X; STA ($2C),Y (slot_store; the LDY comes
   before the LDA in the ROM, and neither has a side effect). */
static void hold(fw *f, fw_regs *r)
{
    uint16_t steady = (uint16_t)(PAR_STEADY + r->x);
    fw_wr8(f, steady, (uint8_t)(fw_rd8(f, steady) - 1));
    r->a = fw_rd8(f, (uint16_t)(PAR_SEG_TARGET + r->x));
}

void par_track_sequential(fw *f, fw_regs *r)
{
    uint8_t x = r->x;
    r->y = fw_rd8(f, (uint16_t)(PAR_START_DUR + x));   /* $C14C LDY; BEQ $C16F */
    if (r->y != 0) {
        track_step(f, r, (uint16_t)(PAR_START_DUR + x), (uint16_t)(PAR_START_VAL + x),
                   (uint16_t)(PAR_SEG_TARGET + x));     /* $C151-$C166: the start transition */
    } else {
        r->a = fw_rd8(f, (uint16_t)(PAR_STEADY + x));   /* $C16F LDA; BEQ $C180 */
        if (r->a != 0) {
            hold(f, r);
        } else {
            r->y = fw_rd8(f, (uint16_t)(PAR_END_DUR + x));   /* $C180-$C198: the end transition, from the target */
            track_step(f, r, (uint16_t)(PAR_END_DUR + x), (uint16_t)(PAR_SEG_TARGET + x),
                       (uint16_t)(PAR_BOUNDARY + x));
        }
    }
    slot_store(f, r);
}

/* ---- par_track_overlap $C1F2 ---- */

/* CMP m, then BMI / BPL: bit 7 of the 8-bit difference a - m (the sign, overflow ignored). */
static bool minus(uint8_t a, uint8_t m) { return ((uint8_t)(a - m) & 0x80) != 0; }

/* $C219-$C249: A = the line value; the median of it, par_start_track and par_end_track by minus(). */
static void median(fw *f, fw_regs *r)
{
    uint8_t start = fw_rd8(f, PAR_START_TRACK), end = fw_rd8(f, PAR_END_TRACK);
    if (!minus(r->a, start)) {                     /* $C21C BPL $C226 */
        if (minus(r->a, end))                      /* $C226 CMP; BPL $C23F: else the line ($C22B) */
            return;
        r->a = start;                              /* $C23F LDA; CMP; BMI $C239 */
        if (minus(r->a, end))
            r->a = end;
    } else {
        if (!minus(r->a, end))                     /* $C21E CMP; BMI $C22E: else the line ($C223) */
            return;
        r->a = start;                              /* $C22E LDA; CMP; BPL $C239 */
        if (!minus(r->a, end))
            r->a = end;
    }
}

void par_track_overlap(fw *f, fw_regs *r)
{
    par_track_start(f, r);                          /* $C1F2 */
    fw_wr8(f, PAR_START_TRACK, r->a);
    par_track_end(f, r);                            /* $C1F8 */
    fw_wr8(f, PAR_END_TRACK, r->a);
    r->y = fw_rd8(f, PAR_FRAMES_LEFT);              /* $C1FE-$C213: the line, over the frames left */
    track_step(f, r, 0, (uint16_t)(PAR_LINE_VAL + r->x), (uint16_t)(PAR_BOUNDARY + r->x));
    r->y = fw_rom8(f, (uint16_t)(PAR_SLOT_MAP - 1 + r->x));   /* $C216 LDY $B796,X */
    median(f, r);
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);       /* STA ($2C),Y on each path */
}

/* ---- par_step_tracks $C0FF ---- */

/* $C112-$C130: the pitch track (no count): pros_play_pitch steps towards pros_play_target over par_frames_left; LSR
   A twice; LDY #$0A; STA ($2C),Y. */
static void pitch_track(fw *f, fw_regs *r)
{
    r->y = fw_rd8(f, PAR_FRAMES_LEFT);
    track_step(f, r, 0, PROS_PLAY_PITCH, PROS_PLAY_TARGET);
    r->a >>= 2;
    r->y = FRAME_PITCH;
    fw_wr_ind_y(f, ISR_FILL_PTR, r->y, r->a);
}

void par_step_tracks(fw *f, fw_regs *r)
{
    r->x = 10;                                      /* $C0FF LDX #$0A ... DEX; BNE $C101 */
    do {
        r->a = fw_rd8(f, (uint16_t)(PAR_SEQUENTIAL + r->x));
        if (r->a != 0)
            par_track_sequential(f, r);             /* $C10C */
        else
            par_track_overlap(f, r);                /* $C106 */
        r->x--;
    } while (r->x != 0);
    pitch_track(f, r);
}
