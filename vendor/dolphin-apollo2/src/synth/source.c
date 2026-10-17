#include <assert.h>
#include <math.h>
#include "synth/calib.h"
#include "synth/params.h"
#include "synth/source.h"

#define PI_F 3.14159265358979f
#define NOISE_SCALE 1.7320508f   /* uniform on [-sqrt(3), sqrt(3)] has unit power */
#define MOD_LOW 0.25f            /* noise level in the second half of the pitch period */
#define MOD_NORM 1.3719887f      /* 1 / sqrt((1 + MOD_LOW^2) / 2): keeps modulated noise at unit power */

void source_init(source *s)
{
    s->noise_state = 0x12345678u;
    s->phase = 0.0f;
    s->prev_flow = 0.0f;
    biquad_peaking(&s->peak, CAL_PEAK_HZ, CAL_PEAK_Q, CAL_PEAK_DB, (float)SYNTH_RATE);
}

static float noise(source *s)
{
    s->noise_state = s->noise_state * 1664525u + 1013904223u;   /* Numerical Recipes LCG */
    return ((float)(s->noise_state >> 8) / 8388608.0f - 1.0f) * NOISE_SCALE;
}

/* Rosenberg (1971) glottal flow over one period (t = 0..1): rises for 2/3 of the open phase,
   falls for 1/3, then stays closed. */
static float flow(float t, float oq)
{
    float tp = oq * (2.0f / 3.0f), tn = oq / 3.0f;
    if (t < tp)
        return 0.5f * (1.0f - cosf(PI_F * t / tp));
    if (t < oq)
        return cosf(PI_F * (t - tp) / (2.0f * tn));
    return 0.0f;
}

float source_step(source *s, const source_params *p, float fs)
{
    assert(p->f0_hz > 0.0f);            /* divided by below; params_from_frame clamps it to CAL_F0_MIN_HZ and up */
    float inc = p->f0_hz / fs;          /* pitch periods per sample */
    float t0 = fs / p->f0_hz;           /* period in samples */
    float voiced;
    if (p->open_quotient > 0.0f) {
        float oq = p->open_quotient;
        float tp = oq * (2.0f / 3.0f), tn = oq / 3.0f;
        /* the flow derivative has mean square (pi^2/8)(1/tp + 1/tn) / t0^2 per sample */
        float k = t0 / sqrtf(PI_F * PI_F / 8.0f * (1.0f / tp + 1.0f / tn));
        float g = flow(s->phase, oq);
        voiced = (g - s->prev_flow) * k;
        s->prev_flow = g;
    } else {
        voiced = s->phase < inc ? sqrtf(t0) : 0.0f;   /* one impulse per period: unit average power */
    }
    if (p->peak_filter)
        voiced = biquad_step(&s->peak, voiced);
    if (p->voiced_gain_db != 0.0f)
        voiced *= powf(10.0f, p->voiced_gain_db / 20.0f);
    float n = noise(s);
    if (p->voicing > 0.25f && p->voicing < 0.75f)     /* voiced fricative: noise follows the glottal cycle */
        n *= (s->phase < 0.5f ? 1.0f : MOD_LOW) * MOD_NORM;
    s->phase += inc;
    if (s->phase >= 1.0f)
        s->phase -= 1.0f;
    return sqrtf(p->voicing) * voiced + sqrtf(1.0f - p->voicing) * n;
}
