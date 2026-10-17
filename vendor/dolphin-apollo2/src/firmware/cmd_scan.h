#ifndef FW_CMD_SCAN_H
#define FW_CMD_SCAN_H
/* The clause scan of the command parser (ROM $ACFA), decompiled (M4.2 spec 1). */
#include "fw.h"

/* $ACFA: copies boot_clause (boot_clause_len characters, at least one) to cmd_text, codes >= $21 through
   cmd_case_map unless bit 0 of cmd_phonetic is set. At each cmd_char the text after it is matched against
   cmd_table: a match (any Y != 0, a partial one on a zero byte included) is left out of cmd_text and queued as
   command n = cmd_count++ (cmd_positions[n] = its cmd_text position, cmd_handlers[n] = the two bytes after the
   zero that follows the matched part (Y+1, Y+2; on a partial match they are pattern bytes Y+1, Y+2),
   cmd_arg_index[n] = the boot_clause index of its last character); an unmatched cmd_char stays as text, raw. Then
   boot_clause_len = the cmd_text length and cmd_next_pos = cmd_positions[0] ($FF set at the start). isr_par_ready
   (C, M4.6b) runs before each character is read. Scratch: $76, $78. */
void cmd_scan_clause(fw *f);

#endif
