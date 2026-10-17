#ifndef AUDIO_WAV_H
#define AUDIO_WAV_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Writes mono 16-bit PCM at `rate` Hz with the canonical 44-byte RIFF header. */
bool wav_write(const char *path, const int16_t *samples, size_t count, uint32_t rate);
#endif
