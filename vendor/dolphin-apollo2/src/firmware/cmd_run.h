#ifndef FW_CMD_RUN_H
#define FW_CMD_RUN_H
/* The deferred-command dispatcher (ROM $A880-$A8BF), decompiled (M4.2 spec 2.4). */
#include "fw.h"

/* $A880: x = a cmd_text position. Runs every queued command due there (while cmd_next < cmd_count and
   cmd_next_pos = x), in queue order, through cmd_run_next; it stops when a command abandoned the C (a bank entry).
   X is kept (cmd_run_done); the ROM leaves Y = cmd_next. */
void cmd_run_due(fw *f, uint8_t x);
/* $A893 (with cmd_run_done $A8BD): runs the command cmd_handlers[cmd_next]. With a C handler (every deferred handler,
   bank_cmd_select included): cmd_vector = its address, cmd_next += 1, cmd_next_pos = cmd_positions[cmd_next], then
   the handler with X = cmd_arg_index[cmd_next - 1] and A = boot_clause[X]. Any other address (the scan's latent NUL
   bug, M4.6c1 spec 2.5) leaves RAM untouched and runs the ROM cmd_run_next through fw_rom_call_unhooked with A = X =
   x and Y = cmd_next, which does its own bookkeeping and jumps where it jumps. X (= x) is kept. False when the C was
   abandoned (a failed or unwound ROM call, a bank entry); that only returns here without a trap. */
bool cmd_run_next(fw *f, uint8_t x);

#endif
