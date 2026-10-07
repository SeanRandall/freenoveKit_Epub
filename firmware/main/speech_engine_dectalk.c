#include "speech_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ttsapi.h"

#define DECTALK_PCM_SAMPLES 2048
#define DECTALK_INDEX_MARKS 32

struct speech_engine {
    speech_engine_config_t config;
    LPTTS_HANDLE_T handle;
    TTS_BUFFER_T output;
    int16_t pcm[DECTALK_PCM_SAMPLES];
    TTS_INDEX_T indices[DECTALK_INDEX_MARKS];
    uint64_t delivered_samples;
    int rate;
    int voice;
    uint32_t pending_marker;
    bool have_marker;
    bool busy;
    bool abort_requested;
};

static speech_engine_t *active_engine;

static void dectalk_queue_buffer(speech_engine_t *engine)
{
    engine->output.lpData = (LPSTR)engine->pcm;
    engine->output.dwMaximumBufferLength = sizeof(engine->pcm);
    engine->output.dwBufferLength = 0;
    engine->output.lpPhonemeArray = NULL;
    engine->output.dwMaximumNumberOfPhonemeChanges = 0;
    engine->output.lpIndexArray = engine->indices;
    engine->output.dwMaximumNumberOfIndexMarks = DECTALK_INDEX_MARKS;
    engine->output.dwNumberOfIndexMarks = 0;
    (void)TextToSpeechAddBuffer(engine->handle, &engine->output);
}

static bool dectalk_deliver(speech_engine_t *engine, TTS_BUFFER_T *buffer)
{
    size_t samples = buffer ? buffer->dwBufferLength / sizeof(int16_t) : 0;
    if (!samples) return true;
    size_t at = 0;
    uint64_t base = engine->delivered_samples;
    for (DWORD i = 0; i < buffer->dwNumberOfIndexMarks; ++i) {
        uint64_t absolute = buffer->lpIndexArray[i].dwIndexSampleNumber;
        size_t mark_at = absolute > base ? (size_t)(absolute - base) : 0;
        if (mark_at > samples) mark_at = samples;
        if (mark_at > at
            && !engine->config.pcm(engine->pcm + at, mark_at - at,
                                   engine->config.context)) {
            engine->abort_requested = true;
            break;
        }
        at = mark_at;
        if (engine->config.marker)
            engine->config.marker(buffer->lpIndexArray[i].dwIndexValue,
                                  engine->config.context);
    }
    if (!engine->abort_requested && at < samples
        && !engine->config.pcm(engine->pcm + at, samples - at,
                               engine->config.context))
        engine->abort_requested = true;
    engine->delivered_samples += samples;
    return !engine->abort_requested;
}

static void dectalk_callback(LONG first, LONG second, DWORD instance, UINT message)
{
    speech_engine_t *engine = (speech_engine_t *)(uintptr_t)instance;
    if (!engine) engine = active_engine;
    if (!engine) return;
    if (message == TTS_MSG_BUFFER) {
        TTS_BUFFER_T *buffer = (TTS_BUFFER_T *)(uintptr_t)(uint32_t)first;
        if (!buffer) (void)TextToSpeechReturnBuffer(engine->handle, &buffer);
        if (!engine->abort_requested)
            (void)dectalk_deliver(engine, buffer);
        /* DTC-01's manual-clock reset can leave its emulated kernel and API
           mutexes contending when invoked from a buffer callback.  Keep
           returning empty buffers after cancellation so the utterance can
           unwind normally; its remaining PCM is deliberately discarded. */
        dectalk_queue_buffer(engine);
    } else if (message == TTS_MSG_INDEX_MARK && engine->config.marker) {
        engine->config.marker((uint32_t)second, engine->config.context);
    }
}

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 120, 350, 5, 180, false },
    { SPEECH_SETTING_VOICE, "voice", 1, 9, 1, 1, false },
};

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine || !config || !config->pcm) { free(engine); return NULL; }
    engine->config = *config;
    engine->rate = 180;
    engine->voice = 1;
    active_engine = engine;
    DWORD options = DO_NOT_USE_AUDIO_DEVICE | TTS_MANUAL_CLOCK;
    if (TextToSpeechStartupEx(&engine->handle, WAVE_MAPPER, options,
                              dectalk_callback,
                              (LONG)(uintptr_t)engine) != MMSYSERR_NOERROR
        || TextToSpeechOpenInMemory(engine->handle, WAVE_FORMAT_1M16)
               != MMSYSERR_NOERROR) {
        if (engine->handle) TextToSpeechShutdown(engine->handle);
        active_engine = NULL;
        free(engine);
        return NULL;
    }
    dectalk_queue_buffer(engine);
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    (void)TextToSpeechReset(engine->handle, FALSE);
    (void)TextToSpeechCloseInMemory(engine->handle);
    (void)TextToSpeechShutdown(engine->handle);
    if (active_engine == engine) active_engine = NULL;
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{ (void)engine; return "DECtalk DTC-01"; }
const char *speech_engine_storage_id(void) { return "dectalk-dtc01"; }
unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{ (void)engine; return 10000; }

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text) return false;
    char *input = NULL;
    if (engine->have_marker) {
        size_t needed = strlen(text) + 40;
        input = malloc(needed);
        if (!input) return false;
        snprintf(input, needed, "[:in %lu]%s", (unsigned long)engine->pending_marker,
                 text);
        engine->have_marker = false;
    }
    engine->busy = true;
    engine->abort_requested = false;
    char *writable = input ? input : strdup(text);
    if (!writable
        || TextToSpeechSpeak(engine->handle, writable, TTS_FORCE)
               != MMSYSERR_NOERROR) {
        free(writable);
        engine->busy = false;
        return false;
    }
    free(writable);
    for (;;) {
        DWORD id = STATUS_SPEAKING, speaking = 0;
        (void)TextToSpeechRun(engine->handle, DECTALK_PCM_SAMPLES);
        (void)TextToSpeechGetStatus(engine->handle, &id, &speaking, 1);
        if (!speaking) break;
        /* Manual-clock synthesis otherwise owns this core continuously and
           can trip the task watchdog on a long utterance. */
        vTaskDelay(1);
    }
    TTS_BUFFER_T *partial = NULL;
    (void)TextToSpeechReturnBuffer(engine->handle, &partial);
    if (partial && !engine->abort_requested)
        (void)dectalk_deliver(engine, partial);
    dectalk_queue_buffer(engine);
    engine->busy = false;
    return !engine->abort_requested;
}

bool speech_engine_insert_marker(speech_engine_t *engine, uint32_t marker)
{
    if (!engine) return false;
    engine->pending_marker = marker & 0x7fff;
    engine->have_marker = true;
    return true;
}
bool speech_engine_busy(speech_engine_t *engine)
{ return engine && engine->busy; }
void speech_engine_wait(speech_engine_t *engine) { (void)engine; }

size_t speech_engine_setting_count(const speech_engine_t *engine)
{ (void)engine; return sizeof(settings) / sizeof(settings[0]); }
bool speech_engine_setting_info(const speech_engine_t *engine, size_t index,
                                speech_setting_info_t *info)
{
    (void)engine;
    if (!info || index >= speech_engine_setting_count(engine)) return false;
    *info = settings[index];
    return true;
}
bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t setting,
                       int value)
{
    if (!engine) return false;
    if (setting == SPEECH_SETTING_RATE && value >= 120 && value <= 350) {
        engine->rate = value;
        return TextToSpeechSetRate(engine->handle, (DWORD)value) == MMSYSERR_NOERROR;
    }
    if (setting == SPEECH_SETTING_VOICE && value >= 1 && value <= 9) {
        engine->voice = value;
        return TextToSpeechSetSpeaker(engine->handle, (SPEAKER_T)(value - 1))
               == MMSYSERR_NOERROR;
    }
    if (setting == SPEECH_SETTING_ANNOTATED_INPUT) return true;
    return false;
}
bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t setting,
                       int *value)
{
    if (!engine || !value) return false;
    if (setting == SPEECH_SETTING_RATE) { *value = engine->rate; return true; }
    if (setting == SPEECH_SETTING_VOICE) { *value = engine->voice; return true; }
    if (setting == SPEECH_SETTING_ANNOTATED_INPUT) { *value = 1; return true; }
    return false;
}
bool speech_engine_supports_annotations(const speech_engine_t *engine)
{ (void)engine; return true; }
