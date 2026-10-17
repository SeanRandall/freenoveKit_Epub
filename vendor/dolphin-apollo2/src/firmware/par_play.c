#include "par_play.h"
#include "cmd_play_event.h"
#include "frm_build.h"
#include "fw_ops.h"
#include "isr_dsp.h"
#include "isr_serial.h"
#include "par_addr.h"
#include "par_start.h"
#include "par_track.h"
#include "ser.h"

#define UART_TX_READY    0x02   /* $B895 BBC1 UART_STATUS: the transmitter is ready */
#define PAR_WAIT_LAST    0xB8C9   /* $B8C9-$B8CC: the frame-flag wait of the last frames */
#define PAR_WAIT_REST    0xB8E6   /* $B8E6-$B8E9: the frame-flag wait of the rest frames */

/* ---- par_play $B877 ---- */

/* DEC par_frames_left ($B8BB, $B8EF, $B8FB): mod 256, so 0 becomes $FF. Returns the new count (its Z flag). */
static uint8_t count_frame(fw *f)
{
    uint8_t n = (uint8_t)(fw_rd8(f, PAR_FRAMES_LEFT) - 1);
    fw_wr8(f, PAR_FRAMES_LEFT, n);
    return n;
}

/* $B8C9-$B8CC, $B8E6-$B8E9 (at): isr_par_ready, then until the timer interrupt has swapped the buffers and set
   isr_frame_flag bit 0, a wait for the next interrupt (M4.6b spec 2.3) and isr_par_ready again. A failed wait ends
   it: under a thunk the fault has longjmped out already, and without a trap (a unit test) the flag would never
   come. */
static void wait_frame_flag(fw *f, fw_regs r, uint16_t at)
{
    for (;;) {
        fw_par_ready(f, r);
        if ((fw_rd8(f, ISR_FRAME_FLAG) & 1) || f->fault)       /* BBC0 isr_frame_flag */
            return;
        fw_wait(f, FW_WAIT_INTERRUPT, at);
        if (f->fault)
            return;
    }
}

/* $B877-$B892: an event is due when cmd_ev_next < cmd_ev_count (CPX / BCS, unsigned) and par_seg_index =
   cmd_ev_pos, low byte, then high byte (CMP / BNE). cmd_run_event runs it with X = cmd_ev_next and A = the
   high byte of par_seg_index, and the JMP par_play ($B892) tests again. False when none is due: X = cmd_ev_next,
   and A = the last byte loaded, or as before when the BCS left first. */
static bool run_due_event(fw *f, fw_regs *r)
{
    r->x = fw_rd8(f, CMD_EV_NEXT);
    if (r->x >= fw_rd8(f, CMD_EV_COUNT))
        return false;
    r->a = fw_rd8(f, PAR_SEG_INDEX);
    if (r->a != fw_rd8(f, CMD_EV_POS))
        return false;
    r->a = fw_rd8(f, PAR_SEG_INDEX + 1);
    if (r->a != fw_rd8(f, CMD_EV_POS + 1))
        return false;
    cmd_run_event(f, r);
    return true;
}

/* $B895-$B89C: the reply output, when the transmitter is ready (UART_STATUS bit 1, BBC1) and a status byte is due
   (TST ser_status_due, the whole byte). */
static void serve_reply(fw *f, fw_regs *r)
{
    if ((fw_sfr8(f, SFR_UART_STATUS) & UART_TX_READY) && fw_rd8(f, SER_STATUS_DUE) != 0)
        ser_tx_service(f, r);
}

/* $B8B1-$B907, with isr_frame_flag set: Y = pros_seg_count's low byte, or its high byte when that is 0 (LDY / BNE,
   twice). With no segment left, false ($B8BB). Otherwise DEC par_frames_left; when it reaches 0, par_next_segment
   and A = the new par_frames_left, and when that is 0 too, the test again from $B8B1 (BEQ $B906). True when a frame
   of the segment follows ($B908). */
static bool segment_frame(fw *f, fw_regs *r)
{
    for (;;) {
        r->y = fw_rd8(f, PROS_SEG_COUNT);
        if (r->y == 0) {
            r->y = fw_rd8(f, PROS_SEG_COUNT + 1);
            if (r->y == 0)
                return false;
        }
        if (count_frame(f) != 0)
            return true;
        par_next_segment(f, r);
        r->a = fw_rd8(f, PAR_FRAMES_LEFT);
        if (r->a != 0)
            return true;
    }
}

/* $B908-$B911: isr_par_ready, par_step_tracks, frm_finish_frame (C; it keeps A, X and Y), then LDM #0: the whole of
   isr_frame_flag cleared, the frame is ready for the next swap. */
static void segment_step(fw *f, fw_regs *r)
{
    fw_par_ready(f, *r);
    par_step_tracks(f, r);
    frm_finish_frame(f, r);
    fw_wr8(f, ISR_FRAME_FLAG, 0);
}

/* $B8BB-$B8CF, once no segment is left: DEC par_frames_left; until it reaches 0, par_step_tracks, frm_finish_frame,
   the flag cleared, and a wait for the next swap. No events, no reply service, no abort test. */
static void last_frames(fw *f, fw_regs *r)
{
    while (count_frame(f) != 0) {
        par_step_tracks(f, r);
        frm_finish_frame(f, r);
        fw_wr8(f, ISR_FRAME_FLAG, 0);
        wait_frame_flag(f, *r, PAR_WAIT_LAST);
    }
}

/* $B8D2-$B8FA, the end: isr_wait_frame (C); par_frames_left = 3; per rest frame frm_copy_rest_frame, frm_snapshot,
   frm_clamp_mark_space and frm_transition_flags (C, the registers flowing from one to the next), a wait for the
   swap, the flag cleared (LDM #0) and DEC par_frames_left (BNE $B8DA). Then cmd_ev_sound_end (a @T effect cut short
   is undone) and isr_par_ready. */
static void rest_frames(fw *f, fw_regs *r)
{
    isr_wait_frame(f, r);
    r->a = PAR_REST_FRAMES;
    fw_wr8(f, PAR_FRAMES_LEFT, r->a);
    do {
        frm_copy_rest_frame(f, r);
        frm_snapshot(f, r);
        frm_clamp_mark_space(f, r);
        frm_transition_flags(f, r);
        wait_frame_flag(f, *r, PAR_WAIT_REST);
        fw_wr8(f, ISR_FRAME_FLAG, 0);
    } while (count_frame(f) != 0);
    cmd_ev_sound_end(f, r);
    fw_par_ready(f, *r);
}

void par_play(fw *f, fw_regs *r)
{
    for (;;) {                                 /* par_play $B877: one pass */
        if (f->fault)
            return;                            /* a failed call or wait without a trap: the ROM would not get past it */
        if (run_due_event(f, r))
            continue;                          /* $B892 JMP par_play */
        serve_reply(f, r);
        fw_par_ready(f, *r);                   /* $B89F */
        if (fw_rd8(f, SER_ABORT) & 1)          /* $B8A2 BBC0 ser_abort */
            break;                             /* $B8A5 JMP $B8D2: a mute */
        if (!(fw_rd8(f, ISR_FRAME_FLAG) & 1)) {   /* $B8A8 BBS0 isr_frame_flag */
            fw_par_ready(f, *r);               /* $B8AB, then $B8AE JMP par_play */
            fw_wait(f, FW_WAIT_INTERRUPT, ROM_PAR_PLAY);   /* the next pass after an interrupt (M4.6b spec 2.3) */
            continue;
        }
        if (!segment_frame(f, r)) {
            last_frames(f, r);
            break;
        }
        segment_step(f, r);                    /* then $B914 JMP par_play */
    }
    rest_frames(f, r);
}

/* ---- boot_loop_play $A7D4 ---- */

/* $B2B3-$B2D8, boot_play_body (JMP from $A7DB): hold the host, set up playback, start the DSP with a frame's wait,
   ramp the volume in (half, a frame, then cmd_volume), play, ramp it out (half, a frame, then 0) and stop the DSP.
   The registers flow from call to call as the ROM's do; A is loaded only for the two isr_set_volume calls. */
static void play_body(fw *f, fw_regs *r)
{
    isr_par_hold(f, r);
    par_play_start(f, r);
    isr_speech_start(f, r);
    isr_volume_half(f, r);
    isr_wait_frame(f, r);
    r->a = fw_rd8(f, CMD_VOLUME);
    isr_set_volume(f, r);
    par_play(f, r);
    isr_volume_half(f, r);
    isr_wait_frame(f, r);
    r->a = 0;
    isr_set_volume(f, r);
    isr_speech_stop(f, r);
}

void boot_loop_play(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, FW_UTT_CONTENT);          /* $A7D4 LDA $6B / BEQ $A7DE */
    if (r->a == 0 || (fw_rd8(f, SER_ABORT) & 1))   /* $A7D8 BBS0 ser_abort,$A7DE */
        return;                                /* $A7DE RTS */
    play_body(f, r);
}
