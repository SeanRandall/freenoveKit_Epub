/* Converted from the Apollo 2 ROM (M4.6c2 spec 2.4), then reviewed: the boot's command strings and the power-up
   prosody parameters. tests/test_firmware_data.c re-encodes them and compares them byte for byte with the ROM. */
#include "fw_data.h"
#include "../boot_addr.h"
#include "../isr_addr.h"
#include "../ser_addr.h"

/* The self-test's command strings ($A7BE, $A7C5): boot_selftest_sound points cmd_text_ptr at one and
   boot_queue_string queues it into the input ring up to its zero end (C's terminating 0, which the encoder writes).
   Each starts with $18 (Ctrl-X, the cancel), then @T sound effects: one short buzz when the self-test passes, three
   long ones when it fails. */
const char fw_boot_selftest_ok_cmds[7] = "\030@T5, ";
const char fw_boot_selftest_fail_cmds[15] = "\030@T7 @T7 @T7, ";

/* A pitch term, added to each pitch target mod 256 (pros_target, $D2DF: ADC): a negative one is its two's
   complement. */
#define TERM(n) ((uint8_t)(n))

/* boot_prosody_defaults ($C65B): the power-up values of pros_params (boot_load_prosody_defaults), one per @R letter.
   A-T are five per phrase type: the start and end baselines (pros_baseline, pros_baseline_end; a pitch target adds
   pros_baseline / 8), then the stress, nucleus and final terms (pros_mark_phrases, pros_phrase_gen). U-Z are the word
   terms of pros_phrase_gen. */
const uint8_t fw_boot_prosody_defaults[26] = {
    180,         /* A: '.' and ';' (and the default): the start baseline */
    25,          /* B: the end baseline */
    TERM(8),     /* C: the stress term */
    TERM(-10),   /* D: the nucleus term */
    TERM(-20),   /* E: the final term */
    180,         /* F: ',' and ':': the start baseline */
    25,          /* G: the end baseline */
    TERM(8),     /* H: the stress term */
    TERM(0),     /* I: the nucleus term */
    TERM(5),     /* J: the final term */
    195,         /* K: '?': the start baseline */
    25,          /* L: the end baseline */
    TERM(10),    /* M: the stress term */
    TERM(10),    /* N: the nucleus term */
    TERM(25),    /* O: the final term */
    180,         /* P: '!': the start baseline */
    25,          /* Q: the end baseline */
    TERM(8),     /* R: the stress term */
    TERM(10),    /* S: the nucleus term */
    TERM(0),     /* T: the final term */
    TERM(0),     /* U: pros_term_stress of a word's first symbol */
    TERM(0),     /* V: added to pros_term_tail at a word's last symbol */
    TERM(15),    /* W: the accent */
    0,           /* X: no reader */
    TERM(1),     /* Y: a later stress of a word */
    TERM(0),     /* Z: pros_term_stress of a word's later symbols */
};

/* The vector table ($FFF4-$FFFF, M5a spec 3.4): the handler the CPU enters for each interrupt line, line n's vector
   at $FFFC - 2n, and the reset's. The board dispatches each address to its C (src/board/board.c). */
const uint16_t fw_vec_int4_timerx = ROM_ISR_INT4_HOST_LINE;   /* $FFF4, line 4: Timer X / CNTR (INT4, the host line) */
const uint16_t fw_vec_int3_step = ROM_ISR_UNUSED;             /* $FFF6, line 3: the stepper counters */
const uint16_t fw_vec_int2_timers = ROM_ISR_TIMER_DSP;        /* $FFF8, line 2: timers 1-3 (timer 3, the DSP tick) */
const uint16_t fw_vec_int1_serial = ROM_ISR_INT1;             /* $FFFA, line 1: the _INT1 pin and the UART receive */
const uint16_t fw_vec_int0 = ROM_ISR_UNUSED;                  /* $FFFC, line 0: the _INT2 pin */
const uint16_t fw_vec_reset = ROM_BOOT_RESET;                 /* $FFFE: the reset */
