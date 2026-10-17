#ifndef SYNTH_SOURCE_H
#define SYNTH_SOURCE_H
/* Excitation for the parallel-formant bank (spec §5). Voicing (impulse train, or a Rosenberg
   pulse when the b11 hypothesis is on) and noise are mixed by the degree of voicing. In the
   mid range (voiced fricatives) the noise is pitch-modulated. Every excitation has unit average
   power at the source when the low-end peak is off; the peak (CAL_PEAK_*) and voiced_gain_db
   shape the voiced component only, never the noise. */
#include <stdbool.h>
#include <stdint.h>
#include "synth/biquad.h"

typedef struct {
    uint32_t noise_state;   /* LCG state */
    float phase;            /* position in the current pitch period, 0..1 */
    float prev_flow;        /* Rosenberg glottal flow at the previous sample */
    biquad peak;            /* voiced-path low-end peak (CAL_PEAK_*), designed at SYNTH_RATE */
} source;

typedef struct {
    float f0_hz;            /* > 0 (source_step asserts it) */
    float voicing;          /* 0 = noise only .. 1 = voiced only */
    float open_quotient;    /* 0 = unit impulse train; > 0 = Rosenberg pulse with this open quotient */
    bool peak_filter;       /* apply the low-end peak to the voiced component */
    float voiced_gain_db;   /* gain of the voiced component only (0 = unity); the noise is never scaled */
} source_params;

void source_init(source *s);
float source_step(source *s, const source_params *p, float fs);

#endif
