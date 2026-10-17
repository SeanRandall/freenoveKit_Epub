#ifndef FW_BOOT_INPUT_H
#define FW_BOOT_INPUT_H
/* The main loop's input side (ROM $A68D-$A741, $C5FE-$C604, $C612-$C631), decompiled (M4.6c2 spec 2.1, 2.5). Each
   routine takes the registers at entry in r and leaves in r the A, X and Y the ROM leaves at its RTS; r->p is not
   used: no caller reads a flag. A polling loop never spins in C (M4.6c2 spec 2.5): where the ROM waits for input,
   the C waits through the host (FW_WAIT_INPUT); where it would loop with nothing changed, the C waits for an
   interrupt first (FW_WAIT_INTERRUPT). A wait that stops the run (f->stopped) abandons the C: under a thunk a longjmp,
   without a trap the routines return at once. */
#include "fw.h"

#define BOOT_CLAUSE_SPACE_END  0xDC   /* boot_get_clause: from this X on, a space or cmd_char ends the clause ... */
#define BOOT_CLAUSE_FULL       0xFA   /* ... and from this X on any character */

/* $C5FE: in serial mode (boot_dip bit 0 clear) the WIT at $C602: a wait for input (FW_WAIT_INPUT) until an interrupt
   has been handled; in parallel mode it returns at once. A, X and Y kept (PHA ... PLA). */
void isr_idle(fw *f, fw_regs *r);
/* $A697: one clause of input into boot_clause. boot_clause[0] = ' ', then the command character carried over in
   FW_CLAUSE_CARRY ($1827, cleared); ser_abort, tn_in_word, FW_UTT_CONTENT and boot_speaking = 0; ser_rx_idle bit 0
   set while the input ring is empty. A pass with the ring empty: ser_rx_idle bit 0, isr_par_ready, CLI, then with a
   status byte due (ser_status_due bit 0) ser_tx_service, else isr_par_ready and isr_idle; a pass on which the ROM
   would loop with nothing changed (ser_tx_service sent nothing; isr_idle in parallel mode) waits for an interrupt at
   $A6C2. With a byte in the ring: isr_par_hold, SEI, a mute pending (ser_abort bit 0) starts again from the top; else
   the byte is read (ser_rx_read steps on, ser_rx_next_page at a page end), boot_speaking bit 0 set, isr_par_ready,
   CLI. A CR ends the clause unkept; . , : ; ! ? end it kept; so does X reaching $FA, and from X $DC on a space or
   cmd_char (which is carried over, X stepped back over it). A space is then stored and boot_clause_len = X + 1. A =
   ' ', X = boot_clause_len, Y = 0 (or the page ser_rx_next_page gave the last byte). */
void boot_get_clause(fw *f, fw_regs *r);
/* $A68D: boot_get_clause, cmd_scan_clause, tn_convert_clause (C calls C). r as boot_get_clause leaves it: the C
   cmd_scan_clause and tn_convert_clause set no register. */
void boot_loop_text(fw *f, fw_regs *r);
/* $C612: the zero-terminated string at cmd_text_ptr into the input ring (ser_rx_put, Y saved in $78 around each),
   after isr_par_hold and before isr_par_ready. A, X and Y kept. */
void boot_queue_string(fw *f, fw_regs *r);

#endif
