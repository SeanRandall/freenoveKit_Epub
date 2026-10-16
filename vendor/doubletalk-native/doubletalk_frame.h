#ifndef DOUBLETALK_FRAME_H
#define DOUBLETALK_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t waveform[256];
    uint8_t waveform_length; /* ROM RAM 02bf */
    uint8_t output_length;   /* ROM RAM 02c0 */
    uint8_t flags;           /* ROM RAM 02c7 */
    uint8_t continuity_sample; /* ROM RAM 02c8: prior waveform/source byte */
    uint8_t last_output;       /* ROM RAM 02c9: prior transformed output */
    uint8_t tone_mode;       /* ROM RAM 02b0 */
    uint8_t table_bank;      /* ROM RAM 02b1 */
} dt_frame_state;

/* Exact no-reverb implementation of ROM 84d7c..84e10 and 84667..84710.
 * The caller owns descriptor duration scheduling. */
bool dt_render_frame(const uint8_t *rom, dt_frame_state *state,
                     uint8_t *output, size_t capacity, size_t *written);

#ifdef __cplusplus
}
#endif

#endif
