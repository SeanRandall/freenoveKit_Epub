#include <math.h>
#include "synth/calib.h"
#include "synth/params.h"

static float map_hz(int log_map, float a, float b, int code)
{
    return log_map ? a * expf(b * (float)code) : a + b * (float)code;
}

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* Keeps every resonator below 0.45 * fs. */
static float freq(int log_map, float a, float b, int code)
{
    return clampf(map_hz(log_map, a, b, code), 50.0f, 0.45f * (float)SYNTH_RATE);
}

static float gain(int code, float offset_db)
{
    if (code <= 1)
        return 0.0f;   /* the firmware clamps amplitudes to 1..63 and uses 1 for silence */
    return powf(10.0f, (CAL_DB_PER_CODE * (float)(code - CAL_REF_CODE) + offset_db) / 20.0f);
}

/* b10 is a pitch period: the ROM clamps the mark-space byte to 47 - 2c <= b11 <= 160 - 2c ($B930),
   so the period is 160 - 2*code samples (spec §1.4). */
#define F0_PERIOD_BASE 160.0f
#define F0_PERIOD_STEP 2.0f

float params_f0_hz(int code)
{
    float period = F0_PERIOD_BASE - F0_PERIOD_STEP * (float)code;
    float min_period = (float)SYNTH_RATE / CAL_F0_MAX_HZ;
    if (period < min_period)
        period = min_period;
    return clampf((float)SYNTH_RATE / period, CAL_F0_MIN_HZ, CAL_F0_MAX_HZ);
}

float params_f0_centre(const uint8_t *frames, size_t count)
{
    size_t hist[256] = { 0 }, voiced = 0, seen = 0;
    for (size_t i = 0; i < count; i++) {
        const uint8_t *f = frames + i * SYNTH_FRAME_BYTES;
        if (f[9] >= 40 && f[2] > 10) {   /* clearly voiced speech */
            hist[f[10]]++;
            voiced++;
        }
    }
    for (int code = 0; code < 256 && voiced > 0; code++) {
        seen += hist[code];
        if (2 * seen >= voiced)          /* the lower median */
            return params_f0_hz(code);
    }
    return 0.0f;
}

void params_from_frame(const uint8_t f[SYNTH_FRAME_BYTES], const synth_options *opt, synth_params *p)
{
    p->fn_hz = freq(CAL_FN_LOG, CAL_FN_A, CAL_FN_B, f[1]);
    p->f1_hz = freq(CAL_F1_LOG, CAL_F1_A, CAL_F1_B, f[3]);
    p->f2_hz = freq(CAL_F2_LOG, CAL_F2_A, CAL_F2_B, f[5]);
    p->f3_hz = freq(CAL_F3_LOG, CAL_F3_A, CAL_F3_B, f[7]);
    p->alf = gain(f[0], CAL_G_ALF_DB + CAL_LTAS_ALF_DB);
    p->a1 = gain(f[2], CAL_G_A1_DB + CAL_LTAS_A1_DB);
    p->a2 = gain(f[4], CAL_G_A2_DB + CAL_LTAS_A2_DB);
    p->a3 = gain(f[6], CAL_G_A3_DB + CAL_LTAS_A3_DB);
    p->ahf = gain(f[8], CAL_G_AHF_DB + CAL_LTAS_AHF_DB);
    p->voicing = clampf((float)(f[9] - 1) / 62.0f, 0.0f, 1.0f);
    p->f0_hz = params_f0_hz(f[10]);
    if (opt->f0_centre_hz > 0.0f)   /* widen the pitch movement around the utterance centre */
        p->f0_hz = clampf(opt->f0_centre_hz * powf(p->f0_hz / opt->f0_centre_hz, CAL_F0_RANGE_GAIN),
                          CAL_F0_MIN_HZ, CAL_F0_MAX_HZ);
    p->open_quotient = opt->b11_pulse_width ? clampf((float)f[11] / 64.0f, 0.1f, 0.9f) : 0.0f;
    p->reset = opt->b13_reset && (f[13] & 0x80) != 0;
}
