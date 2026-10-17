#ifndef FW_ISR_DSP_H
#define FW_ISR_DSP_H
/* Speech timing and the DSP (ROM $ACE8-$ACF9, $B2D9-$B397, $B465-$B472), decompiled (M4.6b spec 2.1, 2.6). Each
   routine takes the registers at entry in r and leaves in r the A, X and Y the ROM leaves at its RTS (or RTI); r->p
   is not used: no caller reads a flag, and the PHP/SEI ... PLP pairs only hold interrupts off, which the C, taking
   no emulated time, needs no more (M4.6b spec 3.5). SFRs are read and written through the host in the ROM's order
   (fw_sfr*), each SEB/CLB as a read then a write. */
#include "fw.h"

/* The SFRs of speech timing (M4.6b spec 2.5; ghidra740/annotations/sfr.txt); P2 (SFR_P2) is in fw.h */
#define SFR_T2_PRE      0xDE
#define SFR_T2_RELOAD   0xDF
#define SFR_T3_PRE      0xE0
#define SFR_T3_RELOAD   0xE1
#define SFR_P23_FUNC    0xED
#define SFR_P3          0xEE   /* bits 3-6: the inverted volume level; bit 0 set by isr_timer_start */
#define SFR_ICR2        0xFE   /* timers 1-3: enable bits 0, 2, 4, request bits 1, 3, 5 */

/* $B2D9, an interrupt handler, entered through vec_int2_timers $FFF8 (timer 3, the 10 kHz DSP tick): CLB5 ICR2;
   isr_tick_div counted down; at 0 the word at (isr_play_ptr),Y with Y = isr_word_ofs goes to the DSP port, its even
   byte first (fw_dsp_write twice), isr_word_ofs + 2 (8 bits), isr_tick_div = 10 and isr_words_left counted down; at
   0 isr_words_left = 10 and isr_word_ofs = 0, and with isr_frame_flag bit 0 clear the frame event (fw_frame_done,
   where the ROM reaches $B306), isr_frame_flag = 1 (LDM) and the buffers swapped by isr_swap_phase counted down (to
   1: isr_play_ptr's low byte $B9, isr_fill_ptr's $A5; to 0: $A5, $B9 and the phase 2 again); with the flag still set
   the frame repeats. A, X and Y as they came (PHA / TYA / PHA ... PLA / TAY / PLA; RTI). */
void isr_timer_dsp(fw *f, fw_regs *r);

/* $ACE8: A = the volume 0-15 (0 silent): P3 bits 3-6 = its inverted level, (A ^ $F) & $F shifted left 3
   (util_shl3), kept in the scratch byte $75; the other P3 bits kept (AND #$87). A = the value written to P3; X and Y
   untouched. */
void isr_set_volume(fw *f, fw_regs *r);
/* $B46C: isr_set_volume with A = cmd_volume >> 1. */
void isr_volume_half(fw *f, fw_regs *r);

/* $B38B: the DSP reset line P2.0 low, high, low (CLB0, SEB0, CLB0 P2), inside PHP/SEI ... PLP. A, X and Y
   untouched. */
void isr_dsp_reset(fw *f, fw_regs *r);
/* $B35D: starts the 10 kHz DSP tick, inside PHP/SEI ... PLP: T2_PRE = T3_PRE = 0; isr_tick_div decremented twice;
   P3.0 set; T2_RELOAD = 4; ICR2 bits 3 and 2 cleared; a wait for timer 2's request bit (FW_WAIT_TIMER2, $B372);
   T3_RELOAD = $28; ICR2 bits 5 and 4 cleared; P23_FUNC = 1; a wait for timer 3's request bit (FW_WAIT_TIMER3,
   $B37F); T3_RELOAD = $31; ICR2 = $10 (timer 3's interrupt alone). A, X and Y untouched. A failed wait ends it (a
   fault). */
void isr_timer_start(fw *f, fw_regs *r);
/* $B398: P23_FUNC = 0, ICR2 = 0, isr_dsp_reset, boot_speaking bit 0 cleared. A, X and Y untouched. */
void isr_speech_stop(fw *f, fw_regs *r);
/* $B323: frm_source_index = cmd_source_prosody's bits 7-5 and frm_filter_index = its bit 4 (both 0 first, then a ROL
   per bit); isr_tick_div = isr_words_left = 10, isr_word_ofs = isr_frame_flag = 0; isr_dsp_reset; isr_timer_start;
   isr_wait_frames with X = 1. A = 0 and X = 0 out, Y untouched. */
void isr_speech_start(fw *f, fw_regs *r);
/* $B356: X frame boundaries: isr_wait_frame, then DEX until X is 0 (X = 0 waits 256). X = 0 out, A and Y untouched. */
void isr_wait_frames(fw *f, fw_regs *r);
/* $B465: until the timer interrupt has set isr_frame_flag bit 0 (a frame boundary), a wait for the next interrupt
   (FW_WAIT_INTERRUPT at $B465, M4.6b spec 2.3); then the whole byte = 0 (LDM). A, X and Y untouched. A failed wait
   ends it (a fault). */
void isr_wait_frame(fw *f, fw_regs *r);

#endif
