#ifndef FW_PAR_PLAY_H
#define FW_PAR_PLAY_H
/* The frame loop of one utterance (ROM $B877-$B916) and the player around it ($A7D4-$A7DE and its body $B2B3-$B2D8),
   decompiled (M4.4b spec 2.1, 2.3, 2.6). Both wait on the timer interrupt: each polling loop tests isr_frame_flag in
   RAM (and par_play's main loop ser_abort first) and, while it is not yet set, waits for the next interrupt through
   the host (fw_wait FW_WAIT_INTERRUPT, M4.6b spec 2.3) before testing again; the C itself takes no emulated time, so
   the interrupt is serviced inside the wait. isr_par_ready is called on every pass where the ROM calls it. Each
   routine takes the registers at entry in r and leaves in r the A, X and Y that the ROM leaves at its RTS. r->p is
   not used: no caller reads a flag. */
#include "fw.h"

#define PAR_REST_FRAMES  3   /* par_play $B8D5 LDA #3: the rest frames sent at the end */

/* $B877: the frame loop. While segments remain (pros_seg_count != 0), each pass runs every command event due at
   par_seg_index (cmd_run_event, one per pass of the test), serves the reply output (ser_tx_service, when UART_STATUS
   bit 1 is set and ser_status_due != 0), calls isr_par_ready, ends at a mute (ser_abort bit 0), and once per frame
   (isr_frame_flag bit 0) counts par_frames_left down, starts the next segment at 0 (par_next_segment), steps the
   tracks (par_step_tracks) and has the frame finished (frm_finish_frame), then clears the flag. When no segment is
   left: the remaining frames with no events, no reply service and no abort test. At the end, or at a mute: three
   rest frames (frm_copy_rest_frame, frm_snapshot, frm_clamp_mark_space, frm_transition_flags), then
   cmd_ev_sound_end and a last isr_par_ready. r in: A, X and Y flow into the first calls. r out: as cmd_ev_sound_end
   leaves them (isr_par_ready preserves them). */
void par_play(fw *f, fw_regs *r);

/* $A7D4: when the utterance has content (FW_UTT_CONTENT, $6B) and no mute is pending (ser_abort bit 0), play it:
   isr_par_hold, par_play_start, isr_speech_start, isr_volume_half, isr_wait_frame, isr_set_volume(cmd_volume),
   par_play, isr_volume_half, isr_wait_frame, isr_set_volume(0), isr_speech_stop (all C since M4.6b). r in: A, X and
   Y flow into the first calls. r out: A = FW_UTT_CONTENT and X, Y untouched when it does not play; else the
   registers isr_speech_stop leaves. */
void boot_loop_play(fw *f, fw_regs *r);

#endif
