#ifndef SYNTH_SYNTH_H
#define SYNTH_SYNTH_H
/* Holmes-style parallel-formant synthesizer (spec §5). Five second-order resonators (FN, F1, F2,
   F3 and a fixed fourth formant) share one excitation. Their outputs are summed with
   alternating signs. Amplitudes ramp linearly across each frame, or within CAL_STEP_RAMP_SAMPLES
   at a b13 bit 7 transition; frequencies update once per frame; a channel that was silent starts
   from a cleared resonator. Output is 16-bit at 10 kHz, 100 samples per frame. */
#include <stdint.h>
#include "synth/biquad.h"
#include "synth/params.h"
#include "synth/resonator.h"
#include "synth/source.h"

typedef struct {
    synth_options opt;
    source src;
    resonator res[5];   /* FN, F1, F2, F3, F4 */
    biquad lpf[2];      /* 4th-order Butterworth output low-pass, two sections */
    float gain[5];      /* gains reached at the end of the previous frame: the start of each ramp */
    float level;        /* synth units -> int16 (CAL_MASTER_GAIN) */
    float voiced_gain_db;   /* CAL_B11_GAIN_DB when b11 is on, else 0: scales the voiced source only */
    uint64_t clipped;   /* output samples clipped to the int16 range */
} synth;

void synth_default_options(synth_options *opt);       /* both filters on, hypotheses off, no pitch centre */
void synth_init(synth *s, const synth_options *opt);   /* opt NULL = synth_default_options */
void synth_frame(synth *s, const uint8_t frame[SYNTH_FRAME_BYTES], int16_t out[SYNTH_FRAME_SAMPLES]);

#endif
