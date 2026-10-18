/*
    rsynth_api.h - the public C interface of rsynth.dll / librsynth.so.

    RSynth is Nick Ing-Simmons' formant synthesizer (1994-2004). This header
    is the only thing a host needs: text goes in as UTF-8, 16-bit mono PCM
    comes out through a callback. The NVDA driver in nvda-addon/ uses exactly
    this, over ctypes.

    Threading: the engine underneath keeps global state (the phone trie, the
    language and dialect tables), so every call that synthesizes or
    translates takes one process-wide lock. Calls from several threads are
    safe; they simply take turns. The audio callback runs on the calling
    thread, inside that lock: it may block (to feed a sound card), but it
    must not call back into this library.

    The rs_set_* setters are the exception: they never take the lock (it is
    held for a whole utterance), so they are plain stores. Call them from
    the thread that speaks, or between calls; they take effect at the start
    of the next rs_speak*.

    Copyright (c) 2026 the rsynth-nvda contributors.
    Licensed under the GNU Library General Public License, version 2 or
    (at your option) any later version.
*/
#ifndef RSYNTH_API_H
#define RSYNTH_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RSYNTH_STATIC)
   /* Linked in as a static library (the GUI does this). */
#  define RSYNTH_EXPORT
#elif defined(_WIN32)
#  if defined(RSYNTH_BUILDING_DLL)
#    define RSYNTH_EXPORT __declspec(dllexport)
#  else
#    define RSYNTH_EXPORT __declspec(dllimport)
#  endif
#elif defined(__GNUC__)
#  define RSYNTH_EXPORT __attribute__((visibility("default")))
#else
#  define RSYNTH_EXPORT
#endif

/* Bumped whenever a function's signature or meaning changes. */
#define RS_API_VERSION 1

typedef struct rs_engine rs_engine;

/* Receives synthesized audio: `count` signed 16-bit mono samples at the
   engine's sample rate. Return 0 to go on, anything else to cancel the rest
   of the current rs_speak* call; synthesis then stops delivering at once. */
typedef int (*rs_audio_fn)(void *user, const int16_t *samples, int count);

/* Pronunciation sources; see docs/03-dictionary.md. */
enum rs_lexicon {
    RS_LEX_CMU_AMERICAN = 0, /* CMU dictionary, American phone mapping (ph_am) */
    RS_LEX_CMU_BRITISH = 1,  /* CMU dictionary, British phone mapping (ph_br) */
    RS_LEX_RULES = 2         /* NRL 1976 letter-to-sound rules only */
};

/* Flags for rs_speak. */
#define RS_SPEAK_SPELL 1u    /* name every character instead of reading words */

RSYNTH_EXPORT int rs_api_version(void);
RSYNTH_EXPORT const char *rs_version_string(void);

/* sample_rate: 8000..48000. The original drivers ran at 8000 or 11025. */
RSYNTH_EXPORT rs_engine *rs_create(int sample_rate);
RSYNTH_EXPORT void rs_destroy(rs_engine *e);
RSYNTH_EXPORT int rs_sample_rate(const rs_engine *e);

/* Frame length in milliseconds (original -m, default 10). Every duration
   in the engine is counted in frames, so this is the speaking rate:
   5 ms speaks twice as fast as 10 ms, with nothing dropped. 1..40. */
RSYNTH_EXPORT void rs_set_frame_ms(rs_engine *e, double ms);
/* Element duration multiplier (original -S, default 1.0). Larger is
   slower. Unlike the frame length it rounds each element to whole frames,
   so values well below 1 drop short elements, as the original did. */
RSYNTH_EXPORT void rs_set_speed(rs_engine *e, double speed);
/* Base voicing frequency in Hz (original -F, default 133). 40..400. */
RSYNTH_EXPORT void rs_set_f0(rs_engine *e, double hz);
/* Overall gain in dB, 60 being unity (original -G, default 57). */
RSYNTH_EXPORT void rs_set_gain_db(rs_engine *e, double db);
/* Output scale applied after synthesis, before clipping: 0..2, default 1. */
RSYNTH_EXPORT void rs_set_volume(rs_engine *e, double scale);
/* Parameter smoothing fraction (original -K, default 0.5); 1 is none. */
RSYNTH_EXPORT void rs_set_smooth(rs_engine *e, double frac);
/* Hold F0 flat (original -M). */
RSYNTH_EXPORT void rs_set_monotone(rs_engine *e, int on);
RSYNTH_EXPORT void rs_set_lexicon(rs_engine *e, int lexicon);

/* Speaker characteristics: the "quasi fixed" fields of speaker_t
   (rsynth.h), which elements.c's rsynth_speaker() fills with constants and
   every element shares. These are what make the voice a different speaker
   without touching the element table. Added after RS_API_VERSION 1 shipped
   in add-on 3.0.0; additions do not bump the version, so a host loading
   the library at run time should look these up by name.

   speaker_t also has B6hz, which nothing in the engine reads (opsynth.c
   uses only the parallel B6phz for F6), so it is not offered. */
enum rs_speaker_param {
    RS_SPK_F4 = 0,  /* F4 frequency, Hz, cascade and parallel  1200..4999 */
    RS_SPK_B4,      /* F4 bandwidth, cascade, Hz                 40..1000 */
    RS_SPK_F5,      /* F5 frequency, Hz, cascade (above 8 kHz
                       sampling only) and parallel             1200..4999 */
    RS_SPK_B5,      /* F5 bandwidth, cascade, Hz                 40..1000 */
    RS_SPK_F6,      /* F6 frequency, Hz, parallel only         1200..4999 */
    RS_SPK_B4P,     /* F4 bandwidth, parallel, Hz                40..1000 */
    RS_SPK_B5P,     /* F5 bandwidth, parallel, Hz                40..1000 */
    RS_SPK_B6P,     /* F6 bandwidth, parallel, Hz                40..2000 */
    RS_SPK_FNP,     /* nasal pole frequency, Hz                   248..528 */
    RS_SPK_BN,      /* nasal pole and zero bandwidth, Hz         40..1000 */
    RS_SPK_COUNT
};
/* Like the other setters: a plain store, clamped to the range, read when
   the next rs_speak* starts. An unknown `param` is ignored. */
RSYNTH_EXPORT void rs_set_speaker(rs_engine *e, int param, double value);
/* The current value; a fresh engine reports the original defaults.
   Returns a negative number for an unknown `param` or a NULL engine. */
RSYNTH_EXPORT double rs_get_speaker(const rs_engine *e, int param);
/* The range rs_set_speaker clamps to (the ranges rsynth.h documents).
   Returns 0, or -1 for an unknown `param`. */
RSYNTH_EXPORT int rs_speaker_range(int param, double *lo, double *hi);

/* Speak UTF-8 text. Returns 0 when finished, 1 when the callback cancelled,
   negative on a bad argument or out of memory. */
RSYNTH_EXPORT int rs_speak(rs_engine *e, const char *utf8, unsigned flags,
                           rs_audio_fn fn, void *user);
/* Speak a SAMPA phone string as `say` would between [ and ]. */
RSYNTH_EXPORT int rs_speak_phones(rs_engine *e, const char *sampa,
                                  rs_audio_fn fn, void *user);

/* Text to SAMPA, as rs_speak would pronounce it. Writes at most `size`
   bytes including the terminating NUL and returns the length the whole
   result needs (excluding the NUL), so a short buffer can be retried. */
RSYNTH_EXPORT int rs_text_to_phones(rs_engine *e, const char *utf8,
                                    unsigned flags, char *out, int size);
/* The text after the front end (docs/04-front-end.md) has made it ASCII
   and rewritten numbers: what the original engine is actually given.
   Same buffer convention as rs_text_to_phones. */
RSYNTH_EXPORT int rs_normalize(rs_engine *e, const char *utf8,
                               unsigned flags, char *out, int size);

#ifdef __cplusplus
}
#endif

#endif /* RSYNTH_API_H */
