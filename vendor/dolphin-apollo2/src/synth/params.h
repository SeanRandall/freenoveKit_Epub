#ifndef SYNTH_PARAMS_H
#define SYNTH_PARAMS_H
/* Holmes-style parallel-formant synthesizer: structural constants and frame -> parameter mapping (spec §5). */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYNTH_RATE          10000   /* Hz: the firmware's DSP tick; spec R2, never resampled */
#define SYNTH_FRAME_SAMPLES 100     /* one 10 ms frame */
#define SYNTH_FRAME_BYTES   20

/* Synthesizer hypothesis switches (spec §5, step 4) and output filters. Zero-init = hypotheses
   off and both filters off, which is not the production default: use synth_default_options (synth.h). */
typedef struct {
    bool b11_pulse_width;   /* b11 (@M mark-space ratio) sets the glottal open quotient (Rosenberg pulse) */
    bool b13_reset;         /* b13 bit 7 (spectral jump) clears the resonator states at frame start */
    float f0_centre_hz;     /* pitch centre of the utterance (params_f0_centre); 0 = no pitch-range gain */
    bool peak_filter;       /* voiced-source low-end peak (CAL_PEAK_*): restores the fundamental */
    bool lowpass;           /* output reconstruction low-pass (CAL_LPF_HZ), like the DAC's analog filter */
} synth_options;

typedef struct {
    float fn_hz, f1_hz, f2_hz, f3_hz;   /* resonator centre frequencies */
    float alf, a1, a2, a3, ahf;         /* linear peak gains; 0 = channel off */
    float voicing;                      /* V (b9): 1 -> 0 (noise) .. 63 -> 1 (voiced) */
    float f0_hz;
    float open_quotient;                /* 0 = impulse source (see synth_options) */
    bool reset;                         /* clear the resonator states at this frame */
} synth_params;

/* Frame bytes (spec §1.4): 0 ALF, 1 FN, 2 A1, 3 F1, 4 A2, 5 F2, 6 A3, 7 F3, 8 AHF, 9 V, 10 F0,
   11 mark-space ratio, 13 transition flags. Bytes 12 and 14-19 are not used by the synthesizer. */
void params_from_frame(const uint8_t frame[SYNTH_FRAME_BYTES], const synth_options *opt, synth_params *p);

/* F0 of pitch code b10 before the range gain: b10 is a period of 160 - 2*code samples at 10 kHz
   (the ROM's mark-space clamp at $B930), clamped to CAL_F0_MIN_HZ..CAL_F0_MAX_HZ. */
float params_f0_hz(int code);

/* Median params_f0_hz of the clearly voiced frames (V >= 40 and A1 > 10) among count contiguous
   SYNTH_FRAME_BYTES-byte frames; 0 if there are none. render passes it as synth_options.f0_centre_hz. */
float params_f0_centre(const uint8_t *frames, size_t count);

#endif
