#ifndef FW_BOOT_H
#define FW_BOOT_H
/* Boot (ROM $A61C-$A68C, $A742-$A78B, $A79F-$A7BD, $A7F0-$A7FD, $A876-$A87F, $ABF0-$ACC6, $ADE0-$ADFD, $C3DF,
   $C605-$C611), decompiled (M4.6c2 spec 2.1). Each routine takes the registers at entry in r and leaves in r the A, X
   and Y the ROM leaves at its RTS; r->p is not used: no caller reads a flag. SFRs are read and written through the host
   in the ROM's order (fw_sfr*), each SEB/CLB as a read then a write. */
#include "fw.h"

/* The SFRs of boot (ghidra740/annotations/sfr.txt) that ser.h and isr_dsp.h do not name */
#define SFR_AD_CTRL      0xE9
#define SFR_P4           0xEB   /* bits 0-3: the DIP switches */
#define SFR_P3_DIR       0xEF
#define SFR_P2_DIR       0xF1
#define SFR_P0_FUNC      0xF5
#define SFR_P0_DIR       0xF7
#define SFR_UART_BAUD    0xFB

#define DIP_PARALLEL     0x01   /* boot_dip bit 0: the parallel interface */
#define DIP_SELFTEST     0x02   /* boot_dip bit 1: the self-test reads P2.1 (and the alternative bank table) */
#define P2_SELFTEST_PIN  1      /* P2.1: the self-test input */

/* $ADE0: AD_CTRL = 0, then boot_dip = A = P4 & $0F. X and Y untouched. */
void boot_read_dip(fw *f, fw_regs *r);
/* $AC22: the ports by boot_dip bit 0. Serial: P0_FUNC $48, P0_DIR $78, P0 $20, P1_LATCH 0, P1_DIR 0; parallel:
   P0_FUNC 0, P0_DIR 8, P0 0, P1 1, P1_DIR $FF. Then in both P2 $70 (the main bank), P2_DIR $FD, P3_DIR $F9,
   P23_FUNC 0, P3 $78. A = 0; X and Y untouched. */
void boot_init_ports(fw *f, fw_regs *r);
/* $ADEB: boot_clear_input_state, boot_set_defaults, then bank_ver_major and bank_ver_minor = the bank header's
   $FFE0 and $FFE1. A = the $FFE1 byte, X untouched, Y = $1A. */
void boot_init_settings(fw *f, fw_regs *r);
/* $ABF0: ser_abort, boot_speaking, ser_rx_idle, ser_rx_idle_saved, cmd_index_count, ser_tx_pending, $07,
   ser_tx_count, isr_par_state and ser_cmd_mode = 0; cmd_index_reset = 1; the output ring pointers ser_tx_read and
   ser_tx_write = $3900, the input ring's ser_rx_read and ser_rx_write = $2100. A = 0; X and Y untouched. */
void boot_clear_input_state(fw *f, fw_regs *r);
/* $A742: the power-up @-settings; cmd_char = ser_cmd_char = '@'; $6B = 4; the flags and counts cmd_source ...
   cmd_speaker and $1827 = 0; cmd_clear_formant; boot_load_prosody_defaults. A, Y as the last leaves them; X
   untouched. */
void boot_set_defaults(fw *f, fw_regs *r);
/* $A7F0: boot_prosody_defaults (26 bytes) copied to pros_params. A = the last byte, Y = 26; X untouched. */
void boot_load_prosody_defaults(fw *f, fw_regs *r);
/* $AC7E: tn_phon_len = 0; UART_BAUD by the P3.1 and P3.2 jumpers (one read of P3 per test): $02 (both low), $BF
   (P3.1 high), $2F (P3.2 high), $05 (both). With X = 1 the rate is also spoken: tn_rule_ptr = its phonetic text
   (boot_baud_text_19200, _300 or _1200; none for $05), cmd_say_phonetic. A, X and Y untouched, or as
   cmd_say_phonetic leaves them. */
void boot_init_baud(fw *f, fw_regs *r);
/* $AC68: UART_CTRL = $D1; A = $4C to P0_FUNC; the host line P0.2 high: CLB5 P0 and isr_host_line_high; low: SEB5
   P0 and isr_host_line_low. A = $4C; X and Y untouched. */
void boot_init_host_lines(fw *f, fw_regs *r);
/* $A876: cmd_put_phonetic (the length-prefixed text at tn_rule_ptr to tn_phonetic), pros_make_segments,
   boot_loop_play (C calls C); it waits as boot_loop_play does. A, X and Y as boot_loop_play leaves them. */
void cmd_say_phonetic(fw *f, fw_regs *r);
/* $C605: boot_selftest_fail = 1 when boot_dip bit 1 is set and the input P2.1 reads high (one read of P2), else 0.
   A, X and Y untouched. */
void boot_selftest(fw *f, fw_regs *r);
/* $A79F: cmd_text_ptr = boot_selftest_ok_cmds, boot_selftest, boot_selftest_fail_cmds when it failed;
   boot_queue_string; then the main loop's steps once: boot_loop_text, pros_make_segments, boot_loop_play (C calls
   C). A, X and Y as boot_loop_play leaves them. */
void boot_selftest_sound(fw *f, fw_regs *r);

/* The entries (M4.6c2 spec 2.1, 3.4). None returns: the main loop runs for ever, and only a stopped wait, a bank entry
   (fw_enter) or a fault leaves it, each a longjmp to f->trap; without a trap (a unit test) the routine returns then. */
/* $A61C, the reset: P3_DIR $F9, P23_FUNC 0, P3 $78; the CPU set-up (fw_reset_cpu: SEI, SP $FF, CLT, CLD, CLC, CLV);
   A = X = Y = 0; ICR1, ICR2 and ICR3 = 0; $C9 down to $01 cleared; cmd_volume = 0 and isr_set_volume; boot_read_dip;
   boot_init_ports; ICR1 = 1; boot_init_settings; cmd_apply_speaker; boot_selftest_sound; in serial mode (boot_dip
   bit 0 clear) boot_init_baud with X = 0 and boot_init_host_lines; then boot_reentry_warm. */
void boot_reset(fw *f);
/* $A665: boot_read_dip, boot_init_settings, then boot_reentry_warm (the ROM falls into it). */
void boot_reentry_cold(fw *f, fw_regs *r);
/* $A66B: ser_abort = 0; CLI (fw_set_interrupts); isr_par_hold; isr_par_ready; cmd_apply_speaker; bank_load_ramcode,
   bank_scan, bank_find_current; then the main loop $A681 for ever: boot_loop_text, pros_make_segments,
   boot_loop_play (C calls C). */
void boot_reentry_warm(fw *f, fw_regs *r);
/* $FFD0, the main bank's cold entry: SEI (the signature byte $78 bank_probe reads), then boot_reentry_cold. */
void bank_entry_cold(fw *f, fw_regs *r);
/* $FFD4, its warm entry: boot_reentry_warm. */
void bank_entry_warm(fw *f, fw_regs *r);
/* $C3DF, the handler of vec_int3_step $FFF6 and vec_int0 $FFFC: RTI, nothing else. The board never raises those
   lines (M4.6c2 spec 2.1). */
void isr_unused(fw *f, fw_regs *r);

#endif
