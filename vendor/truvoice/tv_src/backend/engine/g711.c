/*
 * TV_ENG32 only: the 8-bit telephone encodings Synth_Generate writes when
 * the handle asks for mu-law or A-law output.  They are Sun's reference
 * G.711 routines (linear2ulaw / linear2alaw) as TruVoice compiled them: no
 * clipping, so a sample of 32767 - 0x84 or more comes out as the largest
 * code rather than being clamped first (the result is the same).
 */
#include "engine.h"

/* Segment end points, one table each (the two are identical). */
/* @0x100e4460 */
extern const int16_t g_ulaw_seg_end[8];
/* @0x100e4470 */
extern const int16_t g_alaw_seg_end[8];

/* Index of the first entry >= val, or n. */
/* @0x1005e4e0 */
int32_t TV_CDECL G711_Search(int32_t val, const int16_t *table, int32_t n)
{
    int32_t i;

    for (i = 0; i < n; i++)
        if (table[i] >= val)
            return i;
    return n;
}

/* @0x1005e480 */
uint8_t TV_CDECL G711_LinearToUlaw(int32_t pcm)
{
    uint8_t mask;
    int32_t seg;

    if (pcm < 0) {
        mask = 0x7f;
        pcm = -pcm;
    } else {
        mask = 0xff;
    }
    pcm += 0x84;
    seg = G711_Search(pcm, g_ulaw_seg_end, 8);
    if (seg >= 8)
        return (uint8_t)(0x7f ^ mask);
    return (uint8_t)(((seg << 4) | ((pcm >> (seg + 3)) & 0xf)) ^ mask);
}

/* @0x1005e510 */
uint8_t TV_CDECL G711_LinearToAlaw(int32_t pcm)
{
    uint8_t mask;
    int32_t seg;
    uint8_t aval;

    if (pcm < 0) {
        mask = 0x55;
        pcm = -pcm - 8;
    } else {
        mask = 0xd5;
    }
    seg = G711_Search(pcm, g_alaw_seg_end, 8);
    if (seg >= 8)
        return (uint8_t)(0x7f ^ mask);
    aval = (uint8_t)(seg << 4);
    if (seg < 2)
        aval |= (uint8_t)((pcm >> 4) & 0xf);
    else
        aval |= (uint8_t)((pcm >> (seg + 3)) & 0xf);
    return (uint8_t)(aval ^ mask);
}
