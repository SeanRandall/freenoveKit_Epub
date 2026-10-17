#include <math.h>
#include "synth/biquad.h"

#define PI_F 3.14159265358979f

static void set(biquad *f, float b0, float b1, float b2, float a0, float a1, float a2)
{
    f->b0 = b0 / a0;
    f->b1 = b1 / a0;
    f->b2 = b2 / a0;
    f->a1 = a1 / a0;
    f->a2 = a2 / a0;
    biquad_reset(f);
}

void biquad_peaking(biquad *f, float f0_hz, float q, float gain_db, float fs)
{
    float a = powf(10.0f, gain_db / 40.0f);
    float w = 2.0f * PI_F * f0_hz / fs;
    float alpha = sinf(w) / (2.0f * q), c = cosf(w);
    set(f, 1.0f + alpha * a, -2.0f * c, 1.0f - alpha * a, 1.0f + alpha / a, -2.0f * c, 1.0f - alpha / a);
}

void biquad_lowpass(biquad *f, float f0_hz, float q, float fs)
{
    float w = 2.0f * PI_F * f0_hz / fs;
    float alpha = sinf(w) / (2.0f * q), c = cosf(w);
    set(f, 0.5f * (1.0f - c), 1.0f - c, 0.5f * (1.0f - c), 1.0f + alpha, -2.0f * c, 1.0f - alpha);
}

void biquad_reset(biquad *f)
{
    f->z1 = 0.0f;
    f->z2 = 0.0f;
}

float biquad_step(biquad *f, float x)
{
    float y = f->b0 * x + f->z1;
    f->z1 = f->b1 * x - f->a1 * y + f->z2;
    f->z2 = f->b2 * x - f->a2 * y;
    return y;
}

double biquad_gain_db(const biquad *f, double hz, double fs)
{
    double w = 2.0 * 3.14159265358979 * hz / fs;
    double nr = f->b0 + f->b1 * cos(w) + f->b2 * cos(2.0 * w), ni = -(f->b1 * sin(w) + f->b2 * sin(2.0 * w));
    double dr = 1.0 + f->a1 * cos(w) + f->a2 * cos(2.0 * w), di = -(f->a1 * sin(w) + f->a2 * sin(2.0 * w));
    return 10.0 * log10((nr * nr + ni * ni) / (dr * dr + di * di));
}
