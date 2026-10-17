#ifndef SYNTH_RESONATOR_H
#define SYNTH_RESONATOR_H
/* Second-order digital resonator (Klatt 1980 form), normalised to unity gain at its centre
   frequency, so the amplitude controls of a parallel bank set the spectral peak levels directly. */

typedef struct {
    float a, b, c;     /* y[n] = a*x[n] + b*y[n-1] + c*y[n-2] */
    float y1, y2;
} resonator;

void resonator_reset(resonator *r);                                   /* clears the state only */
void resonator_set(resonator *r, float f_hz, float bw_hz, float fs);  /* coefficients; state kept */
float resonator_step(resonator *r, float x);

#endif
