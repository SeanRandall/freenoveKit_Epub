#ifndef FW_LTS_H
#define FW_LTS_H
/* Letter-to-sound (ROM $AF11-$B1E5), decompiled (M4.0 spec section 3.4). The behaviour of each routine is
   described in its annotation in ghidra740/annotations/lts.txt. */
#include "fw.h"

/* $AF11: step *y forwards past the next CR LF of the list at lts_affix_ptr; returns the last byte read
   ($0A, or 0 at the $00 list end, where *y stops). */
uint8_t lts_affix_next(fw *f, uint8_t *y);
/* $AF1C: step *y backwards past the previous CR LF ($0D, or 0 at the $00 list start). */
uint8_t lts_affix_prev(fw *f, uint8_t *y);
/* $B007: A = lts_char_class bit 6 of cmd_text[lts_match_pos]; lts_match_pos steps. */
uint8_t lts_sym_vowel(fw *f);
/* $B05E: match one rule context byte; returns 0 for no match. y (the rule index, preserved) is only read by
   the literal symbol and saved to $78 by the affix symbols. */
uint8_t lts_match_symbol(fw *f, uint8_t sym, uint8_t y);
/* $B0E3: try the rules of the list at tn_rule_ptr; true when one matched and its output was appended. */
bool lts_match_list(fw *f);
/* $B172: convert the character c at tn_key_pos. */
void lts_convert_char(fw *f, uint8_t c);

#endif
