#ifndef DOUBLETALK_VOICE_H
#define DOUBLETALK_VOICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DT_PHONEME_COUNT 42

typedef struct {
    uint32_t waveform_offset;
    uint8_t flags;
    uint8_t samples;
} dt_descriptor;

typedef bool (*dt_descriptor_callback)(const dt_descriptor *descriptor,
                                       void *context);

/* phase 0 is the left-transition table at 0x6240; phase 1 is the right table
 * at 0x7008. Returns -1 for the firmware's 0xffff/no-transition entry. */
int32_t dt_context_chain(const uint8_t *rom, unsigned phase,
                         uint8_t left, uint8_t right);

bool dt_walk_chain(const uint8_t *rom, uint16_t chain_offset,
                   dt_descriptor_callback callback, void *context);

#ifdef __cplusplus
}
#endif

#endif
