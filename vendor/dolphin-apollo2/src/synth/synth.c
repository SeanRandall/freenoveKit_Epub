#include <math.h>
#include <string.h>
#include "synth/calib.h"
#include "synth/synth.h"

static const float k_sign[5] = { 1.0f, -1.0f, 1.0f, -1.0f, 1.0f };   /* spec §5: alternating signs */

void synth_default_options(synth_options *opt)
{
    memset(opt, 0, sizeof *opt);
    opt->peak_filter = true;
    opt->lowpass = true;
}

void synth_init(synth *s, const synth_options *opt)
{
    memset(s, 0, sizeof *s);
    if (opt)
        s->opt = *opt;
    else
        synth_default_options(&s->opt);
    source_init(&s->src);
    s->level = CAL_MASTER_GAIN * 32767.0f;
    /* The b11 (Rosenberg) hypothesis is level-matched on the voiced path only: the noise keeps its level. */
    s->voiced_gain_db = s->opt.b11_pulse_width ? CAL_B11_GAIN_DB : 0.0f;
    biquad_lowpass(&s->lpf[0], CAL_LPF_HZ, 0.5411961f, (float)SYNTH_RATE);   /* Butterworth Q pair */
    biquad_lowpass(&s->lpf[1], CAL_LPF_HZ, 1.3065630f, (float)SYNTH_RATE);
}

void synth_frame(synth *s, const uint8_t frame[SYNTH_FRAME_BYTES], int16_t out[SYNTH_FRAME_SAMPLES])
{
    synth_params p;
    source_params sp;
    float freq[5], bw[5], target[5];
    params_from_frame(frame, &s->opt, &p);
    freq[0] = p.fn_hz;
    freq[1] = p.f1_hz;
    freq[2] = p.f2_hz;
    freq[3] = p.f3_hz;
    freq[4] = CAL_F4_HZ;
    bw[0] = CAL_BN_HZ;
    bw[1] = CAL_B1_HZ;
    bw[2] = CAL_B2_HZ;
    bw[3] = CAL_B3_HZ;
    bw[4] = CAL_B4_HZ;
    target[0] = p.alf;
    target[1] = p.a1;
    target[2] = p.a2;
    target[3] = p.a3;
    target[4] = p.ahf;
    for (int k = 0; k < 5; k++) {
        /* Gains act on the resonator outputs, so a muted channel's resonator keeps ringing at the
           silence frame's formants and would release that energy as a thump at the next onset
           (glides after silence). A channel that was silent holds no energy. */
        if (p.reset || s->gain[k] == 0.0f)
            resonator_reset(&s->res[k]);
        resonator_set(&s->res[k], freq[k], bw[k], (float)SYNTH_RATE);
    }
    sp.f0_hz = p.f0_hz;
    sp.voicing = p.voicing;
    sp.open_quotient = p.open_quotient;
    sp.peak_filter = s->opt.peak_filter;
    sp.voiced_gain_db = s->voiced_gain_db;
    /* b13 bit 7 marks a transition (closure, release, onset, some vowel/sonorant boundaries): the
       amplitudes reach their targets within CAL_STEP_RAMP_SAMPLES. A full-frame ramp lets the
       vowel's energy thump at a stop closure; an instant step clicks at vowel/sonorant boundaries. */
    int ramp = (frame[13] & 0x80) ? CAL_STEP_RAMP_SAMPLES : SYNTH_FRAME_SAMPLES;
    for (int n = 0; n < SYNTH_FRAME_SAMPLES; n++) {
        float x = source_step(&s->src, &sp, (float)SYNTH_RATE);
        float t = n + 1 >= ramp ? 1.0f : (float)(n + 1) / (float)ramp;   /* reaches the target at the ramp end */
        float y = 0.0f;
        for (int k = 0; k < 5; k++) {
            float g = s->gain[k] + (target[k] - s->gain[k]) * t;
            y += k_sign[k] * g * resonator_step(&s->res[k], x);
        }
        if (s->opt.lowpass)
            y = biquad_step(&s->lpf[1], biquad_step(&s->lpf[0], y));
        y *= s->level;
        if (y > 32767.0f) {
            y = 32767.0f;
            s->clipped++;
        } else if (y < -32768.0f) {
            y = -32768.0f;
            s->clipped++;
        }
        out[n] = (int16_t)lrintf(y);
    }
    memcpy(s->gain, target, sizeof s->gain);
}
