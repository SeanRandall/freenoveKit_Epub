#ifndef FW_TN_RULES_H
#define FW_TN_RULES_H
/* The rule-format helpers of text normalisation (ROM $B0C3-$B171), decompiled (M4.0 spec section 3.4). */
#include "fw.h"

/* (tn_rule_ptr),Y: byte y of the current rule (text normalisation's and the letter-to-sound rules'). */
static inline uint8_t tn_rule_byte(fw *f, uint8_t y) { return fw_ind_y(f, (uint8_t)TN_RULE_PTR, y); }

/* tn_append_phonetic: len = the length byte at (tn_rule_ptr),*y. Unless len is 0: $78 = len, *x = tn_phon_len,
   and with the voice on (cmd_voice) the len bytes after the length byte are appended to tn_phonetic (*y ends on
   the last one, $78 counts down to 0, *x and tn_phon_len advance by len). len = 0 leaves *x, *y and $78 alone.
   Returns A = $FF. */
uint8_t tn_append_phonetic(fw *f, uint8_t len, uint8_t *x, uint8_t *y);
/* $B0C3: append the spoken name of c (the output of the first rule of its list); returns X = the old tn_pos,
   and tn_pos += 1. */
uint8_t tn_say_char(fw *f, uint8_t c);
/* $B138: advance *y past the context bytes (>= $1E) of the rule at tn_rule_ptr; returns that count or
   length byte. */
uint8_t tn_rule_skip_text(fw *f, uint8_t *y);
/* $B142: tn_rule_ptr += *y + 1 + a (past the count or length byte at *y and the a bytes it counts); *y = 0. */
void tn_rule_skip_field(fw *f, uint8_t a, uint8_t *y);

#endif
