#ifndef FW_CMD_MATCH_H
#define FW_CMD_MATCH_H
/* The command matcher and the argument helpers of the command parser (M4.2 spec 1, 2.6), decompiled. Patterns
   and tables are read from the converted data; the text from RAM (stale bytes after the clause included). */
#include "fw.h"

/* $C50D: cmd_case_map[c] (upper case; control codes, DEL and many codes above $7F become spaces). */
uint8_t cmd_upcase(fw *f, uint8_t c);
/* $C520: the value of an argument character after cmd_upcase: '0'-'9' -> 0-9, 'A'-'Z' -> 10-35, ':'-'@' -> 35;
   others wrap ('+' -> $FB). Callers range-check it. Z of the ROM exit = (result == 0). */
uint8_t cmd_hex_value(fw *f, uint8_t c);
/* $B450: the byte of the two hex characters in $34 (high) and $35 (low); $1809 = the high value << 4. */
uint8_t cmd_hex_byte(fw *f);
/* $ABDF: $75 = letter; searching back from boot_clause[x - 1] for a character whose upper case is letter,
   stopping at index 1: returns the index after it, or 1 if there is none (boot_clause[0] is never read). */
uint8_t cmd_find_back(fw *f, uint8_t letter, uint8_t x);
/* $AD90: compares the text at cmd_text_ptr (case-mapped) with the pattern at cmd_pat_ptr ('*' any character,
   '#' a digit): returns the index of the first zero (the pattern's end: a full match; the text's: a partial
   one), or 0 on a mismatch. */
uint8_t cmd_match_pattern(fw *f);
/* $ADBF: from entry cmd_table_ofs of the pointer table at cmd_table_ptr, the first pattern that matches: returns
   cmd_match_pattern's Y and leaves cmd_pat_ptr and cmd_table_ofs at that entry; 0 at the $FF end marker. */
uint8_t cmd_match_table(fw *f);

#endif
