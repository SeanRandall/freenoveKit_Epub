#include "doubletalk_scheduler.h"

#include <string.h>

enum { RATE_TABLE = 0x48da, TRANSFORM_BASE = 0x4e33 };

static uint16_t rom16(const uint8_t *rom, size_t offset)
{
    return (uint16_t)(rom[offset] | (uint16_t)rom[offset + 1] << 8);
}

uint16_t dt_scheduler_rate_period(const uint8_t *rom,
                                  dt_scheduler_state *state)
{
    int index = (int8_t)state->rate_control;
    if (state->frame.flags & 0x80)
        index += (int8_t)state->pitch_bias;
    else
        index -= (int8_t)state->pitch_control - 5;
    index += (int8_t)state->pitch_delta + 8;
    if (index < 0) index = 0;
    if (index > 22) index = 22;
    state->rate_table_offset = (uint16_t)(index * 2);
    uint16_t period = rom16(rom, RATE_TABLE + state->rate_table_offset);
    /* The emulated card implements rate boost with a private writable overlay
     * over entries 10..22.  The hybrid renderer reads the immutable ROM
     * directly, so apply the same scale here.  Clamp to the authentic table's
     * minimum just as the emulated path does, avoiding the firmware's broken
     * sub-64-frame region and clipped final consonants. */
    unsigned percent = state->rate_percent ? state->rate_percent : 100;
    if (percent != 100 && index >= 10) {
        uint16_t stock_min = 0xffff;
        for (int i = 10; i <= 22; ++i) {
            uint16_t candidate = rom16(rom, RATE_TABLE + (size_t)i * 2);
            if (candidate < stock_min) stock_min = candidate;
        }
        uint32_t scaled = (uint32_t)period * percent / 100u;
        if (scaled < stock_min) scaled = stock_min;
        period = (uint16_t)scaled;
    }
    return period;
}

static bool append(uint8_t *output, size_t capacity, size_t *position,
                   const uint8_t *source, size_t count)
{
    if (*position + count > capacity) return false;
    memcpy(output + *position, source, count);
    *position += count;
    return true;
}

/* ROM 84c91. False means its stack-unwind path ended this descriptor before
 * the current frame was emitted. */
static bool refill_duration(const uint8_t *rom, dt_scheduler_state *state)
{
    state->duration_counter = (uint16_t)(state->duration_counter +
                                         dt_scheduler_rate_period(rom, state));
    if ((int16_t)state->rate_table_offset < 16) {
        state->repeat_count++;
        return true;
    }
    state->repeat_count--;
    return state->repeat_count != 0;
}

static bool duration_allows_frame(const uint8_t *rom,
                                  dt_scheduler_state *state)
{
    bool timed = (state->frame.flags & 0x80) ||
                 (!(state->frame.flags & 0x01) && !state->chain_first);
    if (!timed) return true;
    state->duration_counter = (uint16_t)(state->duration_counter - 0x40);
    if ((int16_t)state->duration_counter > 0) return true;
    return refill_duration(rom, state);
}

static bool voiced_leadin(const uint8_t *rom, dt_scheduler_state *state,
                          uint8_t *output, size_t capacity, size_t *position)
{
    unsigned count = state->frame.output_length;
    if (*position + count > capacity) return false;
    uint8_t source = rom[TRANSFORM_BASE +
                         (size_t)(uint8_t)(state->frame.table_bank + 10) * 256 +
                         state->frame.last_output];
    source = (uint8_t)(source * 2 + 0x80);
    for (unsigned i = 0; i < count; ++i)
        output[(*position)++] = rom[TRANSFORM_BASE +
                                    (size_t)state->frame.table_bank * 256 + source];
    state->frame.last_output = output[*position - 1];
    return true;
}

static void glide_pitch(dt_scheduler_state *state)
{
    int8_t difference = (int8_t)(state->target_pitch - state->live_pitch);
    if (difference) {
        int8_t step = difference >> 4;
        if (step == 0 && difference > 0) step = 1;
        state->live_pitch = (uint8_t)(state->live_pitch + step);
    }
}

bool dt_render_descriptor(const uint8_t *rom, const dt_descriptor *descriptor,
                          dt_scheduler_state *state, uint8_t *output,
                          size_t capacity, size_t *written)
{
    if (!rom || !descriptor || !state || !output || !written) return false;
    size_t position = 0;
    memcpy(state->frame.waveform, rom + descriptor->waveform_offset,
           descriptor->samples);
    memset(state->frame.waveform + descriptor->samples, 0x80,
           sizeof state->frame.waveform - descriptor->samples);
    state->frame.waveform_length = descriptor->samples;
    state->frame.flags = descriptor->flags;
    state->repeat_count = 1;

    if (descriptor->flags & 0x80) {
        do {
            glide_pitch(state);
            uint8_t length = descriptor->samples;
            if ((int8_t)state->live_pitch > 0) {
                if ((int16_t)state->rate_table_offset < 16 ||
                    (int16_t)state->duration_counter > 0x40) {
                    state->frame.output_length = (uint8_t)(
                        state->live_pitch + ((int8_t)state->live_pitch >> 2));
                    if (!voiced_leadin(rom, state, output, capacity, &position))
                        return false;
                }
            } else if (length >= 100) {
                int8_t pitch = (int8_t)state->live_pitch;
                length = (uint8_t)(length + pitch + (pitch >> 2));
            }
            state->frame.output_length = length;
            if (!duration_allows_frame(rom, state)) break;
            uint8_t frame[256]; size_t count = 0;
            if (!dt_render_frame(rom, &state->frame, frame, sizeof frame, &count) ||
                !append(output, capacity, &position, frame, count)) return false;
            state->repeat_count--;
        } while (state->repeat_count);
    } else {
        if (state->duration_active != 1) {
            state->duration_counter = dt_scheduler_rate_period(rom, state);
            state->duration_active = 1;
        }
        state->frame.output_length = descriptor->samples;
        do {
            if (!duration_allows_frame(rom, state)) break;
            uint8_t frame[256]; size_t count = 0;
            if (!dt_render_frame(rom, &state->frame, frame, sizeof frame, &count) ||
                !append(output, capacity, &position, frame, count)) return false;
            if (descriptor->flags & 0x01) break;
            state->repeat_count--;
        } while (state->repeat_count);
    }
    *written = position;
    return true;
}
