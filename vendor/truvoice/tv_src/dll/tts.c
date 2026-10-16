/*
 * The 24 tts_* exports of TV_ENG32.DLL (REFERENCE 8.6, 10), over the C
 * engine and front end: the drop-in replacement DLL (AGENTS.md TODO 14).
 * All are cdecl and return a short error code (cgrm.h).
 *
 * A handle here is this file's own struct, not the DLL's 0xbcb8-byte
 * object: callers only pass it back.  The checks, their order and the codes
 * are TV_ENG32's.
 *
 * TRUVOICE_EXACT=1 in the environment makes the front end keep the
 * original's defects (fe_fixes = 0), so that the output is TV_ENG32's; by
 * default they are fixed (REFERENCE 3.4c, 9.7).
 *
 * tts_Speak and the sound card are in speak.c, the user dictionary and
 * tts_UserDic in userdic.c.
 */
#include <stdlib.h>
#include <string.h>

#include "tts.h"

#include <commctrl.h>

#define TTS_CALLBACK_TYPE 1000
#define MAX_HANDLES 2

/* @0x100bbe50 */
extern const uint32_t g_voice_pitch[10];
/* @0x100bbea0 */
extern const uint32_t g_voice_speed[10];

extern int fe_fixes;
extern void *(*fe_malloc_hook)(size_t);

/* The handle table: 0x100ff11c, count 0x100ff118, and its lock */
static CRITICAL_SECTION g_lock;
static TtsHandle *g_handles[MAX_HANDLES];
HINSTANCE tts_instance;             /* 0x10137078 */
static unsigned g_count;

/* The index of h in the table, or -1. @0x1005ef20 */
int tts_validate(const void *h)
{
    unsigned i;

    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_count; i++)
        if (g_handles[i] == h) {
            LeaveCriticalSection(&g_lock);
            return (int)i;
        }
    LeaveCriticalSection(&g_lock);
    return -1;
}

/* Handle_Construct 1005e960 and Handle_Init 1005ea70: the settings of
 * tts_Open and the defaults (voice 0 and its pitch and speed, volume 16),
 * the two locks, and the pause and buffer events (manual, set), and
 * InitCommonControls, as there.  PreFormat and TextIn are on. */
static TtsHandle *handle_new(uint32_t options)
{
    TtsHandle *t = (TtsHandle *)calloc(1, sizeof *t);

    if (t == NULL)
        return NULL;
    tv_settings_default(&t->s, options);
    InitializeCriticalSection(&t->queue_lock);
    InitializeCriticalSection(&t->audio_lock);
    t->pause_event = CreateEventA(NULL, TRUE, TRUE, NULL);
    t->buf_done[0] = CreateEventA(NULL, TRUE, TRUE, NULL);
    t->buf_done[1] = CreateEventA(NULL, TRUE, TRUE, NULL);
    t->dic_event = CreateEventA(NULL, TRUE, TRUE, NULL);
    InitCommonControls();
    t->preformat = 1;
    t->textin_on = 1;
    t->refs = 1;
    return t;
}

/* Handle_Destruct 1005e9b0 */
static void handle_free(TtsHandle *t)
{
    tts_queue_clear(&t->queue);
    free(t->queue.item);
    if (t->pause_event != NULL)
        CloseHandle(t->pause_event);
    if (t->buf_done[0] != NULL)
        CloseHandle(t->buf_done[0]);
    if (t->buf_done[1] != NULL)
        CloseHandle(t->buf_done[1]);
    if (t->dic_event != NULL)
        CloseHandle(t->dic_event);
    if (t->thread != NULL)
        CloseHandle(t->thread);
    DeleteCriticalSection(&t->queue_lock);
    DeleteCriticalSection(&t->audio_lock);
    free(t);
}

/* tts_Close and each speaking thread hold a reference: a thread that is
 * still ending its utterance when tts_Close returns frees the handle. */
void tts_release(TtsHandle *t)
{
    if (InterlockedDecrement(&t->refs) == 0)
        handle_free(t);
}

/* ---- open and close ---------------------------------------------------- */

/* @0x1005ef70 */
TTS_EXPORT tts_Open(UINT device, DWORD options, void **h)
{
    short rate = (short)LOWORD(options);
    WORD format = HIWORD(options);
    TtsHandle *t;

    if (g_count >= MAX_HANDLES)
        return TTS_TOOMANYHANDLES;
    if (h == NULL)
        return TTS_BADPARAM;
    *h = NULL;
    if (rate != 0 && rate != 1)
        return TTS_BADRATE;
    if (format > 3)
        return TTS_BADFORMAT;
    EnterCriticalSection(&g_lock);
    t = handle_new(options);
    if (t == NULL) {
        LeaveCriticalSection(&g_lock);
        return TTS_NOMEMORY;
    }
    t->wfx.wFormatTag = format == 2 ? 7 : format == 3 ? 6 : WAVE_FORMAT_PCM;
    t->wfx.nChannels = 1;
    t->wfx.nSamplesPerSec = t->s.sample_rate;
    t->wfx.nAvgBytesPerSec = format == 0 ? t->s.sample_rate * 2 : t->s.sample_rate;
    t->wfx.nBlockAlign = format == 0 ? 2 : 1;
    t->wfx.wBitsPerSample = format == 0 ? 16 : 8;
    t->wfx.cbSize = 0;
    /* only asks whether the device takes the format; it is opened when
     * speaking starts */
    if (device != DEVICE_FILE_ONLY && waveOutOpen(NULL, device, &t->wfx, 0, 0, WAVE_FORMAT_QUERY) != 0) {
        handle_free(t);
        LeaveCriticalSection(&g_lock);
        return TTS_WAVEOPEN;
    }
    t->device = device;
    tts_userdic_open(g_count == 0);
    g_handles[g_count++] = t;
    LeaveCriticalSection(&g_lock);
    *h = t;
    return 0;
}

/* @0x1005f1b0 */
TTS_EXPORT tts_Close(void *h)
{
    int i;

    EnterCriticalSection(&g_lock);
    i = tts_validate(h);
    if (i < 0) {
        LeaveCriticalSection(&g_lock);
        return TTS_INVALIDHANDLE;
    }
    tts_Stop(h);
    tts_userdic_close((TtsHandle *)h, g_count == 1);
    memmove(&g_handles[i], &g_handles[i + 1], (g_count - i - 1) * sizeof *g_handles);
    g_handles[--g_count] = NULL;
    LeaveCriticalSection(&g_lock);
    tts_release((TtsHandle *)h);
    return 0;
}

/* ---- settings ---------------------------------------------------------- */

/* @0x100600e0 */
TTS_EXPORT tts_SetVoice(void *h, WORD voice)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (voice > 9)
        return TTS_BADSETTING;
    t->s.voice = voice;
    t->s.pitch = (uint16_t)g_voice_pitch[voice];
    t->s.rate = (uint16_t)g_voice_speed[voice];
    return 0;
}

/* @0x10060170 */
TTS_EXPORT tts_SetSpeed(void *h, WORD wpm)
{
    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (wpm < 50 || wpm > 250)
        return TTS_BADSETTING;
    ((TtsHandle *)h)->s.rate = wpm;
    return 0;
}

/* @0x10060270 */
TTS_EXPORT tts_SetPitch(void *h, WORD pitch)
{
    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (pitch < 50 || pitch > 400)
        return TTS_BADSETTING;
    ((TtsHandle *)h)->s.pitch = pitch;
    return 0;
}

/* 16 is the loudest.  @0x100601f0 */
TTS_EXPORT tts_SetVolume(void *h, WORD volume)
{
    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (volume > 16)
        return TTS_BADSETTING;
    ((TtsHandle *)h)->s.volume = volume;
    return 0;
}

/* 0 normal, 1 whisper, 2 monotone.  @0x100607c0 */
TTS_EXPORT tts_SetChr(void *h, WORD chr)
{
    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (chr > 2)
        return TTS_BADSETTING;
    ((TtsHandle *)h)->s.cmode = chr;
    return 0;
}

/* 1 is the email context, stored as 4.  @0x10060730 */
TTS_EXPORT tts_SetCtx(void *h, short ctx)
{
    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (ctx == 0)
        ((TtsHandle *)h)->s.ctx = 0;
    else if (ctx == 1)
        ((TtsHandle *)h)->s.ctx = 4;
    else
        return TTS_BADSETTING;
    return 0;
}

/* The getters check the pointer before the handle. */
#define GETTER(name, expr)                          \
    TTS_EXPORT name(void *h, WORD *v)               \
    {                                               \
        if (v == NULL)                              \
            return TTS_BADPARAM;                    \
        if (tts_validate(h) < 0)                        \
            return TTS_INVALIDHANDLE;               \
        *v = (WORD)(expr);                          \
        return 0;                                   \
    }
/* @0x100600a0 */
GETTER(tts_GetVoice, ((TtsHandle *)h)->s.voice)
/* @0x10060130 */
GETTER(tts_GetSpeed, ((TtsHandle *)h)->s.rate)
/* @0x10060230 */
GETTER(tts_GetPitch, ((TtsHandle *)h)->s.pitch)
/* @0x100601b0 */
GETTER(tts_GetVolume, ((TtsHandle *)h)->s.volume)
/* @0x10060780 */
GETTER(tts_GetChr, ((TtsHandle *)h)->s.cmode)
/* @0x100606e0 */
GETTER(tts_GetCtx, ((TtsHandle *)h)->s.ctx == 4 ? 1 : 0)

/* Context 0, volume 16, voice 0 with its pitch and speed, chr 0.  While
 * speaking, the speaking thread also gets the new pitch, speed and volume.
 * @0x100602b0 */
TTS_EXPORT tts_Reset(void *h)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    t->s.ctx = 0;
    t->s.volume = 16;
    t->s.voice = 0;
    t->s.cmode = 0;
    t->s.pitch = (uint16_t)g_voice_pitch[t->s.voice];
    t->s.rate = (uint16_t)g_voice_speed[t->s.voice];
    if (t->speaking) {
        t->pend_pitch = t->s.pitch;
        t->pend_pitch_on = 1;
        t->pend_volume = t->s.volume;
        t->pend_volume_on = 1;
        t->pend_speed = t->s.rate;
        t->pend_speed_on = 1;
    } else {
        t->pend_pitch_on = t->pend_volume_on = t->pend_speed_on = 0;
    }
    return 0;
}

/* Only type 1000, the end of speech.  @0x10060800 */
TTS_EXPORT tts_CallBack(void *h, short type, TtsCallback fn, DWORD user)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (t->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    if (type != TTS_CALLBACK_TYPE)
        return TTS_BADCALLBACK;
    EnterCriticalSection(&t->audio_lock);
    t->callback = fn;
    t->callback_user = user;
    LeaveCriticalSection(&t->audio_lock);
    return 0;
}

/* ---- speaking (speak.c) ---------------------------------------------------- */

/* The text between ESC[1I and ESC[0I (phoneme input), spoken with the
 * context 0 for the time.  @0x1005f9e0 */
TTS_EXPORT tts_SpeakPhoneme(void *h, const char *ph)
{
    WORD ctx = 0;
    char *buf;
    short r;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (((TtsHandle *)h)->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    buf = (char *)malloc(strlen(ph) + 0x28);
    if (buf == NULL)
        return TTS_NOMEMORY;
    strcpy(buf, "\x1b[1I");
    strcat(buf, ph);
    strcat(buf, "\x1b[0I");
    tts_GetCtx(h, &ctx);
    if (ctx == 1)
        tts_SetCtx(h, 0);
    r = tts_Speak(h, buf);
    if (ctx == 1)
        tts_SetCtx(h, 1);
    free(buf);
    return r;
}

/* ---- files and phonemes ------------------------------------------------ */

static int32_t hfile_write(void *ctx, const void *p, uint32_t n)
{
    UINT w = _lwrite((HFILE)(INT_PTR)ctx, (LPCCH)p, n);

    return w == (UINT)HFILE_ERROR ? -1 : (int32_t)w;
}

static void hfile_seek(void *ctx, int32_t off, int whence)
{
    _llseek((HFILE)(INT_PTR)ctx, off, whence == TV_SEEK_SET ? FILE_BEGIN : FILE_CURRENT);
}

/* type 0 WAVE, 1 raw, 2 AU; synchronous.  @0x1005fb50 */
TTS_EXPORT tts_SaveFile(void *h, HFILE f, const char *text, short type)
{
    TvOut o;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    o.write = hfile_write;
    o.seek = hfile_seek;
    o.ctx = (void *)(INT_PTR)f;
    return (short)tv_save_file(&((TtsHandle *)h)->s, &o, text, type);
}

/* *len gets the length of the phoneme spelling with its NUL; buf (if not
 * NULL) the spelling, or its first size - 1 characters and -1017.
 * @0x10060870 */
TTS_EXPORT tts_Phoneme(void *h, const char *text, char *buf, int size, unsigned *len)
{
    char *ph;
    unsigned n;
    short r = 0;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (tv_phonemes(text, &ph) != 0)
        return TTS_NOMEMORY;
    n = (unsigned)strlen(ph) + 1;
    *len = n;
    if (buf != NULL) {
        if (size < (int)n) {
            r = TTS_BUFFERSMALL;
            if (size > 0) {
                memcpy(buf, ph, (size_t)size - 1);
                buf[size - 1] = 0;
            }
        } else {
            strcpy(buf, ph);
        }
    }
    free(ph);
    return r;
}

/* ---- the user dictionary: userdic.c ------------------------------------ */

/* ---- load and unload ---------------------------------------------------- */

/* TRUVOICE_EXACT: the front end's blocks get 256 spare bytes, because the
 * original writes past the end of some word strings (as tvsay -0). */
static void *padded_malloc(size_t n)
{
    return malloc(n + 256);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    char v[8];

    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        tts_instance = inst;
        InitializeCriticalSection(&g_lock);
        if (GetEnvironmentVariableA("TRUVOICE_EXACT", v, sizeof v) && v[0] == '1') {
            fe_fixes = 0;
            fe_malloc_hook = padded_malloc;
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        DeleteCriticalSection(&g_lock);
    }
    return TRUE;
}
