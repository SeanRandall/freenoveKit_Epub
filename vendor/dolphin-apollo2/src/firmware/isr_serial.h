#ifndef FW_ISR_SERIAL_H
#define FW_ISR_SERIAL_H
/* The serial interrupt handlers (ROM $C2B4-$C37D, $C3E0-$C3F9), decompiled (M4.6a spec 2.1, 2.3). isr_int1 and
   isr_int4_host_line are entered by the CPU through their vectors and end with RTI; every path leaves A, X and Y as
   they came (the paths that call a routine push and pull them, the others touch none), so r comes back as it went.
   isr_host_line_high and isr_host_line_low are subroutines (JSR ... RTS) that keep A, X and Y. The C takes no
   emulated time and cannot be interrupted, so the nested interrupts the ROM allows around ser_rx_byte and ser_tx_get
   (PHP, CLI ... SEI, PLP) are taken after the RTI instead: a difference of timing only. */
#include "fw.h"

/* $C2B4 INT1 (vec_int1_serial $FFFA). In serial mode (boot_dip bit 0 clear), ser_int1_serial $C33C: on an _INT1
   request (ICR1 bit 1) the byte on P1_LATCH goes to ser_rx_byte (ser_int1_latch $C367, the _INT1 enable bit 0 off
   meanwhile); otherwise, with the UART receive interrupt (UART_CTRL bit 4) and INT4 (ICR3 bit 6) off and the busy
   line P0.5 set meanwhile, the UART's byte when UART_STATUS has no error bit (2-4); UART_RX is then read and written
   back; P0.3 is cleared at the end. In parallel mode, the host's strobe: P0.0 high clears the _INT1 request and ends;
   P0.4 low releases P1 (and with isr_par_state bit 0 set clears it and P1_LATCH); P0.4 high reads P1_LATCH: 5 asks
   for the next reply byte (ser_tx_get, onto P1), 7 or more is a data byte (P1 = the byte with bit 0 flipped, bits 0
   and 1 when ser_tx_pending bit 0 is set, isr_par_state = 1 or 3, then ser_rx_byte) unless isr_par_state bit 0 is
   set, and anything else is ignored. */
void isr_int1(fw *f, fw_regs *r);
/* $C3E0 INT4 (vec_int4_timerx $FFF4): the host line P0.2 changed. High: isr_host_line_high; low:
   isr_host_line_low. */
void isr_int4_host_line(fw *f, fw_regs *r);
/* $C3EB: ICR3 = $40 (INT4 enabled, its request cleared), the busy line P0.5 set, ser_status_due = 1. */
void isr_host_line_high(fw *f, fw_regs *r);
/* $C3F4: ICR3 = $48, the busy line P0.5 cleared. */
void isr_host_line_low(fw *f, fw_regs *r);

/* The parallel host handshake (ROM $C5D6-$C5EB, M4.6b spec 2.1): subroutines that keep A (PHA ... PLA at $C5EA), X
   and Y, in serial mode (boot_dip bit 0 clear) doing nothing else. */
/* $C5D6: parallel mode: P0.3 set, ready for the next host byte. */
void isr_par_ready(fw *f, fw_regs *r);
/* $C5DE: parallel mode: P0.3 cleared and a pending _INT1 request (ICR1 bit 1) cleared, inside PHP/SEI ... PLP. */
void isr_par_hold(fw *f, fw_regs *r);

#endif
