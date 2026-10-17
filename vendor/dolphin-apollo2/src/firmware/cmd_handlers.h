#ifndef FW_CMD_HANDLERS_H
#define FW_CMD_HANDLERS_H
/* The deferred command handlers (ROM $A8C0-$ABDE), decompiled (M4.2 spec 2.2). The ROM enters each one by
   JMP (cmd_vector) from cmd_run_next, with X = its cmd_arg_index entry and A = boot_clause[X], and each one ends
   with JMP cmd_run_done, which restores X; cmd_run_due then reloads Y. So a handler has no register output. Only
   the dispatcher (cmd_run.c) calls them. */
#include "fw.h"

typedef void cmd_handler(fw *f, uint8_t x, uint8_t a);

/* The setters without an apply helper (cmd_set.c) */
void cmd_say_version(fw *f, uint8_t x, uint8_t a);        /* $A902 @?: the version texts to tn_phonetic */
void cmd_set_phrase_pause(fw *f, uint8_t x, uint8_t a);   /* $A91F @D: 0-$F */
void cmd_set_word_pause(fw *f, uint8_t x, uint8_t a);     /* $A98A @Q: 0-9 */
void cmd_set_mark_space(fw *f, uint8_t x, uint8_t a);     /* $A99A @M: two digits, 0-$3F */
void cmd_set_spell(fw *f, uint8_t x, uint8_t a);          /* $AA4F @S: 0-1 */
void cmd_set_punct(fw *f, uint8_t x, uint8_t a);          /* $AAF5 @P: 0-1 */
void cmd_set_hyper(fw *f, uint8_t x, uint8_t a);          /* $AB4D @H: 0-1 */
void cmd_set_phonetic(fw *f, uint8_t x, uint8_t a);       /* $AB9F @X: 0-1 */
void cmd_set_formant(fw *f, uint8_t x, uint8_t a);        /* $A92F @U: parameter, two digits, sign */
void cmd_ignore(fw *f, uint8_t x, uint8_t a);             /* $ABDC @G, @I^, @+: nothing */

/* The setters with an apply helper (cmd_set.c) */
void cmd_set_pitch(fw *f, uint8_t x, uint8_t a);          /* $AA24 @F: + - or 0-$F */
void cmd_set_speed(fw *f, uint8_t x, uint8_t a);          /* $A9C9 @W: 1-9 */
void cmd_set_prosody(fw *f, uint8_t x, uint8_t a);        /* $A9DC @R: 0-7, or a letter and two digits */
void cmd_set_source(fw *f, uint8_t x, uint8_t a);         /* $AB5D @$: 0-7 */
void cmd_set_speaker(fw *f, uint8_t x, uint8_t a);        /* $AA61 @K: 0-1 */
void cmd_set_voice(fw *f, uint8_t x, uint8_t a);          /* $AA74 @V: 0 (off) or presets 1-6 */

/* Event producers (cmd_event.c): queue a command event at the phonetic position tn_phon_len */
void cmd_sound(fw *f, uint8_t x, uint8_t a);              /* $A8C0 @T */
void cmd_set_volume(fw *f, uint8_t x, uint8_t a);         /* $AB05 @A */
void cmd_set_voicing(fw *f, uint8_t x, uint8_t a);        /* $AB70 @B */
void cmd_mark_index(fw *f, uint8_t x, uint8_t a);         /* $AB2E @I+ (deferred part) */

/* Replies (cmd_reply.c): queued in the serial transmit ring by the C ser_tx_* routines (ser.c, M4.6a) */
void cmd_echo(fw *f, uint8_t x, uint8_t a);               /* $AB99 @E */
void cmd_query(fw *f, uint8_t x, uint8_t a);              /* $ABD3 @c? */
void cmd_list_languages(fw *f, uint8_t x, uint8_t a);     /* $ABBE @L */

/* Bank switching (bank.c, M4.6c1) */
void bank_cmd_select(fw *f, uint8_t x, uint8_t a);        /* $ABAF @=: the bank cmd_hex_value(a); enters it, or returns
                                                             when it is absent */

/* $A868, a helper of cmd_say_version (cmd_set.c) and cmd_say_phonetic (boot.c, M4.6c2): the length-prefixed phonetic
   text at tn_rule_ptr appended to tn_phonetic (tn_append_phonetic). A and Y kept; returns the X tn_append_phonetic
   leaves (x as it came for a length of 0). */
uint8_t cmd_put_phonetic(fw *f, uint8_t x);

/* The Y a ROM handler is entered with: cmd_next - 1 (cmd_run_due's LDY, before cmd_run_next's INC). */
static inline uint8_t cmd_handler_y(const fw *f) { return (uint8_t)(fw_rd8(f, CMD_NEXT) - 1); }

#endif
