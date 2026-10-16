/*
 * tts_Speak and the sound card (REFERENCE 10.1, 10.2): the text queue, the
 * speaking thread, the wave device and its two buffers, the end-of-speech
 * callback, and tts_Pause / tts_Resume / tts_Stop.
 *
 * tts_Speak queues the prepared text (the settings prefix, and the front end
 * in the email context, as tts_SaveFile); the first one opens the device
 * and starts Speak_Thread, which feeds the queue to an engine and plays its
 * audio one second at a time through two alternating WAVEHDRs.  Texts
 * queued while it speaks join the same utterance: one callback at the end.
 *
 * Device -3 (no thread; events +0xbc84 / +0xbc88) is not ported: tts_Open
 * refuses it (REFERENCE 10.2).
 */
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "crt.h"
#include "bytelist.h"
#include "tts.h"

/* ---- the queue ---------------------------------------------------------- */

/* Grows the array by half as much again.  @0x10060990 */
static int queue_reserve(TtsQueue *q, uint32_t n)
{
    uint32_t bytes;
    TtsQueueItem *p;

    if (n * sizeof *q->item <= q->cap_bytes)
        return 1;
    bytes = n * sizeof *q->item + n * sizeof *q->item / 2;
    p = (TtsQueueItem *)realloc(q->item, bytes);
    if (p == NULL)
        return 0;
    q->item = p;
    q->cap_bytes = bytes;
    return 1;
}

/* A copy of size bytes of data at the end.  @0x10060a10 */
static int queue_push(TtsQueue *q, const char *data, uint32_t size)
{
    char *p;

    if (data == NULL || size == 0 || !queue_reserve(q, q->count + 1))
        return 0;
    p = (char *)malloc(size);
    if (p == NULL)
        return 0;
    memcpy(p, data, size);
    q->item[q->count].data = p;
    q->item[q->count].size = size;
    q->count++;
    return 1;
}

/* @0x10060a90 */
static int queue_remove(TtsQueue *q, uint32_t i)
{
    if (i >= q->count)
        return 0;
    free(q->item[i].data);
    memmove(&q->item[i], &q->item[i + 1], (q->count - i - 1) * sizeof *q->item);
    q->count--;
    return 1;
}

void tts_queue_clear(TtsQueue *q)
{
    while (q->count != 0)
        queue_remove(q, 0);
}

/* ---- the wave device ---------------------------------------------------- */

/* WOM_DONE: the event of the buffer that finished.  @0x1005e7f0 */
static void CALLBACK audio_wave_callback(HWAVEOUT hwo, UINT msg, DWORD_PTR inst, DWORD_PTR p1, DWORD_PTR p2)
{
    TtsHandle *t = (TtsHandle *)inst;

    (void)hwo;
    (void)p2;
    if (msg != WOM_DONE)
        return;
    if ((WAVEHDR *)p1 == &t->hdr[0])
        SetEvent(t->buf_done[0]);
    else if ((WAVEHDR *)p1 == &t->hdr[1])
        SetEvent(t->buf_done[1]);
}

/* The first utterance opens the device and prepares the two headers; the
 * others only count themselves.  @0x1005ed30 */
int tts_audio_open(TtsHandle *t)
{
    MMRESULT r;
    DWORD len;

    if (t->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    EnterCriticalSection(&t->audio_lock);
    if (t->audio_users == 0 && !t->paused) {
        r = waveOutOpen(&t->hwo, t->device, &t->wfx, (DWORD_PTR)audio_wave_callback, (DWORD_PTR)t,
                        CALLBACK_FUNCTION);
        if (r != MMSYSERR_NOERROR) {
            LeaveCriticalSection(&t->audio_lock);
            return r == MMSYSERR_ALLOCATED ? TTS_WAVEALLOCATED : TTS_WAVEOPEN;
        }
        t->hdr[1].dwFlags = 0;
        t->hdr[0].dwFlags = 0;
        t->hdr[0].lpData = (LPSTR)t->buf[0];
        t->prepared = 1;
        t->hdr[1].lpData = (LPSTR)t->buf[1];
        len = t->s.format == 1 ? 11000 : 22000;
        t->hdr[1].dwBufferLength = len;
        t->hdr[0].dwBufferLength = len;
        waveOutPrepareHeader(t->hwo, &t->hdr[0], sizeof t->hdr[0]);
        waveOutPrepareHeader(t->hwo, &t->hdr[1], sizeof t->hdr[1]);
    }
    t->audio_users++;
    LeaveCriticalSection(&t->audio_lock);
    return 0;
}

/* An utterance is over: the last one closes the device and, if notify,
 * calls the tts_CallBack function.  @0x1005ee50 */
MMRESULT tts_audio_done(TtsHandle *t, int notify)
{
    MMRESULT r = 0;

    EnterCriticalSection(&t->audio_lock);
    if (t->audio_users > 0)
        t->audio_users--;
    if (t->audio_users == 0 && t->prepared) {
        waveOutReset(t->hwo);
        waveOutUnprepareHeader(t->hwo, &t->hdr[0], sizeof t->hdr[0]);
        waveOutUnprepareHeader(t->hwo, &t->hdr[1], sizeof t->hdr[1]);
        SetEvent(t->buf_done[0]);
        SetEvent(t->buf_done[1]);
        r = waveOutClose(t->hwo);
        t->hwo = NULL;
        t->prepared = 0;
        if (notify && t->callback != NULL)
            t->callback(t->callback_user);
    }
    LeaveCriticalSection(&t->audio_lock);
    return r;
}

/* ---- the speaking thread ------------------------------------------------ */

/* The engine is set up as SaveFile_Synthesize sets it up (port/truvoice.c)
 * and fed from the queue, each entry with its NUL; an entry is removed
 * once fed.  Audio goes out one second (out_limit) at a time, or at the end
 * of the input, into the two buffers in turn: after each waveOutWrite the
 * thread waits until the other buffer has played.  At the end of the input
 * it waits for both, and when the queue is empty too it ends the
 * utterance.  A stop ends it through Engine_Abort, and tts_Stop ends the
 * utterance.
 *
 * The DLL keeps the engine in the thread's frame (stack 50000); here it is
 * on the heap.  @0x10001000 */
static DWORD WINAPI speak_thread(void *arg)
{
    TtsHandle *t = (TtsHandle *)arg;
    Engine *e = (Engine *)calloc(1, sizeof *e);
    TvStage2Save s2 = {0};
    WAVEHDR *hdr;
    const char *data = NULL;
    uint32_t size = 0, pos = 0;
    int flushed = 0, first = 1;
    int32_t r;

    if (e == NULL) {
        /* (not in the DLL, whose engine is on the stack) */
        EnterCriticalSection(&t->queue_lock);
        t->speaking = 0;
        LeaveCriticalSection(&t->queue_lock);
        tts_audio_done(t, 1);
        tts_release(t);
        return 0;
    }
    Engine_Construct(e);
    e->w_212c = 0;
    e->w_212e = 1;
    e->sample_rate = (uint16_t)t->s.sample_rate;
    e->out_format = (uint16_t)t->s.format;
    e->out_limit = tv_second_bytes(&t->s);
    Engine_Init(e);
    e->preformat = t->preformat;
    e->textin_on = t->textin_on;
    e->out_buf = t->buf[0];
    t->cur_done = t->buf_done[0];
    hdr = &t->hdr[0];
    e->out_count = 0;
    if (e->textin_on)
        Engine_CreateTextIn(e);
    for (;;) {
        if (t->paused)
            WaitForSingleObject(t->pause_event, INFINITE);
        if (t->stop) {
            t->speaking = 0;
            Engine_Abort(e);
            break;
        }
        /* tts_Reset's values; they also lock the rate, volume and pitch
         * against the text's escape sequences */
        if (t->pend_speed_on) {
            e->lock_rate = 1;
            e->cur_speed = t->pend_speed;
            Engine_SetSpeed(e, t->pend_speed);
            t->pend_speed_on = 0;
        }
        if (t->pend_volume_on) {
            e->lock_volume = 1;
            e->cur_volume = t->pend_volume;
            Engine_SetVolume(e, t->pend_volume);
            t->pend_volume_on = 0;
        }
        if (t->pend_pitch_on) {
            e->lock_pitch = 1;
            e->cur_pitch = t->pend_pitch;
            Engine_SetPitch(e, t->pend_pitch);
            t->pend_pitch_on = 0;
        }
        if (e->st_idle && e->st_input_empty && e->item_done) {
            EnterCriticalSection(&t->queue_lock);
            if (t->queue.count != 0) {
                size = t->queue.item[0].size;
                data = t->queue.item[0].data;
                pos = 0;
                e->item_done = 0;
                LeaveCriticalSection(&t->queue_lock);
                Engine_Feed(e, data, size, &pos);
                if (e->item_done) {
                    EnterCriticalSection(&t->queue_lock);
                    queue_remove(&t->queue, 0);
                    LeaveCriticalSection(&t->queue_lock);
                }
                Engine_Flush(e, 1);
                flushed = 1;
            } else if (!flushed) {
                /* the end: tts_Speak starts a new thread from here on */
                t->speaking = 0;
                LeaveCriticalSection(&t->queue_lock);
                while (e->out_state == 0) {
                    e->trk_34 = 1;
                    e->out_state = Stage4_Run(e);
                }
                if (e->textin != NULL)
                    tv_delete(e->textin);
                e->textin = NULL;
                tts_audio_done(t, 1);
                break;
            } else {
                LeaveCriticalSection(&t->queue_lock);
            }
        } else if (!e->item_done && Engine_InFree(e) > 0x800) {
            Engine_Feed(e, data, size, &pos);
            if (e->item_done) {
                EnterCriticalSection(&t->queue_lock);
                queue_remove(&t->queue, 0);
                LeaveCriticalSection(&t->queue_lock);
            }
        } else if (e->st_idle) {
            Engine_Flush(e, 0);
            flushed = 1;
        }
        r = tv_engine_step(e, &s2);
        if (e->out_count >= e->out_limit || ((r & 1) && e->out_count != 0)) {
            if (r & 1)
                Output_TrimTail(e);
            hdr->dwBufferLength = e->out_count;
            waveOutWrite(t->hwo, hdr, sizeof *hdr);
            ResetEvent(t->cur_done);
            if (first) {
                e->out_buf = t->buf[1];
                hdr = &t->hdr[1];
                first = 0;
                t->cur_done = t->buf_done[1];
            } else {
                e->out_buf = t->buf[0];
                hdr = &t->hdr[0];
                first = 1;
                t->cur_done = t->buf_done[0];
            }
            WaitForSingleObject(t->cur_done, INFINITE);
            e->out_count = 0;
        }
        if (r & 1) {
            flushed = 0;
            WaitForSingleObject(t->buf_done[0], INFINITE);
            WaitForSingleObject(t->buf_done[1], INFINITE);
        }
    }
    if (e->textin != NULL)
        tv_delete(e->textin);
    tv_engine_done(&s2);
    free(e);
    /* (the DLL closes its thread handle here; tts_Speak and tts_Close do
     * here, so that tts_Stop can always wait on it) */
    tts_release(t);
    return 0;
}

/* ---- the pronunciation manager ------------------------------------------ */

/* @0x10001d40 (port/stubs.c) */
void TV_THISCALL ByteList_Append(void *self, int32_t b);

/* Speaks text as the dictionary dialog's Word, Like and Phone buttons do,
 * on the caller's thread, and returns its phoneme spelling (malloc'd, or
 * NULL) in *out.  The device must be open (tts_audio_open); this ends the
 * utterance, without the callback.  The engine has its defaults (no
 * settings prefix), no TextIn, and Stage 2's byte list on as for
 * tts_Phoneme; the text goes in through Engine_PutText.  Audio goes out as
 * in the speaking thread.  @0x100014b0 */
int tts_speak_transcribe(TtsHandle *t, const char *text, char **out)
{
    Engine *e = (Engine *)calloc(1, sizeof *e);
    TvStage2Save s2 = {0};
    tv_bytelist list;
    WAVEHDR *hdr = &t->hdr[0];
    int first = 1;
    int32_t r;

    *out = NULL;
    if (e == NULL)
        return -1;
    memset(&list, 0, sizeof list);
    Engine_Construct(e);
    e->s2_bytes = &list;
    e->w_212e = 1;
    e->w_212c = 1;
    e->sample_rate = (uint16_t)t->s.sample_rate;
    e->out_format = (uint16_t)t->s.format;
    e->out_limit = tv_second_bytes(&t->s);
    Engine_Init(e);
    e->out_buf = t->buf[0];
    t->cur_done = t->buf_done[0];
    e->out_count = 0;
    Engine_PutText(e, text);
    do {
        r = tv_engine_step(e, &s2);
        if (e->out_count >= e->out_limit || ((r & 1) && e->out_count != 0)) {
            if (r & 1)
                Output_TrimTail(e);
            hdr->dwBufferLength = e->out_count;
            waveOutWrite(t->hwo, hdr, sizeof *hdr);
            ResetEvent(t->cur_done);
            if (first) {
                e->out_buf = t->buf[1];
                hdr = &t->hdr[1];
                first = 0;
                t->cur_done = t->buf_done[1];
            } else {
                e->out_buf = t->buf[0];
                hdr = &t->hdr[0];
                first = 1;
                t->cur_done = t->buf_done[0];
            }
            WaitForSingleObject(t->cur_done, INFINITE);
            e->out_count = 0;
        }
    } while (!(r & 1));
    ByteList_Append(&list, 0);
    tv_engine_done(&s2);
    free(e);
    if (list.failed)
        tv_bytelist_free(&list);
    else
        *out = list.buf;
    WaitForSingleObject(t->buf_done[0], 2000);
    WaitForSingleObject(t->buf_done[1], 2000);
    tts_audio_done(t, 0);
    return 0;
}

/* ---- the exports -------------------------------------------------------- */

/* Queues the text; if nothing is speaking, opens the device and starts the
 * thread with the caller's priority.  @0x1005f680 */
TTS_EXPORT tts_Speak(void *h, const char *text)
{
    TtsHandle *t = (TtsHandle *)h;
    char *full;
    HANDLE th;
    DWORD id;
    int r;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (t->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    full = tv_prepare_text(&t->s, text);
    if (full == NULL)
        return TTS_NOMEMORY;
    EnterCriticalSection(&t->queue_lock);
    if (!queue_push(&t->queue, full, (uint32_t)strlen(full) + 1)) {
        LeaveCriticalSection(&t->queue_lock);
        free(full);
        return TTS_NOMEMORY;
    }
    free(full);
    if (!t->speaking) {
        t->stop = 0;
        t->pend_pitch_on = 0;
        t->pend_volume_on = 0;
        t->pend_speed_on = 0;
        r = tts_audio_open(t);
        if (r != 0) {
            queue_remove(&t->queue, t->queue.count - 1);
            LeaveCriticalSection(&t->queue_lock);
            return (short)r;
        }
        InterlockedIncrement(&t->refs);
        th = CreateThread(NULL, 0x40000, speak_thread, t, CREATE_SUSPENDED, &id);
        if (th == NULL) {
            InterlockedDecrement(&t->refs);
            queue_remove(&t->queue, t->queue.count - 1);
            LeaveCriticalSection(&t->queue_lock);
            tts_audio_done(t, 0);
            return TTS_NOTHREAD;
        }
        if (t->thread != NULL)
            CloseHandle(t->thread);
        t->thread = th;
        t->speaking = 1;
        SetThreadPriority(th, GetThreadPriority(GetCurrentThread()));
        ResumeThread(th);
    }
    LeaveCriticalSection(&t->queue_lock);
    return 0;
}

/* With no wave device open these do nothing.  @0x100604e0 */
TTS_EXPORT tts_Pause(void *h)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (t->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    EnterCriticalSection(&t->audio_lock);
    if (t->hwo != NULL) {
        t->paused = 1;
        ResetEvent(t->pause_event);
        waveOutPause(t->hwo);
    }
    LeaveCriticalSection(&t->audio_lock);
    return 0;
}

/* @0x10060560 */
TTS_EXPORT tts_Resume(void *h)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (t->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    EnterCriticalSection(&t->audio_lock);
    if (t->hwo != NULL && t->paused) {
        t->paused = 0;
        SetEvent(t->pause_event);
        waveOutRestart(t->hwo);
    }
    LeaveCriticalSection(&t->audio_lock);
    return 0;
}

/* Pauses the device, tells the thread to stop and waits for it (up to 5 s),
 * then closes the device, calls the tts_CallBack function and empties the
 * queue.  With no wave device open it does nothing.  @0x10060340 */
TTS_EXPORT tts_Stop(void *h)
{
    TtsHandle *t = (TtsHandle *)h;

    if (tts_validate(h) < 0)
        return TTS_INVALIDHANDLE;
    if (t->device == DEVICE_FILE_ONLY)
        return TTS_NODEVICE;
    EnterCriticalSection(&t->audio_lock);
    if (t->hwo == NULL) {
        LeaveCriticalSection(&t->audio_lock);
        return 0;
    }
    waveOutPause(t->hwo);
    t->stop = 1;
    t->paused = 0;
    SetEvent(t->pause_event);
    SetEvent(t->buf_done[0]);
    SetEvent(t->buf_done[1]);
    LeaveCriticalSection(&t->audio_lock);
    if (t->speaking && WaitForSingleObject(t->thread, 5000) != WAIT_OBJECT_0)
        t->speaking = 0;
    EnterCriticalSection(&t->audio_lock);
    if (t->prepared) {
        waveOutReset(t->hwo);
        waveOutUnprepareHeader(t->hwo, &t->hdr[0], sizeof t->hdr[0]);
        waveOutUnprepareHeader(t->hwo, &t->hdr[1], sizeof t->hdr[1]);
        waveOutClose(t->hwo);
        t->hwo = NULL;
        t->prepared = 0;
        if (t->callback != NULL)
            t->callback(t->callback_user);
    }
    t->audio_users = 0;
    LeaveCriticalSection(&t->audio_lock);
    EnterCriticalSection(&t->queue_lock);
    tts_queue_clear(&t->queue);
    LeaveCriticalSection(&t->queue_lock);
    return 0;
}
