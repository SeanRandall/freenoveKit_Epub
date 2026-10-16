/*
 * TruVoice's file driver (REFERENCE 9.3): tts_SaveFile and what it calls,
 * from TV_ENG32.DLL.  tts_SaveFile is synchronous: it writes the file
 * header, prefixes the text with the handle's settings as escape sequences,
 * and SaveFile_Synthesize runs a whole engine in its own stack frame,
 * writing the audio out one second at a time.
 *
 * In the email context (ctx 4) the prefixed text goes through the front end
 * first (port/email.c).
 */
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "crt.h"
#include "truvoice.h"
#include "bytelist.h"

/* @0x100bbe50 */
extern const uint32_t g_voice_pitch[10];
/* @0x100bbea0 */
extern const uint32_t g_voice_speed[10];

#ifdef _WIN32
#include <windows.h>
static CRITICAL_SECTION tv_cs[TV_LOCKS];
static volatile LONG tv_cs_state[TV_LOCKS];     /* 0 none, 1 being made, 2 made */
void tv_lock(int which)
{
    if (tv_cs_state[which] != 2) {
        if (InterlockedCompareExchange(&tv_cs_state[which], 1, 0) == 0) {
            InitializeCriticalSection(&tv_cs[which]);
            InterlockedExchange(&tv_cs_state[which], 2);
        } else {
            while (tv_cs_state[which] != 2)
                Sleep(0);
        }
    }
    EnterCriticalSection(&tv_cs[which]);
}
void tv_unlock(int which) { LeaveCriticalSection(&tv_cs[which]); }
#else
void tv_lock(int which) { (void)which; }
void tv_unlock(int which) { (void)which; }
#endif

/*
 * Stage 2 keeps part of its working state in globals, 0x10137098-0x101370c0
 * and the breath counter 0x10137104 (stage2.c), which all engines in the
 * process share.  One utterance leaves them to the next.  Two engines that
 * run at once (the two handles, or tts_Speak and tts_SaveFile together) read
 * and write each other's, and in TV_ENG32 what they say then depends on how
 * the threads interleave (REFERENCE 10.7).  Here the engines take their
 * steps one at a time, and when the engine about to step is not the one
 * whose state the globals hold, the globals are swapped: the other engine's
 * go to its save area, and this one's come back, or on its first step the
 * state the last engine to finish left.  An engine running alone never
 * swaps, so it sees what it would in TV_ENG32.  (The globals are zero when
 * the DLL loads: they are past the raw data of .data.)
 */
/* @0x10137098 */ extern int32_t g_s2_count;
/* @0x1013709c */ extern uint8_t g_s2_voiced;
/* @0x101370a0 */ extern Node   *g_s2_last;
/* @0x101370a4 */ extern uint8_t g_s2_value;
/* @0x101370a8 */ extern int32_t g_s2_run;
/* @0x101370ac */ extern int32_t g_s2_state;
/* @0x101370b0 */ extern uint8_t g_s2_hold;
/* @0x101370b4 */ extern int32_t g_s2_b15;
/* @0x101370b8 */ extern int32_t g_s2_saved;
/* @0x101370bc */ extern Node *g_s2_node;
/* @0x101370c0 */ extern int32_t g_s2_total;
/* @0x10137104 */ extern uint32_t g_s2_rnd;

static TvStage2Save *s2_owner;      /* whose state the globals hold, or NULL */
static TvStage2Save s2_rest;        /* the last engine to finish left this */

static void s2_save(TvStage2Save *s)
{
    s->count = g_s2_count;
    s->voiced = g_s2_voiced;
    s->last = g_s2_last;
    s->value = g_s2_value;
    s->run = g_s2_run;
    s->state = g_s2_state;
    s->hold = g_s2_hold;
    s->b15 = g_s2_b15;
    s->saved = g_s2_saved;
    s->node = g_s2_node;
    s->total = g_s2_total;
    s->rnd = g_s2_rnd;
    s->valid = 1;
}

static void s2_load(const TvStage2Save *s)
{
    g_s2_count = s->count;
    g_s2_voiced = s->voiced;
    g_s2_last = (Node *)s->last;
    g_s2_value = s->value;
    g_s2_run = s->run;
    g_s2_state = s->state;
    g_s2_hold = s->hold;
    g_s2_b15 = s->b15;
    g_s2_saved = s->saved;
    g_s2_node = (Node *)s->node;
    g_s2_total = s->total;
    g_s2_rnd = s->rnd;
}

int32_t tv_engine_step(Engine *e, TvStage2Save *s2)
{
    int32_t r;

    tv_lock(TV_LOCK_ENGINE);
    if (s2_owner != s2) {
        /* with no owner the globals are what the last engine left */
        if (s2_owner != NULL) {
            s2_save(s2_owner);
            s2_load(s2->valid ? s2 : &s2_rest);
        } else if (s2->valid)
            s2_load(s2);
        s2_owner = s2;
    }
    r = Engine_Step(e);
    tv_unlock(TV_LOCK_ENGINE);
    return r;
}

void tv_engine_done(TvStage2Save *s2)
{
    tv_lock(TV_LOCK_ENGINE);
    if (s2_owner == s2) {
        s2_save(&s2_rest);
        s2_owner = NULL;
    } else if (s2->valid) {
        /* the globals hold a running engine's; the next one starts from this */
        s2_rest = *s2;
    }
    tv_unlock(TV_LOCK_ENGINE);
}

void tv_settings_default(TvSettings *s, uint32_t options)
{
    memset(s, 0, sizeof *s);
    s->sample_rate = (options & 0xffff) == 1 ? 8000 : 11025;
    s->format = options >> 16;
    /* Handle_Init 1005ea70 */
    s->voice = 0;
    s->volume = 16;
    s->pitch = (uint16_t)g_voice_pitch[0];
    s->rate = (uint16_t)g_voice_speed[0];
    s->cmode = 0;
    s->ctx = 0;
}

/* @0x1005f240 */
void tv_build_prefix(const char *text, char *out, const TvSettings *s)
{
    char *p = out;

    p += sprintf(p, "\x1b[%uV", (unsigned)s->voice);
    p += sprintf(p, "\x1b[%ua", 16u - s->volume);
    p += sprintf(p, "\x1b[%ur", (unsigned)s->rate);
    p += sprintf(p, "\x1b[%up", (unsigned)(s->pitch >> 1));
    p += sprintf(p, "\x1b[%uX", (unsigned)s->ctx);
    p += sprintf(p, "\x1b[%uc", (unsigned)s->cmode);
    *p++ = '\n';
    strcpy(p, text);
}

/* The prefix, then the front end in the email context (tts_SaveFile
 * 1005fb50 and tts_Speak 1005f680 do the same). */
char *tv_prepare_text(const TvSettings *s, const char *text)
{
    char *full = (char *)malloc(strlen(text) + 0x28);
    char *e;

    if (full == NULL)
        return NULL;
    tv_build_prefix(text, full, s);
    if (s->ctx != 4)
        return full;
    e = tv_email_text(full);
    free(full);
    return e;
}

/* Called by SaveFile_Synthesize and Speak_Thread on the last block:
 * trailing silence is dropped, down to two samples for 16-bit and two bytes
 * otherwise. */
/* @0x10001d70 */
void Output_TrimTail(Engine *e)
{
    uint8_t *b = e->out_buf;
    uint8_t quiet;
    uint32_t n;

    if (e->out_format == 0) {
        int16_t v = *(int16_t *)(b + (e->out_count & ~1u) - 2);
        while (v == 0 && e->out_count > 4) {
            e->out_count -= 2;
            v = *(int16_t *)(b + (e->out_count & ~1u) - 2);
        }
        return;
    }
    if (e->out_format == 2)
        quiet = 0xff;
    else if (e->out_format == 3)
        quiet = 0xd5;
    else if (e->out_format == 1)
        quiet = 0x80;
    else
        return;
    if (b[e->out_count - 1] != quiet)
        return;
    while ((n = e->out_count) > 2) {
        e->out_count = n - 1;
        if (b[n - 2] != quiet)
            return;
    }
}

/* Speak_Thread's stop: the input is marked fed and empty, and the rings and
 * TextIn's tokens are dropped.  Only with TextIn on (always, in TruVoice). */
/* @0x10024fa0 */
void Engine_Abort(Engine *e)
{
    TextIn *ti = e->textin;

    if (!e->textin_on || ti == NULL)
        return;
    e->st_input_empty = 1;
    ti->input_done = 1;
    e->item_done = 1;
    ti->item_done = 1;
    Engine_ResetRings(e);
    TextIn_Flush(ti, 0);
    Engine_ResetRings(e);
    e->st_idle = 1;
}

/* The pronunciation manager's way in (UserDic_Speak 100014b0): the text
 * goes to PreFormat character by character, with no TextIn. */
/* @0x10024410 */
void Engine_PutText(Engine *e, const char *text)
{
    static const char tail[] = "\x1b[0i\x1b[C  ";
    int i = 0;

    while (text[i] != '\0') {
        Preformat_PutChar(e, (uint8_t)text[i]);
        i++;
    }
    if (i != 0) {
        e->st_idle = 0;
        for (i = 0; tail[i] != '\0'; i++)
            Preformat_PutChar(e, (uint8_t)tail[i]);
    }
}

/* One second of audio at the handle's rate and format: what out_buf holds
 * before SaveFile_Synthesize writes it out. */
uint32_t tv_second_bytes(const TvSettings *s)
{
    int16_t rate = (int16_t)s->sample_rate;
    uint16_t fmt = (uint16_t)s->format;

    if (fmt == 2 || fmt == 3 || fmt == 1)
        return (uint32_t)((rate / 100) * 100);
    return (uint32_t)((rate / 100) * 200);
}

/* The DLL runs the engine in its frame without clearing it first; the C
 * clears it, which changes nothing the engine reads (Engine_Construct). */
/* @0x10001990 */
int32_t tv_synthesize(const TvOut *o, const char *text, uint8_t *buf, const TvSettings *s)
{
    Engine *e = (Engine *)calloc(1, sizeof *e);
    TvStage2Save s2 = {0};
    uint32_t len, pos = 0;
    int32_t total = 0;
    int32_t r;

    if (e == NULL)
        return -1;
    Engine_Construct(e);
    e->w_212c = 0;
    e->w_212e = 1;
    e->sample_rate = (uint16_t)s->sample_rate;
    e->out_format = (uint16_t)s->format;
    e->out_limit = tv_second_bytes(s);
    e->preformat = 1;
    e->textin_on = 1;
    e->out_count = 0;
    e->out_buf = buf;
    Engine_Init(e);
    if (e->textin_on)
        Engine_CreateTextIn(e);
    len = (uint32_t)strlen(text);
    e->item_done = 0;
    Engine_Feed(e, text, len, &pos);
    Engine_Flush(e, 1);
    for (;;) {
        if (!e->item_done && Engine_InFree(e) > 0x800)
            Engine_Feed(e, text, len, &pos);
        else if (e->st_idle && !e->st_input_empty)
            Engine_Flush(e, 0);
        r = tv_engine_step(e, &s2);
        if (e->out_count >= e->out_limit || ((r & 1) && e->out_count != 0)) {
            if (r & 1)
                Output_TrimTail(e);
            if (o->write(o->ctx, e->out_buf, e->out_count) != (int32_t)e->out_count) {
                total = -1;
                break;
            }
            total += (int32_t)e->out_count;
            e->out_count = 0;
        }
        if ((r & 1) && e->st_input_empty)
            break;
    }
    if (e->textin != NULL)
        tv_delete(e->textin);
    tv_engine_done(&s2);
    free(e);
    return total;
}

static void put32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void put16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

/* 1005fb20: a dword in big-endian order, for the AU header. */
static void put32be(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

/* The WAVE header: RIFF, WAVE, a "fmt " chunk of 18 bytes (the handle's
 * WAVEFORMATEX, as tts_Open 1005ef70 sets it) and the data chunk. */
static void wave_header(uint8_t h[0x2e], const TvSettings *s, uint32_t n)
{
    int lin16 = s->format == 0;

    memcpy(h, "RIFF", 4);
    put32le(h + 4, n + 0x26);
    memcpy(h + 8, "WAVEfmt ", 8);
    put32le(h + 16, 0x12);
    put16le(h + 20, s->format == 2 ? 7 : s->format == 3 ? 6 : 1);
    put16le(h + 22, 1);
    put32le(h + 24, s->sample_rate);
    put32le(h + 28, lin16 ? s->sample_rate * 2 : s->sample_rate);
    put16le(h + 32, lin16 ? 2 : 1);
    put16le(h + 34, lin16 ? 16 : 8);
    put16le(h + 36, 0);
    memcpy(h + 38, "data", 4);
    put32le(h + 42, n);
}

/* The AU header: .snd, data offset 0x18, size, encoding, rate, 1 channel. */
static void au_header(uint8_t h[0x18], const TvSettings *s, uint32_t n)
{
    static const uint32_t enc[4] = { 3, 2, 1, 0x1b };

    memcpy(h, ".snd", 4);
    put32be(h + 4, 0x18);
    put32be(h + 8, n);
    put32be(h + 12, s->format < 4 ? enc[s->format] : 0);
    put32be(h + 16, s->sample_rate);
    put32be(h + 20, 1);
}

/* The buffer is 40000 bytes (calloc(20000, 2)) in the DLL, less than one
 * second of 16-bit audio needs only above 20000 Hz, which tts_Open never
 * sets; the C makes it at least one second. */
/* @0x1005fb50 */
int tv_save_file(const TvSettings *s, const TvOut *o, const char *text, int type)
{
    uint8_t h[0x2e];
    char *full;
    uint8_t *buf;
    size_t bufsize;
    int32_t n;

    if (type != TV_FILE_WAVE && type != TV_FILE_RAW && type != TV_FILE_AU)
        return TV_ERR_FILETYPE;
    full = tv_prepare_text(s, text);
    if (full == NULL)
        return TV_ERR_NOMEMORY;
    bufsize = tv_second_bytes(s) > 40000 ? tv_second_bytes(s) : 40000;
    buf = (uint8_t *)calloc(bufsize, 1);
    if (buf == NULL) {
        free(full);
        return TV_ERR_NOMEMORY;
    }
    if (type == TV_FILE_WAVE) {
        wave_header(h, s, 0);
        if (o->write(o->ctx, h, 0x2e) != 0x2e)
            goto werr;
    } else if (type == TV_FILE_AU) {
        au_header(h, s, 0);
        if (o->write(o->ctx, h, 0x18) != 0x18)
            goto werr;
    }
    n = tv_synthesize(o, full, buf, s);
    free(buf);
    free(full);
    if (n < 0)
        return TV_ERR_WRITE;
    if (type == TV_FILE_RAW)
        return 0;
    o->seek(o->ctx, 0, TV_SEEK_SET);
    if (type == TV_FILE_WAVE) {
        wave_header(h, s, (uint32_t)n);
        if (o->write(o->ctx, h, 0x2e) != 0x2e)
            return TV_ERR_WRITE;
    } else {
        au_header(h, s, (uint32_t)n);
        if (o->write(o->ctx, h, 0x18) != 0x18)
            return TV_ERR_WRITE;
    }
    /* back past the audio, from the end of the header, as the DLL does */
    o->seek(o->ctx, n, TV_SEEK_CUR);
    return 0;
werr:
    free(buf);
    free(full);
    return TV_ERR_WRITE;
}

static int32_t file_write(void *ctx, const void *p, uint32_t n)
{
    return fwrite(p, 1, n, (FILE *)ctx) == n ? (int32_t)n : -1;
}

static void file_seek(void *ctx, int32_t off, int whence)
{
    fseek((FILE *)ctx, off, whence == TV_SEEK_SET ? SEEK_SET : SEEK_CUR);
}

void tv_out_file(TvOut *o, FILE *f)
{
    o->write = file_write;
    o->seek = file_seek;
    o->ctx = f;
}

/* @0x10001d40 (port/stubs.c) */
void TV_THISCALL ByteList_Append(void *self, int32_t b);

/* The engine as tts_Phoneme runs it: no settings prefix, no PreFormat, no
 * audio (w_212e = 0), and Stage 2's phoneme byte list on (w_212c = 1). */
/* @0x10001760 */
int tv_phonemes(const char *text, char **out)
{
    Engine *e = (Engine *)calloc(1, sizeof *e);
    TvStage2Save s2 = {0};
    tv_bytelist list;
    uint32_t len, pos = 0;
    int32_t r;

    *out = NULL;
    if (e == NULL)
        return -1;
    memset(&list, 0, sizeof list);
    Engine_Construct(e);
    e->s2_bytes = &list;
    e->w_212e = 0;
    e->w_212c = 1;
    e->preformat = 0;
    e->textin_on = 1;
    Engine_Init(e);
    if (e->textin_on)
        Engine_CreateTextIn(e);
    len = (uint32_t)strlen(text);
    e->item_done = 0;
    Engine_Feed(e, text, len, &pos);
    Engine_Flush(e, 1);
    do {
        if (!e->item_done && Engine_InFree(e) > 0x800)
            Engine_Feed(e, text, len, &pos);
        else if (e->st_idle && !e->st_input_empty)
            Engine_Flush(e, 0);
        r = tv_engine_step(e, &s2);
    } while (!(r & 1) || !e->st_input_empty);
    ByteList_Append(&list, 0);
    if (e->textin != NULL)
        tv_delete(e->textin);
    tv_engine_done(&s2);
    free(e);
    if (list.failed) {
        tv_bytelist_free(&list);
        return -1;
    }
    *out = list.buf;
    return 0;
}
