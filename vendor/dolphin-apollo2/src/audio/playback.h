#ifndef AUDIO_PLAYBACK_H
#define AUDIO_PLAYBACK_H
/* Live playback through the default waveOut device. Our code hands 10 kHz audio to the OS and
   never resamples it (spec R2). */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* True if a device accepts mono 16-bit PCM at `rate` (WAVE_FORMAT_QUERY: nothing is played). */
bool playback_supported(uint32_t rate);

/* Plays and blocks until done. On failure returns false with a message in err. */
bool playback_pcm16(const int16_t *samples, size_t count, uint32_t rate, char *err, size_t errlen);

#endif
