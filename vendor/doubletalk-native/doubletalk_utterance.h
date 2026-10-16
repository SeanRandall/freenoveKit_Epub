#ifndef DOUBLETALK_UTTERANCE_H
#define DOUBLETALK_UTTERANCE_H

#include "doubletalk_scheduler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void dt_utterance_init(dt_scheduler_state *state);

typedef void (*dt_descriptor_observer)(const dt_descriptor *descriptor,
                                       const dt_scheduler_state *state,
                                       const uint8_t *pcm, size_t pcm_length,
                                       void *context);

/* Render one decoded compact stream. State is retained between blocks. */
bool dt_render_bytecode(const uint8_t *rom, const uint8_t *bytecode,
                        size_t length, dt_scheduler_state *state,
                        uint8_t *output, size_t capacity, size_t *written);

bool dt_render_bytecode_observed(const uint8_t *rom, const uint8_t *bytecode,
                                 size_t length, dt_scheduler_state *state,
                                 uint8_t *output, size_t capacity,
                                 size_t *written,
                                 dt_descriptor_observer observer,
                                 void *observer_context);

#ifdef __cplusplus
}
#endif

#endif
