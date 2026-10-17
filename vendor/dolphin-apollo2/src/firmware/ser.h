#ifndef FW_SER_H
#define FW_SER_H
/* The serial code's receive path and reply output (ROM $C37E-$C3DE, $C3FA-$C412, $C4B5-$C5FD), decompiled (M4.6a
   spec 2.1, 2.2, 2.6). Each routine takes the registers at entry in r and leaves in r the A, X and Y the ROM leaves
   at its RTS; r->p is not used: no caller reads a flag, and the PHP/SEI ... PLP pairs only hold interrupts off,
   which the C, taking no emulated time, needs no more. SFRs are read and written through the host in the ROM's
   order (fw_sfr*), and the UART wait goes through the host too (fw_wait). */
#include "fw.h"

/* The SFRs of the serial code (M4.6a spec 2.5; ghidra740/annotations/sfr.txt) */
#define SFR_UART_TX      0xE4
#define SFR_UART_RX      0xE5
#define SFR_UART_CTRL    0xE6   /* bit 4: receive interrupt enable */
#define SFR_UART_STATUS  0xE7   /* bit 1: transmitter ready; bits 2-4: receive errors */
#define SFR_P1_LATCH     0xF2   /* the parallel data byte (an input latch) */
#define SFR_P1           0xF3
#define SFR_P1_DIR       0xF4
#define SFR_P0           0xF6
#define SFR_ICR3         0xFD   /* bits 6/7: INT4 enable/request */
#define SFR_ICR1         0xFF   /* bits 0/1: _INT1 enable/request */
#define P0_HOST_LINE     2      /* P0.2: the host's handshake line (an input) */
#define P0_BUSY          5      /* P0.5: the busy line to the host */

#define SER_NAK          0x15   /* ser_tx_get's answer when nothing is pending */
#define SER_CTRL_A       0x01   /* a mute */
#define SER_CTRL_X       0x18   /* a mute */

/* $C37E: a received byte A, at interrupt level. Ctrl-A or Ctrl-X mutes (ser_rx_mute $C391): ser_abort and
   cmd_index_reset bit 0 set, cmd_char = ser_cmd_char, ser_rx_idle and ser_rx_idle_saved bit 0 set, command mode off
   (ser_cmd_mode bit 0), the input ring emptied (ser_rx_read = ser_rx_write), A = ser_rx_write's high byte. The
   command character ser_cmd_char starts command mode (ser_rx_cmd_start $C3AC): ser_cmd_mode bit 0 set,
   ser_rx_idle_saved = ser_rx_idle, ser_cmd_ptr = ser_cmd_buf, ser_cmd_table_ofs = 0, the 5 bytes of ser_cmd_buf
   cleared, Y = 0. In command mode cmd_imm_match takes the byte, else ser_rx_put. X kept. */
void ser_rx_byte(fw *f, fw_regs *r);
/* $C3CC: A at ser_rx_write, which moves on (ser_rx_next_page at a page end); ser_rx_idle bit 0 cleared. Y = 0, or the
   new page at a page end; A and X kept. */
void ser_rx_put(fw *f, fw_regs *r);
/* $C3FA: the command character and then ser_cmd_buf up to its first 0 queued as ordinary input (ser_rx_put); A and X
   kept, Y as the last ser_rx_put leaves it. */
void ser_rx_flush_cmd(fw *f, fw_regs *r);
/* $C5EC: Y = the next page of the input ring, Y + 1 with $39 wrapping to $21 (the ring is $2100-$38FF); A and X
   kept. */
void ser_rx_next_page(fw *f, fw_regs *r);

/* $C5F5: Y = the next page of the output ring, Y + 1 with $3A wrapping to $39 (the ring is the one page $3900);
   A and X kept. */
void ser_tx_next_page(fw *f, fw_regs *r);
/* $C4B5: A = the reply byte at ser_tx_read, which moves on (ser_tx_next_page at a page end); ser_tx_count is tested
   before it is counted down, and ser_tx_pending = 0 when it was or becomes 0. Y = the new count (0 when it was 0).
   With ser_tx_pending bit 0 clear: A = NAK, ser_tx_pending = 0, Y kept. X kept. */
void ser_tx_get(fw *f, fw_regs *r);
/* $C4DD: A at ser_tx_write, which moves on; ser_tx_count + 1, ser_tx_pending = 1. A, X and Y kept (the ROM reads
   its byte back from the stack, LDA $0103,X after TSX: the A it pushed). */
void ser_tx_put(fw *f, fw_regs *r);
/* $C504: waits for the UART (FW_WAIT_UART_TX), then sends A (UART_TX); A, X and Y kept. */
void ser_tx_send(fw *f, fw_regs *r);
/* $C55A: A = the hex digit of A & $0F (ser_hex_chars), Y = A & $0F; X kept. */
void ser_hex_digit(fw *f, fw_regs *r);
/* $C53A: the two bytes its caller pushed last become hex digits in place: pair[0] (LDA $0105,X after its TSX: the
   last push) the digit of its low nibble, pair[1] ($0106,X) the digit of its high nibble. A = X; X and Y kept. */
void ser_hex_digits(fw *f, fw_regs *r, uint8_t pair[2]);
/* $C571: A as two hex digits, the low nibble's first (ser_hex_digits on two copies of A, then ser_tx_put of each);
   A, X and Y kept. */
void ser_tx_hex(fw *f, fw_regs *r);
/* $C581: in serial mode (boot_dip bit 0 clear) with the host line P0.2 high: the busy line P0.5 cleared, one reply
   byte sent (ser_tx_get, ser_tx_send) and ser_status_due = ser_tx_pending; when that is 0, after the UART is ready
   again, P0.5 set if P0.2 is still high. A and X kept; Y as ser_tx_get leaves it when it ran. True when it sent a
   byte; false when it returned at once and changed nothing (parallel mode, or P0.2 low: boot_get_clause then waits
   for an interrupt, M4.6c2 spec 2.5). */
bool ser_tx_service(fw *f, fw_regs *r);
/* $C5A4: a record reply: ser_reply_id, then ser_reply_count and ser_reply_size in hex, then ser_reply_count records
   of ser_reply_size bytes from ser_reply_ptr, then CR; ser_reply_count is counted down to 0. A, X and Y kept. */
void ser_tx_records(fw *f, fw_regs *r);

#endif
