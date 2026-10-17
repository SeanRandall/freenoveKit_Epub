#ifndef ENGINE_NATIVE_ENGINE_H
#define ENGINE_NATIVE_ENGINE_H
/* The native engine (M5a spec 3.6): the C firmware on the CPU-less board, with no ROM and no emulated CPU, run as the
   ROM engine runs the machine: the reset vector's C (boot_reset) under the boot phase's deadline, the ROM engine's
   idle handler (the input fed at the first input idle, the run stopped at the next), each bank entry's C run afresh,
   cycle for cycle as the all-C hybrid. Nothing here includes a machine/ or cpu740/ header. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "board/board.h"
#include "engine/frame_list.h"
#include "engine/native_host.h"

#define NATIVE_BOOT_CYCLES  100000000ULL   /* ROM_ENGINE_BOOT_CYCLES: the boot phase's budget */
#define NATIVE_SPEAK_CYCLES 400000000ULL   /* ROM_ENGINE_SPEAK_CYCLES: the speaking phase's, from the feed */

/* The board's input pins P0-P4 and the TEST-ONLY timer scale (machine_config without the coverage). */
typedef struct { uint8_t port_in[5]; uint32_t cycle_num, cycle_den; } native_config;

/* Watches a native run (engine_watch): on_frame at every frame event, on_idle at every input idle before the idle
   handler feeds or stops; both get the board, which on_idle may change (an injection). */
typedef struct {
    void (*on_frame)(void *user, board *b);
    void (*on_idle)(void *user, board *b);
    void *user;
    /* Optional product-facing stream seam. Returning false stops the native
       run cleanly after the current produced frame. */
    bool (*on_output)(void *user, const uint8_t frame[ENGINE_FRAME_BYTES]);
} native_watch;

/* A native run's state, the caller's storage: the board (its final state when the run is over; for ENGINE_FAULT its
   fault and fault_addr say which and where), the host and the firmware on it, the bank entries made (unwinds: C call
   chains abandoned at fw_enter) and the entries run (entries: the reset's included). Each run starts it afresh. */
typedef struct native_engine {
    board b;
    native_host h;
    unsigned entries, unwinds;
} native_engine;

/* The machine's defaults (machine_default_config): the switches off (serial mode), P2 pulled to $70, scale 1/1. */
void native_default_config(native_config *cfg);

/* The run (rom_engine_run_input's on the board): the board reset with cfg's pins and scale (NULL: the defaults), the
   reset vector's C run with the firmware's trap set; at the first input idle the frame counters reset, the frames kept
   from then on (from the start too if include_boot), the input fed and the deadline moved to NATIVE_SPEAK_CYCLES from
   there; at the next idle the run stops: ENGINE_IDLE. The phase deadline reached: ENGINE_CYCLE_LIMIT. ENGINE_FAULT:
   input longer than the serial queue, a firmware fault (a read outside the converted data, a failed ROM call: the
   NUL bug's fallback at $A893), an unmapped vector, the DME strobe, code under another bank. produced gets the
   produced frames; raw, when not NULL, the raw DSP-port frames of the speaking phase; watch (may be NULL) sees every
   event. */
engine_status native_engine_run_input(native_engine *e, const native_config *cfg, const engine_input *in,
                                      bool include_boot, frame_list *produced, frame_list *raw,
                                      const native_watch *watch);
/* native_engine_run_input with the text and a CR, no watch. */
engine_status native_engine_speak(native_engine *e, const native_config *cfg, const char *text, bool include_boot,
                                  frame_list *produced, frame_list *raw);

#endif
