#ifndef FW_ISR_ADDR_H
#define FW_ISR_ADDR_H
/* ROM code addresses of speech timing, the DSP, the volume and the parallel handshake (M4.6b spec 2.1), checked
   against build/ghidra740/dump and the JSR/JMP operands and vectors in the ROM image. A caller in parentheses was ROM
   until M4.6c2: it is C now and calls the C. */
#define ROM_ISR_SET_VOLUME     0xACE8   /* called by boot_loop_play $B2C4, $B2D2, isr_volume_half $B46F,
                                           cmd_ev_set_volume $BCEF (and boot_reset $A642) */
#define ROM_ISR_TIMER_DSP      0xB2D9   /* INT2: entered through vec_int2_timers $FFF8 (timer 3, the DSP tick) */
#define ROM_ISR_SWAP_BUFFERS   0xB306   /* isr_timer_dsp's frame boundary: the frame event (FW_FRAME_EVENT) */
#define ROM_ISR_SPEECH_START   0xB323   /* called by boot_loop_play $B2B9 */
#define ROM_ISR_WAIT_FRAMES    0xB356   /* called by isr_speech_start $B352 */
#define ROM_ISR_TIMER_START    0xB35D   /* called by isr_speech_start $B34D */
#define ROM_ISR_DSP_RESET      0xB38B   /* called by isr_speech_start $B34A, isr_speech_stop $B39E */
#define ROM_ISR_SPEECH_STOP    0xB398   /* called by boot_loop_play $B2D5 */
#define ROM_ISR_WAIT_FRAME     0xB465   /* called by boot_loop_play $B2BF, $B2CD, isr_wait_frames $B356, par_play
                                           $B8D2 */
#define ROM_ISR_VOLUME_HALF    0xB46C   /* called by boot_loop_play $B2BC, $B2CA */
#define ROM_ISR_PAR_READY      0xC5D6   /* parallel mode: ready for the next host byte; A, X, Y preserved. Called by
                                           tn_convert_clause $B1F6, cmd_scan_clause $AD09, par_play $B89F, $B8AB,
                                           $B8C9, $B8E6, $B8F7, $B908, pros_append_segment $CA3B, pros_mark_stress
                                           $CB1F, pros_mark_phrases $CF28, pros_phrase_gen $D155 (and boot_reentry_warm
                                           $A672, boot_get_clause $A6D0, $A6DC, $A6FC, boot_queue_string $C62B) */
#define ROM_ISR_PAR_HOLD       0xC5DE   /* called by boot_loop_play $B2B3 (and boot_reentry_warm $A66F, boot_get_clause
                                           $A6E4, boot_queue_string $C617) */

#endif
