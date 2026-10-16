#ifndef DOUBLETALK_SCHEDULER_H
#define DOUBLETALK_SCHEDULER_H

#include "doubletalk_frame.h"
#include "doubletalk_voice.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    dt_frame_state frame;
    uint8_t repeat_count;       /* 02be */
    uint8_t rate_control;       /* 02b8 */
    uint8_t pitch_delta;        /* 02b9 */
    uint8_t pending_pitch;      /* 02ba */
    uint8_t pitch_bias;         /* 02bb */
    uint16_t duration_counter;  /* 02bc */
    uint8_t live_pitch;         /* 02c2 */
    uint8_t target_pitch;       /* 02c3 */
    uint8_t requested_pitch;    /* 02c4 */
    uint8_t prior_phoneme;      /* 02c5 */
    uint8_t current_phoneme;    /* 02c6 */
    uint8_t duration_active;    /* 02cd */
    uint8_t chain_first;        /* 02ce */
    uint8_t force_pitch;        /* 02ca */
    uint8_t pitch_control;      /* RAM 0096 */
    uint8_t pitch_step;         /* RAM 009e */
    uint8_t rate_percent;       /* host boost: 100 = authentic */
    uint16_t rate_table_offset; /* 02cb */
} dt_scheduler_state;

uint16_t dt_scheduler_rate_period(const uint8_t *rom,
                                  dt_scheduler_state *state);

/* Exact normal/no-reverb descriptor scheduler. Appends PCM to output. */
bool dt_render_descriptor(const uint8_t *rom, const dt_descriptor *descriptor,
                          dt_scheduler_state *state, uint8_t *output,
                          size_t capacity, size_t *written);

#ifdef __cplusplus
}
#endif

#endif
