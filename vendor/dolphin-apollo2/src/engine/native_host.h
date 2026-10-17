#ifndef ENGINE_NATIVE_HOST_H
#define ENGINE_NATIVE_HOST_H
/* The native host (M5a spec 3.5): the fw_host of the C firmware on a CPU-less board, with no ROM and no emulated CPU.
   Every call does what the hybrid host (tests/oracle/engine/thunks.c) does on the machine, the cycles included: the
   SFRs, the DSP port and the frame event on the board; each wait on the board's clock, with the hybrid's budgets, the
   phase deadline and its end (a stop at the deadline, else a failure); read_bank $FF; enter records the bank entry;
   the CPU calls the I flag; a ROM call fails. Nothing here includes a machine/ or cpu740/ header. */
#include <stdbool.h>
#include <stdint.h>
#include "board/board.h"
#include "firmware/fw.h"

typedef struct native_host {
    board *b;
    fw f;                      /* the firmware, its RAM the board's */
    fw_host host;
    uint64_t deadline;         /* the engine's phase deadline (m->phase_deadline): a wait that reaches it stops the
                                  run; 0: none (each wait then gets FW_ROM_CALL_BUDGET and fails at its end) */
    board_idle_fn idle;        /* the input wait's idle handler (NULL: none, a stop) */
    void *idle_user;
    bool entered;              /* fw_enter left the C through a bank entry ... */
    uint16_t entry;            /* ... at this address, with A and X: */
    uint8_t entry_a, entry_x;
} native_host;

/* Binds h to b: fw_init on b's RAM with the converted image and h's host calls; b->rom = the image, b->fw = &h->f.
   False when the data image does not build. */
bool native_host_init(native_host *h, board *b);

#endif
