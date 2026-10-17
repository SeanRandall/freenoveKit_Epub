#ifndef SYNTH_BIQUAD_H
#define SYNTH_BIQUAD_H
/* Second-order IIR sections from R. Bristow-Johnson's Audio EQ Cookbook, normalised so a0 = 1:
   y = b0 x + b1 x[-1] + b2 x[-2] - a1 y[-1] - a2 y[-2], run in transposed direct form II. */

typedef struct {
    float b0, b1, b2, a1, a2;
    float z1, z2;
} biquad;

/* Peaking EQ: gain_db at f0_hz, unity at DC and Nyquist. Sets coefficients and clears the state. */
void biquad_peaking(biquad *f, float f0_hz, float q, float gain_db, float fs);

/* Low-pass with |H(f0_hz)| = q. Q 0.5411961 and 1.3065630 in cascade make a 4th-order Butterworth.
   Sets coefficients and clears the state. */
void biquad_lowpass(biquad *f, float f0_hz, float q, float fs);

void biquad_reset(biquad *f);
float biquad_step(biquad *f, float x);

/* Analytic |H(e^jw)| in dB at hz. */
double biquad_gain_db(const biquad *f, double hz, double fs);

#endif
