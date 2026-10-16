#include "doubletalk_utterance.h"

#include <string.h>

typedef struct {
    const uint8_t *rom;
    dt_scheduler_state *state;
    uint8_t *output;
    size_t capacity;
    size_t position;
    bool failed;
    dt_descriptor_observer observer;
    void *observer_context;
} chain_render;

static bool render_one(const dt_descriptor *descriptor, void *opaque)
{
    chain_render *render = (chain_render *)opaque;
    size_t count = 0;
    if (!dt_render_descriptor(render->rom, descriptor, render->state,
                              render->output + render->position,
                              render->capacity - render->position, &count)) {
        render->failed = true;
        return false;
    }
    render->position += count;
    if (render->observer)
        render->observer(descriptor, render->state,
                         render->output + render->position - count, count,
                         render->observer_context);
    render->state->chain_first = 0;
    return true;
}

static bool render_chain(chain_render *render, int32_t offset)
{
    if (offset < 0) return true;
    render->state->chain_first = 1;
    if (!dt_walk_chain(render->rom, (uint16_t)offset, render_one, render))
        return false;
    return !render->failed;
}

static bool render_phoneme(chain_render *render, uint8_t phoneme)
{
    dt_scheduler_state *state = render->state;
    state->current_phoneme = phoneme;
    int32_t left = dt_context_chain(render->rom, 0, state->prior_phoneme,
                                    state->current_phoneme);
    if (!render_chain(render, left)) return false;

    state->target_pitch = state->requested_pitch;
    if (state->force_pitch == 1) {
        state->live_pitch = state->requested_pitch;
        state->force_pitch = 0;
    }
    state->pitch_bias = (uint8_t)((int8_t)state->requested_pitch * 2 / 19);
    state->pitch_delta = state->pending_pitch;
    state->pending_pitch = 0;
    state->frame.flags |= 0x80;
    state->duration_counter = dt_scheduler_rate_period(render->rom, state);
    state->duration_active = 0;

    int32_t right = dt_context_chain(render->rom, 1, state->prior_phoneme,
                                     state->current_phoneme);
    if (!render_chain(render, right)) return false;
    state->prior_phoneme = state->current_phoneme;
    return true;
}

void dt_utterance_init(dt_scheduler_state *state)
{
    if (!state) return;
    memset(state, 0, sizeof *state);
    state->frame.continuity_sample = 0x80;
    state->frame.last_output = 0x80;
    state->force_pitch = 1;
    state->pitch_control = 5;
    state->pitch_step = 5;
    state->rate_percent = 100;
}

bool dt_render_bytecode(const uint8_t *rom, const uint8_t *bytecode,
                        size_t length, dt_scheduler_state *state,
                        uint8_t *output, size_t capacity, size_t *written)
{
    return dt_render_bytecode_observed(rom, bytecode, length, state, output,
                                       capacity, written, NULL, NULL);
}

bool dt_render_bytecode_observed(const uint8_t *rom, const uint8_t *bytecode,
                                 size_t length, dt_scheduler_state *state,
                                 uint8_t *output, size_t capacity,
                                 size_t *written,
                                 dt_descriptor_observer observer,
                                 void *observer_context)
{
    if (!rom || !bytecode || !state || !output || !written) return false;
    chain_render render = {rom, state, output, capacity, 0, false,
                           observer, observer_context};
    for (size_t i = 0; i < length; ++i) {
        uint8_t code = bytecode[i];
        if (code < 0x2a) {
            if (!render_phoneme(&render, code)) return false;
        } else if (code == 0x64) {
            if (++i >= length || render.position + 2 > capacity) return false;
            output[render.position++] = 0x64;
            output[render.position++] = bytecode[i];
        } else if (code > 0x57) {
            int8_t value = -5;
            if (i + 1 < length) {
                int candidate = (int)bytecode[i + 1] - 0x37;
                if (candidate <= 0 && candidate > -10) {
                    value = (int8_t)candidate;
                    ++i;
                }
            }
            if (code == 0x58) value = (int8_t)-value;
            state->pending_pitch = (uint8_t)value;
        } else if (code < 0x38) {
            int value = ((int)code - 0x32) * 10;
            if (i + 1 < length) {
                int digit = (int)bytecode[i + 1] - 0x37;
                if (digit <= 0 && digit > -10) {
                    value += digit;
                    ++i;
                }
            }
            state->requested_pitch = (uint8_t)(value + 20);
            if (state->prior_phoneme == 0)
                state->live_pitch = state->requested_pitch;
        } else if (code < 0x40) {
            int pitch = (int8_t)state->requested_pitch;
            int delta = (int)state->pitch_step * 2;
            if (code == 0x38) {
                pitch -= delta;
                if (pitch < -60) pitch = -60;
            } else {
                pitch += delta;
                if (pitch > 80) pitch = 80;
            }
            state->requested_pitch = (uint8_t)pitch;
        } else if (code < 0x4a) {
            state->rate_control = code - 0x40;
        } else if (code < 0x54) {
            state->frame.table_bank = code - 0x4a;
        } else if (code < 0x56) {
            if (++i >= length) return false;
            int value = -(int)((int8_t)(bytecode[i] - 0x37));
            (void)value; /* delay/reverb feedback is intentionally unsupported */
        } else if (code == 0x56) {
            if (++i >= length) return false;
            state->frame.tone_mode = (uint8_t)-(int8_t)(bytecode[i] - 0x37);
        } else {
            state->force_pitch = 1;
        }
    }
    *written = render.position;
    return true;
}
