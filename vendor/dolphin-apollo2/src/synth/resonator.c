#include <math.h>
#include "synth/resonator.h"

#define PI_F 3.14159265358979f

void resonator_reset(resonator *r)
{
    r->y1 = 0.0f;
    r->y2 = 0.0f;
}

void resonator_set(resonator *r, float f_hz, float bw_hz, float fs)
{
    float rad = expf(-PI_F * bw_hz / fs);
    float theta = 2.0f * PI_F * f_hz / fs;
    r->c = -rad * rad;
    r->b = 2.0f * rad * cosf(theta);
    /* |1 - b e^-jw - c e^-2jw| at w = theta, so the gain at f_hz is exactly 1 */
    float re = 1.0f - r->b * cosf(theta) - r->c * cosf(2.0f * theta);
    float im = r->b * sinf(theta) + r->c * sinf(2.0f * theta);
    r->a = sqrtf(re * re + im * im);
}

float resonator_step(resonator *r, float x)
{
    float y = r->a * x + r->b * r->y1 + r->c * r->y2;
    r->y2 = r->y1;
    r->y1 = y;
    return y;
}
