#include "doubletalk_frame.h"

#include <stdint.h>

enum { TRANSFORM_BASE = 0x4e33 };

static int16_t sign8(uint8_t value) { return (int8_t)value; }

/* Literal translation of 84e14. It returns the evolving correction in AX and
 * adjusted quotient in BX; the caller applies AL and subtracts BX per tap. */
static void ramp_parameters(int16_t difference, uint8_t frame_length,
                            int16_t *ax, int16_t *bx)
{
    int8_t span = (int8_t)(frame_length - frame_length / 4);
    if (!span) { *ax = difference; *bx = 0; return; }
    int16_t quotient = (int16_t)(difference / span);
    *ax = difference;
    *bx = quotient - 1;
    if (difference >= 0) *bx += 2;
}

static void apply_ramp(uint8_t *waveform, int start, int direction,
                       uint8_t frame_length, int16_t difference)
{
    int16_t ax, bx;
    ramp_parameters(difference, frame_length, &ax, &bx);
    if (!difference) return;
    if (difference >= 0) {
        do {
            waveform[start] = (uint8_t)(waveform[start] + (uint8_t)ax);
            start += direction;
            ax = (int16_t)(ax - bx);
        } while (ax >= 0);
    } else {
        do {
            waveform[start] = (uint8_t)(waveform[start] + (uint8_t)ax);
            start += direction;
            ax = (int16_t)(ax - bx);
        } while (ax < 0);
    }
}

static uint8_t lookup(const uint8_t *rom, uint8_t bank, uint8_t value)
{
    return rom[TRANSFORM_BASE + (size_t)bank * 256 + value];
}

bool dt_render_frame(const uint8_t *rom, dt_frame_state *state,
                     uint8_t *output, size_t capacity, size_t *written)
{
    if (!rom || !state || !output || !written ||
        state->output_length > capacity)
        return false;

    if ((state->flags & 0x80) &&
        state->waveform_length > state->output_length) {
        int16_t difference = sign8((uint8_t)(
            state->waveform[state->waveform_length - 1] -
            state->waveform[state->output_length - 1]));
        if (difference)
            apply_ramp(state->waveform, state->output_length - 1, -1,
                       state->output_length, difference);
    }

    int16_t join = sign8((uint8_t)(state->continuity_sample - state->waveform[0]));
    if (join && ((state->flags & 0x80) || join > 27 || join < -27))
        apply_ramp(state->waveform, 0, 1, state->output_length, join);

    uint8_t previous = state->continuity_sample;
    int mode = 2 - state->tone_mode;
    for (unsigned i = 0; i < state->output_length; ++i) {
        uint8_t source = state->waveform[i];
        uint8_t transformed;
        if (state->flags & 0x01) {
            transformed = source;
        } else if (mode < 0) {
            transformed = source;
        } else if (mode == 0) {
            int8_t centered = (int8_t)(uint8_t)(previous - 0x80);
            int8_t quarter = centered >> 2;
            int8_t value = (int8_t)(centered - quarter - (int8_t)source);
            transformed = (uint8_t)value;
        } else if (mode & 1) {
            int8_t half = (int8_t)(uint8_t)(previous - 0x80) >> 1;
            transformed = (uint8_t)(half - (int8_t)source);
        } else {
            int8_t quarter = (int8_t)(uint8_t)(previous - 0x80) >> 2;
            transformed = (uint8_t)(quarter - (int8_t)source);
        }
        previous = source;
        output[i] = lookup(rom, state->table_bank, transformed);
    }
    state->continuity_sample = previous;
    state->last_output = output[state->output_length - 1];
    *written = state->output_length;
    return true;
}
