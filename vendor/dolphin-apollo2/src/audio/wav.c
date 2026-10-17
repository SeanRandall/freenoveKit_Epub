#include <stdio.h>
#include "audio/wav.h"

static void put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
    put16(p, v);
    put16(p + 2, v >> 16);
}

bool wav_write(const char *path, const int16_t *samples, size_t count, uint32_t rate)
{
    uint8_t h[44] = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0,
                      1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0 };
    uint32_t data = (uint32_t)(count * 2);
    put32(h + 4, 36 + data);
    put32(h + 24, rate);
    put32(h + 28, rate * 2);
    put32(h + 40, data);
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    bool ok = fwrite(h, 1, sizeof h, f) == sizeof h;
    for (size_t i = 0; ok && i < count; i++) {
        uint8_t b[2];
        put16(b, (uint16_t)samples[i]);
        ok = fwrite(b, 1, 2, f) == 2;
    }
    return fclose(f) == 0 && ok;
}
