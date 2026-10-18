/*
    rsynth_api.c - the library's public face (include/rsynth_api.h).

    This file plays the part saymain.c played for `say`: it picks the
    language, sets up a speaker, hands rsynth_init() a sample handler and a
    flush handler, and calls say_string(). Where saymain.c wrote samples to
    a sound card and a file, the handlers here collect them into blocks and
    pass each block to the host's callback.

    Every engine starts each rs_speak* call from a freshly initialised
    synthesizer: same noise seed, same filter state. Output therefore
    depends only on the text and the settings, never on what was spoken
    before, which is what makes the golden tests in tests/ possible. The
    original carried filter state from one utterance into the next; none of
    it is audible, because each utterance ends on silence.

    Copyright (c) 1994,2001-2004 Nick Ing-Simmons (saymain.c, on which the
    handlers below are modelled).
    Copyright (c) 2026 the rsynth-nvda contributors.
    Licensed under the GNU Library General Public License, version 2 or
    (at your option) any later version.
*/
#include "config.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "useconfig.h"
#include "darray.h"
#include "rsynth.h"
#include "lang.h"
#include "say.h"
#include "phones.h"
#include "english.h"
#include "rs_internal.h"
#include "rsynth_api.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static SRWLOCK lock = SRWLOCK_INIT;
#define LOCK() AcquireSRWLockExclusive(&lock)
#define UNLOCK() ReleaseSRWLockExclusive(&lock)
#else
#include <pthread.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)
#endif

/* say.c, text.c and english.c reach the language through this global,
   which saymain.c used to define. English is the only one built. */
lang_t *lang = &English;

/* Samples handed to the callback at a time: about 46 ms at 22050 Hz.
   Small enough that a cancel is heard promptly, large enough that the
   callback is not called per sample. */
#define BLOCK 1024

/* The original defaults, from saymain.c. */
#define DEFAULT_FRAME_MS 10.0
#define DEFAULT_F0 133.0
#define DEFAULT_GAIN 57.0
#define DEFAULT_SMOOTH 0.5

struct rs_engine {
    int sample_rate;
    double frame_ms;
    double speed;
    double smooth;
    double volume;
    int monotone;
    int lexicon;
    double f0;
    double gain;
    double spk[RS_SPK_COUNT];   /* rs_set_speaker values, copied in begin() */
    speaker_t speaker;      /* this engine's own copy; see rs_create */

    /* Per call. */
    rsynth_t *rs;
    rs_audio_fn fn;
    void *user;
    int cancelled;
    int16_t block[BLOCK];
    int nblock;
};

/* The speaker_t field behind each rs_speaker_param, and the range
   rsynth.h's comments give for it. */
static const struct {
    size_t offset;
    double lo, hi;
} spk_field[RS_SPK_COUNT] = {
    { offsetof(speaker_t, F4hz), 1200, 4999 },
    { offsetof(speaker_t, B4hz), 40, 1000 },
    { offsetof(speaker_t, F5hz), 1200, 4999 },
    { offsetof(speaker_t, B5hz), 40, 1000 },
    { offsetof(speaker_t, F6hz), 1200, 4999 },
    { offsetof(speaker_t, B4phz), 40, 1000 },
    { offsetof(speaker_t, B5phz), 40, 1000 },
    { offsetof(speaker_t, B6phz), 40, 2000 },
    { offsetof(speaker_t, FNPhz), 248, 528 },
    { offsetof(speaker_t, BNhz), 40, 1000 },
};

#define SPK_FIELD(sp, i) (*(float *) ((char *) (sp) + spk_field[i].offset))

/* --- sample path -------------------------------------------------------- */

static void
deliver(rs_engine *e)
{
    if (e->nblock && !e->cancelled && e->fn) {
        if (e->fn(e->user, e->block, e->nblock)) {
            e->cancelled = 1;
            /* Patch P3: the element loop in holmes.c and the text loop in
               say.c both stop at the next element or word. */
            if (e->rs)
                e->rs->flags |= RSYNTH_STOP;
        }
    }
    e->nblock = 0;
}

/* saymain.c's clip(), without the peak bookkeeping. */
static int16_t
clip(float input)
{
    long temp = (long) input;
    if (temp < -32767)
        temp = -32767;
    else if (temp > 32767)
        temp = 32767;
    return (int16_t) temp;
}

static void *
on_sample(void *user_data, float sample, unsigned nsamp, rsynth_t *rs)
{
    rs_engine *e = (rs_engine *) user_data;
    (void) nsamp;
    (void) rs;
    if (!e->cancelled) {
        e->block[e->nblock++] = clip(sample * (float) e->volume);
        if (e->nblock == BLOCK)
            deliver(e);
    }
    return user_data;
}

static void *
on_flush(void *user_data, unsigned nsamp, rsynth_t *rs)
{
    rs_engine *e = (rs_engine *) user_data;
    (void) nsamp;
    (void) rs;
    deliver(e);
    return user_data;
}

/* --- per-call set-up ---------------------------------------------------- */

static void
apply_lexicon(const rs_engine *e)
{
    switch (e->lexicon) {
    case RS_LEX_RULES:
        English.lookup = NULL;
        break;
    case RS_LEX_CMU_BRITISH:
        English.lookup = rs_dict_lookup;
        dialect = ph_br;
        break;
    default:
        /* What dict.c's choose_dialect() picks for the CMU dictionary:
           "wash" is W AA1 SH there, so American. */
        English.lookup = rs_dict_lookup;
        dialect = ph_am;
        break;
    }
}

static void
release_rs(rs_engine *e)
{
    if (e->rs) {
        /* Not rsynth_term(): that also frees the phone trie every engine
           shares. The private block is opaque here but plain malloc'd. */
        free(e->rs->pvt);
        free(e->rs);
        e->rs = NULL;
    }
}

/* Called with the lock held. */
static int
begin(rs_engine *e, rs_audio_fn fn, void *user)
{
    long frame;
    release_rs(e);
    e->rs = rsynth_init(e->sample_rate, (float) DEFAULT_FRAME_MS,
                        &e->speaker, on_sample, on_flush, e);
    if (!e->rs || !e->rs->pvt) {
        if (e->rs) {
            free(e->rs);
            e->rs = NULL;
        }
        return -2;
    }
    frame = (long) (e->sample_rate * e->frame_ms / 1000.0 + 0.5);
    e->rs->samples_frame = frame > 0 ? frame : 1;
    e->rs->speed = (float) e->speed;
    e->rs->smooth = (float) e->smooth;
    e->speaker.F0Hz = (float) e->f0;
    e->speaker.Gain0 = (float) e->gain;
    {
        int i;
        for (i = 0; i < RS_SPK_COUNT; i++)
            SPK_FIELD(&e->speaker, i) = (float) e->spk[i];
    }
    if (e->monotone)
        e->rs->flags |= RSYNTH_MONOTONE;
    e->fn = fn;
    e->user = user;
    e->cancelled = 0;
    e->nblock = 0;
    apply_lexicon(e);
    return 0;
}

static int
end(rs_engine *e)
{
    int cancelled = e->cancelled;
    e->fn = NULL;
    e->user = NULL;
    return cancelled ? 1 : 0;
}

/* --- public ------------------------------------------------------------- */

RSYNTH_EXPORT int
rs_api_version(void)
{
    return RS_API_VERSION;
}

RSYNTH_EXPORT const char *
rs_version_string(void)
{
    return "rsynth-nvda 3.0.0 (RSynth 2.x by Nick Ing-Simmons)";
}

RSYNTH_EXPORT rs_engine *
rs_create(int sample_rate)
{
    rs_engine *e;
    if (sample_rate < 8000 || sample_rate > 48000)
        return NULL;
    e = (rs_engine *) calloc(1, sizeof(*e));
    if (!e)
        return NULL;
    e->sample_rate = sample_rate;
    e->frame_ms = DEFAULT_FRAME_MS;
    e->speed = 1.0;
    e->smooth = DEFAULT_SMOOTH;
    e->volume = 1.0;
    e->lexicon = RS_LEX_CMU_AMERICAN;
    e->f0 = DEFAULT_F0;
    e->gain = DEFAULT_GAIN;
    /* rsynth_speaker() fills and returns one static speaker_t; take a copy
       so each engine's pitch and gain are its own. */
    LOCK();
    e->speaker = *rsynth_speaker((float) DEFAULT_F0, (float) DEFAULT_GAIN,
                                 Elements);
    UNLOCK();
    {
        int i;
        for (i = 0; i < RS_SPK_COUNT; i++)
            e->spk[i] = SPK_FIELD(&e->speaker, i);
    }
    return e;
}

RSYNTH_EXPORT void
rs_destroy(rs_engine *e)
{
    if (!e)
        return;
    LOCK();
    release_rs(e);
    UNLOCK();
    free(e);
}

RSYNTH_EXPORT int
rs_sample_rate(const rs_engine *e)
{
    return e ? e->sample_rate : 0;
}

static double
clampd(double v, double lo, double hi)
{
    if (!(v >= lo))         /* also catches NaN */
        return lo;
    return v > hi ? hi : v;
}

/* Setters are plain stores into the engine and never take the lock: the
   lock is held for a whole utterance, audio callback included, and a host
   changing the rate on its UI thread must not wait for that. The values
   are read when an rs_speak* call starts. Call setters from the thread
   that speaks, or between calls (the NVDA driver queues them to its speak
   thread for exactly this reason). */
RSYNTH_EXPORT void
rs_set_frame_ms(rs_engine *e, double ms)
{
    if (e)
        e->frame_ms = clampd(ms, 1.0, 40.0);
}

RSYNTH_EXPORT void
rs_set_speed(rs_engine *e, double speed)
{
    /* Element durations are stored in a byte (phtoelm.c); the longest
       element is 16 frames, so 15 keeps every one below 256. */
    if (e)
        e->speed = clampd(speed, 0.1, 15.0);
}

RSYNTH_EXPORT void
rs_set_f0(rs_engine *e, double hz)
{
    if (e)
        e->f0 = clampd(hz, 40.0, 400.0);
}

RSYNTH_EXPORT void
rs_set_gain_db(rs_engine *e, double db)
{
    /* opsynth.c treats Gain0 - 3 <= 0 as "use 57"; keep above that. */
    if (e)
        e->gain = clampd(db, 4.0, 80.0);
}

RSYNTH_EXPORT void
rs_set_volume(rs_engine *e, double scale)
{
    if (e)
        e->volume = clampd(scale, 0.0, 2.0);
}

RSYNTH_EXPORT void
rs_set_smooth(rs_engine *e, double frac)
{
    if (e)
        e->smooth = clampd(frac, 0.01, 1.0);
}

RSYNTH_EXPORT void
rs_set_monotone(rs_engine *e, int on)
{
    if (e)
        e->monotone = on ? 1 : 0;
}

RSYNTH_EXPORT void
rs_set_lexicon(rs_engine *e, int lexicon)
{
    if (e && lexicon >= RS_LEX_CMU_AMERICAN && lexicon <= RS_LEX_RULES)
        e->lexicon = lexicon;
}

RSYNTH_EXPORT void
rs_set_speaker(rs_engine *e, int param, double value)
{
    if (e && param >= 0 && param < RS_SPK_COUNT)
        e->spk[param] = clampd(value, spk_field[param].lo,
                               spk_field[param].hi);
}

RSYNTH_EXPORT double
rs_get_speaker(const rs_engine *e, int param)
{
    if (!e || param < 0 || param >= RS_SPK_COUNT)
        return -1.0;
    return e->spk[param];
}

RSYNTH_EXPORT int
rs_speaker_range(int param, double *lo, double *hi)
{
    if (param < 0 || param >= RS_SPK_COUNT)
        return -1;
    if (lo)
        *lo = spk_field[param].lo;
    if (hi)
        *hi = spk_field[param].hi;
    return 0;
}

/* Spelling: say.c's spell_out() names one character at a time from the
   language's character-name table, through the letter-to-sound rules. */
static void
spell_phones(const char *ascii, darray_ptr phone)
{
    const char *s;
    for (s = ascii; *s; s++)
        if (!isspace((unsigned char) *s))
            spell_out((char *) s, 1, phone);
}

RSYNTH_EXPORT int
rs_speak(rs_engine *e, const char *utf8, unsigned flags, rs_audio_fn fn,
         void *user)
{
    char *text;
    int rc;
    if (!e || !utf8)
        return -1;
    LOCK();
    rc = begin(e, fn, user);
    if (rc) {
        UNLOCK();
        return rc;
    }
    text = rs_frontend(utf8, (flags & RS_SPEAK_SPELL) != 0);
    if (!text) {
        end(e);
        UNLOCK();
        return -2;
    }
    if (flags & RS_SPEAK_SPELL) {
        darray_t phone;
        darray_init(&phone, sizeof(char), 256);
        spell_phones(text, &phone);
        if (phone.items)
            rsynth_phones(e->rs, (char *) darray_find(&phone, 0), phone.items);
        darray_free(&phone);
    } else {
        say_string(e->rs, text);
    }
    free(text);
    rc = end(e);
    UNLOCK();
    return rc;
}

RSYNTH_EXPORT int
rs_speak_phones(rs_engine *e, const char *sampa, rs_audio_fn fn, void *user)
{
    int rc;
    size_t n;
    char *copy;
    if (!e || !sampa)
        return -1;
    n = strlen(sampa);
    /* rsynth_phones takes a char * it does not write through, and the
       phone characters must be ASCII like everything else. */
    copy = (char *) malloc(n + 1);
    if (!copy)
        return -2;
    {
        size_t i;
        for (i = 0; i < n; i++) {
            unsigned char c = (unsigned char) sampa[i];
            copy[i] = (c < 0x20 || c > 0x7E) ? ' ' : (char) c;
        }
        copy[n] = '\0';
    }
    LOCK();
    rc = begin(e, fn, user);
    if (!rc) {
        if (n)
            rsynth_phones(e->rs, copy, (int) n);
        rc = end(e);
    }
    UNLOCK();
    free(copy);
    return rc;
}

static int
copy_out(const char *s, size_t n, char *out, int size)
{
    if (out && size > 0) {
        size_t k = n < (size_t) size - 1 ? n : (size_t) size - 1;
        memcpy(out, s, k);
        out[k] = '\0';
    }
    return (int) n;
}

RSYNTH_EXPORT int
rs_text_to_phones(rs_engine *e, const char *utf8, unsigned flags, char *out,
                  int size)
{
    char *text;
    darray_t phone;
    int n;
    if (!e || !utf8)
        return -1;
    LOCK();
    apply_lexicon(e);
    text = rs_frontend(utf8, (flags & RS_SPEAK_SPELL) != 0);
    if (!text) {
        UNLOCK();
        return -2;
    }
    darray_init(&phone, sizeof(char), 256);
    if (flags & RS_SPEAK_SPELL)
        spell_phones(text, &phone);
    else
        xlate_string(text, &phone);
    n = copy_out(phone.items ? (const char *) darray_find(&phone, 0) : "",
                 phone.items, out, size);
    darray_free(&phone);
    free(text);
    UNLOCK();
    return n;
}

RSYNTH_EXPORT int
rs_normalize(rs_engine *e, const char *utf8, unsigned flags, char *out,
             int size)
{
    char *text;
    int n;
    if (!e || !utf8)
        return -1;
    LOCK();
    apply_lexicon(e);   /* ordinals are pronounced through the lexicon */
    text = rs_frontend(utf8, (flags & RS_SPEAK_SPELL) != 0);
    UNLOCK();
    if (!text)
        return -2;
    n = copy_out(text, strlen(text), out, size);
    free(text);
    return n;
}
