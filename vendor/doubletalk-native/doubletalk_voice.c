#include "doubletalk_voice.h"

enum {
    LEFT_TABLE = 0x6240,
    RIGHT_TABLE = 0x7008,
    LIST_BASE = 0x10e60,
    WAVE_BASE = 0x16720,
    ROM_SIZE = 0x80000,
};

static uint16_t rom16(const uint8_t *rom, size_t offset)
{
    return (uint16_t)(rom[offset] | (uint16_t)rom[offset + 1] << 8);
}

int32_t dt_context_chain(const uint8_t *rom, unsigned phase,
                         uint8_t left, uint8_t right)
{
    if (!rom || phase > 1 || left >= DT_PHONEME_COUNT ||
        right >= DT_PHONEME_COUNT)
        return -1;
    size_t table = phase ? RIGHT_TABLE : LEFT_TABLE;
    size_t index = (size_t)left * DT_PHONEME_COUNT + right;
    uint16_t value = rom16(rom, table + index * 2);
    return value == 0xffff ? -1 : value;
}

bool dt_walk_chain(const uint8_t *rom, uint16_t chain_offset,
                   dt_descriptor_callback callback, void *context)
{
    if (!rom || !callback) return false;
    size_t position = LIST_BASE + chain_offset;
    for (;;) {
        if (position + 2 > ROM_SIZE) return false;
        uint16_t page = rom16(rom, position);
        if (page == 0xffff) return true;
        if (position + 6 > ROM_SIZE) return false;
        uint16_t offset = rom16(rom, position + 2);
        dt_descriptor descriptor = {
            .waveform_offset = WAVE_BASE + (uint32_t)page * 0x10000u + offset,
            .flags = rom[position + 4],
            .samples = rom[position + 5],
        };
        if (descriptor.waveform_offset + descriptor.samples > ROM_SIZE)
            return false;
        if (!callback(&descriptor, context)) return true;
        position += 6;
    }
}
