#ifndef FW_CMD_IMM_H
#define FW_CMD_IMM_H
/* The immediate commands (ROM $C413-$C4B4), decompiled (M4.6a spec 2.1, 2.6): matched at interrupt level byte by
   byte as they arrive, and run at once. Each routine takes the registers at entry in r and leaves in r the A, X and
   Y the ROM leaves; r->p is not used. */
#include "fw.h"

/* $C413: in command mode, A = the received byte: stored at ser_cmd_ptr, then ser_cmd_buf matched against
   cmd_imm_table from ser_cmd_table_ofs (cmd_match_table, zero page $44-$49 and $4C restored after). No match:
   command mode off and the buffer passed on as input (ser_rx_flush_cmd). A partial match: ser_cmd_table_ofs = the
   entry matched, ser_cmd_ptr + 1. A full match: cmd_imm_vector = the entry's handler, run with A = the byte at
   ser_cmd_ptr and Y = 0 (the C dispatches by the handler's ROM address: cmd_imm_index_add or cmd_imm_index_query;
   any other is a fault). A and X kept; Y = the entry (partial), else as ser_rx_flush_cmd leaves it (no match, @I+)
   or 0 (@I?). */
void cmd_imm_match(fw *f, fw_regs *r);
/* $C483 @I+, entered by cmd_imm_match's JMP (cmd_imm_vector): the first after a mute (cmd_index_reset bit 0) zeroes
   cmd_index_count and clears the bit; cmd_index_count + 1; command mode off and the command passed on as input
   (ser_rx_flush_cmd), where cmd_mark_index marks its place in the speech. It ends with JMP cmd_imm_done. */
void cmd_imm_index_add(fw *f, fw_regs *r);
/* $C495 @I?: the reply I, cmd_index_count in hex, then T, or M when ser_rx_idle_saved bit 0 is set and boot_speaking
   bit 0 clear; command mode off and ser_rx_idle = ser_rx_idle_saved (A = it), so the query leaves no text. It ends
   with JMP cmd_imm_done. */
void cmd_imm_index_query(fw *f, fw_regs *r);

#endif
