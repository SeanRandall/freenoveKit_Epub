#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"

#define NUMBER(n) \
    extern const uint8_t number_##n##_start[] \
        asm("_binary_remsound_number_" #n "_wav_start"); \
    extern const uint8_t number_##n##_end[] \
        asm("_binary_remsound_number_" #n "_wav_end")
NUMBER(0); NUMBER(5); NUMBER(10); NUMBER(15); NUMBER(20); NUMBER(25);
NUMBER(30); NUMBER(35); NUMBER(40); NUMBER(45); NUMBER(50); NUMBER(55);
NUMBER(60); NUMBER(65); NUMBER(70); NUMBER(75); NUMBER(80); NUMBER(85);
NUMBER(90); NUMBER(95); NUMBER(100);

typedef struct { const uint8_t *start, *end; } clip_t;
static const clip_t clips[] = {
    { number_0_start, number_0_end }, { number_5_start, number_5_end },
    { number_10_start, number_10_end }, { number_15_start, number_15_end },
    { number_20_start, number_20_end }, { number_25_start, number_25_end },
    { number_30_start, number_30_end }, { number_35_start, number_35_end },
    { number_40_start, number_40_end }, { number_45_start, number_45_end },
    { number_50_start, number_50_end }, { number_55_start, number_55_end },
    { number_60_start, number_60_end }, { number_65_start, number_65_end },
    { number_70_start, number_70_end }, { number_75_start, number_75_end },
    { number_80_start, number_80_end }, { number_85_start, number_85_end },
    { number_90_start, number_90_end }, { number_95_start, number_95_end },
    { number_100_start, number_100_end },
};

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16
           | (uint32_t)p[3] << 24;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

void remsound_play_number(i2s_chan_handle_t audio, int value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    value = ((value + 2) / 5) * 5;
    clip_t clip = clips[value / 5];
    const uint8_t *p = clip.start + 12;
    const int16_t *source = NULL;
    size_t frames = 0;
    unsigned rate = 0, channels = 0, bits = 0;
    while (p + 8 <= clip.end) {
        uint32_t size = le32(p + 4);
        const uint8_t *data = p + 8;
        if (data + size > clip.end) return;
        if (!memcmp(p, "fmt ", 4) && size >= 16) {
            channels = le16(data + 2); rate = le32(data + 4);
            bits = le16(data + 14);
        } else if (!memcmp(p, "data", 4) && bits == 16
                   && (channels == 1 || channels == 2) && rate) {
            source = (const int16_t *)data;
            frames = size / (2 * channels);
            break;
        }
        p = data + ((size + 1) & ~1U);
    }
    if (!source) return;
    int16_t out[256 * 2];
    uint64_t phase = 0, step = ((uint64_t)rate << 32) / 48000;
    while ((phase >> 32) < frames) {
        size_t count = 0;
        while (count < 256 && (phase >> 32) < frames) {
            size_t index = phase >> 32;
            int16_t left = source[index * channels];
            int16_t right = channels == 2 ? source[index * 2 + 1] : left;
            out[count * 2] = left; out[count * 2 + 1] = right;
            ++count; phase += step;
        }
        size_t written;
        i2s_channel_write(audio, out, count * 4, &written, portMAX_DELAY);
    }
}
