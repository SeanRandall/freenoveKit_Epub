#ifndef RETROREADER_MONOLOGUE_PORT_H
#define RETROREADER_MONOLOGUE_PORT_H

#include <stddef.h>
#include <stdint.h>

typedef struct monologue_voice monologue_voice_t;

monologue_voice_t *monologue_voice_open(void);
void monologue_voice_close(monologue_voice_t *voice);
unsigned monologue_voice_sample_rate(const monologue_voice_t *voice);
unsigned monologue_voice_bits(const monologue_voice_t *voice);
void *monologue_voice_scb(monologue_voice_t *voice);

#endif
