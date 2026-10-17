#ifndef ENGINE_FRAME_LIST_H
#define ENGINE_FRAME_LIST_H
/* What every engine shares (M5a spec 3.1): the frame list, the input of a run and its status. No machine/ or cpu740/
   header: the ROM and hybrid engines (rom_engine.h includes this) and the native engine use the same types. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ENGINE_FRAME_BYTES 20   /* APOLLO_FRAME_BYTES, BOARD_FRAME_BYTES */

typedef struct {
    uint8_t (*frames)[ENGINE_FRAME_BYTES];
    size_t count, cap;
} frame_list;

/* Appends one frame (aborts when out of memory). */
void frame_list_add(frame_list *fl, const uint8_t frame[ENGINE_FRAME_BYTES]);
void frame_list_free(frame_list *fl);

/* One frame per line, 40 uppercase hex digits + '\n'. Caller frees. */
char *frame_list_hex(const frame_list *fl);

/* The input an engine run feeds at the end of boot: n bytes (raw serial input: a NUL may be among them), then a CR
   when cr is set. */
typedef struct { const uint8_t *bytes; size_t n; bool cr; } engine_input;

/* How an engine run ended, in machine_status's order (MACHINE_IDLE, MACHINE_FAULT, MACHINE_CYCLE_LIMIT): the second
   input idle, a fault, or the phase deadline. */
typedef enum { ENGINE_IDLE, ENGINE_FAULT, ENGINE_CYCLE_LIMIT } engine_status;

#endif
