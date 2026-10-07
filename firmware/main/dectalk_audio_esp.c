/* DECtalk is always used through its in-memory output on the reader.  These
   stubs satisfy the desktop audio API without pulling ALSA/dlopen or WinMM
   into the ESP32 build. */
#include <stddef.h>
#include "tts_audio.h"

struct tts_audio { int unused; };

tts_audio_t *tts_audio_open(unsigned device, unsigned *error)
{
    (void)device;
    if (error) *error = 6; /* MMSYSERR_NODRIVER */
    return NULL;
}
int tts_audio_room(tts_audio_t *audio) { (void)audio; return 0; }
int tts_audio_write(tts_audio_t *audio, const int16_t *pcm, int count)
{ (void)audio; (void)pcm; (void)count; return 0; }
int64_t tts_audio_written(tts_audio_t *audio) { (void)audio; return 0; }
int64_t tts_audio_done(tts_audio_t *audio) { (void)audio; return 0; }
void tts_audio_flush(tts_audio_t *audio) { (void)audio; }
void tts_audio_pause(tts_audio_t *audio, int paused)
{ (void)audio; (void)paused; }
void tts_audio_close(tts_audio_t *audio) { (void)audio; }

/* The desktop API can optionally expose an event queue through a pipe.  EVV
   uses the direct callback and never requests that option, but the reference
   API still contains the link-time call. */
int pipe(int descriptors[2])
{
    if (descriptors) descriptors[0] = descriptors[1] = -1;
    return -1;
}
