/*
 * The replacement TV_ENG32.DLL's handle (REFERENCE 10.2), shared by the
 * exports (tts.c) and the speaking thread (speak.c).
 */
#ifndef TV_DLL_TTS_H
#define TV_DLL_TTS_H

#include <windows.h>
#include <mmsystem.h>

#include "truvoice.h"

#define TTS_EXPORT __declspec(dllexport) short __cdecl

/* Error codes (cgrm.h) */
#define TTS_INVALIDHANDLE  (-1000)
#define TTS_WAVEALLOCATED  (-1001)
#define TTS_TOOMANYHANDLES (-1002)
#define TTS_WAVEOPEN       (-1004)
#define TTS_BADRATE        (-1005)
#define TTS_BADFORMAT      (-1006)
#define TTS_BADSETTING     (-1011)
#define TTS_BADCALLBACK    (-1012)
#define TTS_BADPARAM       (-1013)
#define TTS_NOMEMORY       (-1014)
#define TTS_NODEVICE       (-1015)
#define TTS_NOTHREAD       (-1016)
#define TTS_BUFFERSMALL    (-1017)

#define DEVICE_FILE_ONLY ((UINT)-2)

/* Each of the two wave buffers (+0xe4, +0x5ea4) */
#define TTS_WAVE_BUF 24000

typedef void (__cdecl *TtsCallback)(DWORD user);

/* The text queue (+0x00): {count, capacity, array of {data, size}} */
typedef struct TtsQueueItem {
    char *data;
    uint32_t size;
} TtsQueueItem;
typedef struct TtsQueue {
    uint32_t count;
    uint32_t cap_bytes;
    TtsQueueItem *item;
} TtsQueue;

/* The DLL's handle offsets are given for reference: callers only pass the
 * pointer back, so the layout is this file's own. */
typedef struct TtsHandle {
    TvSettings s;               /* voice, rate, pitch, volume, format, chr, context */
    UINT device;                /* +0x88: -1 mapper, -2 file only */
    WAVEFORMATEX wfx;           /* +0x8c */
    TtsCallback callback;       /* +0xbc64 */
    DWORD callback_user;        /* +0xbc68 */
    TtsQueue queue;             /* +0x00 */
    CRITICAL_SECTION queue_lock;/* +0x04 */
    CRITICAL_SECTION audio_lock;/* +0x1c */
    HANDLE pause_event;         /* +0x34, manual, set */
    volatile int paused;        /* +0x38 */
    volatile int stop;          /* +0x3c */
    int audio_users;            /* +0x40: utterances holding the device */
    volatile int speaking;      /* +0x4c */
    HANDLE thread;              /* +0x50 */
    HANDLE buf_done[2];         /* +0x54, +0x58: manual, set; WOM_DONE sets them */
    HANDLE cur_done;            /* +0x5c */
    HANDLE dic_event;           /* +0x60, manual, set: a dictionary window is being made */
    /* +0x68/+0x6e/+0x80 and flags +0x70/+0x74/+0x78: tts_Reset while
     * speaking hands the defaults to the speaking thread */
    uint16_t pend_pitch, pend_speed, pend_volume;
    volatile int pend_pitch_on, pend_speed_on, pend_volume_on;
    HWAVEOUT hwo;               /* +0xa0 */
    WAVEHDR hdr[2];             /* +0xa4, +0xc4 */
    uint8_t preformat;          /* +0xbca8 */
    uint8_t textin_on;          /* +0xbcac */
    int prepared;               /* +0xbcb0: headers prepared, device open */
    HWND dic_window;            /* +0xbcb4: the pronunciation manager */
    /* not in the DLL: the handle is freed when tts_Close and every speaking
     * thread are done with it (the DLL frees it in tts_Close) */
    volatile LONG refs;
    uint8_t buf[2][TTS_WAVE_BUF];
} TtsHandle;

/* tts.c */
extern HINSTANCE tts_instance;
int tts_validate(const void *h);
void tts_release(TtsHandle *t);

/* speak.c */
void tts_queue_clear(TtsQueue *q);
int tts_audio_open(TtsHandle *t);
MMRESULT tts_audio_done(TtsHandle *t, int notify);
int tts_speak_transcribe(TtsHandle *t, const char *text, char **out);

/* userdic.c */
void tts_userdic_open(int first);
void tts_userdic_close(TtsHandle *t, int last);
TTS_EXPORT tts_Speak(void *h, const char *text);
TTS_EXPORT tts_Stop(void *h);

#endif
