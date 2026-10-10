#include "speech_engine.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ttsapi.h"

#define DT43_PCM_SAMPLES 2048
#define DT43_INDEX_MARKS 32

struct speech_engine {
    speech_engine_config_t config;
    LPTTS_HANDLE_T handle;
    TTS_BUFFER_T output;
    int16_t pcm[DT43_PCM_SAMPLES];
    TTS_INDEX_T indices[DT43_INDEX_MARKS];
    uint64_t delivered_samples;
    int rate;
    int voice;
    uint32_t pending_marker;
    bool have_marker;
    volatile bool busy;
    volatile bool abort_requested;
};

static speech_engine_t *active_engine;

extern const uint8_t dt43_dictionary_start[]
    asm("_binary_dtalk_us_dic_start");
extern const uint8_t dt43_dictionary_end[]
    asm("_binary_dtalk_us_dic_end");

static bool install_dictionary(void)
{
    static const char *path = "/sdcard/.evv/voices/dectalk43/dtalk_us.dic";
    size_t expected = (size_t)(dt43_dictionary_end - dt43_dictionary_start);
    struct stat current;
    if (stat(path, &current) == 0 && (size_t)current.st_size == expected)
        return true;

    (void)mkdir("/sdcard/.evv", 0775);
    (void)mkdir("/sdcard/.evv/voices", 0775);
    (void)mkdir("/sdcard/.evv/voices/dectalk43", 0775);
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fwrite(dt43_dictionary_start, 1, expected, file) == expected;
    if (fclose(file) != 0) ok = false;
    return ok;
}

static void queue_buffer(speech_engine_t *engine)
{
    engine->output.lpData = (LPSTR)engine->pcm;
    engine->output.dwMaximumBufferLength = sizeof(engine->pcm);
    engine->output.dwBufferLength = 0;
    engine->output.lpPhonemeArray = NULL;
    engine->output.dwMaximumNumberOfPhonemeChanges = 0;
    engine->output.lpIndexArray = engine->indices;
    engine->output.dwMaximumNumberOfIndexMarks = DT43_INDEX_MARKS;
    engine->output.dwNumberOfIndexMarks = 0;
    (void)TextToSpeechAddBuffer(engine->handle, &engine->output);
}

static bool deliver(speech_engine_t *engine, TTS_BUFFER_T *buffer)
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

static void dt43_callback(LONG first, LONG second, DWORD instance, UINT message)
{
    speech_engine_t *engine = (speech_engine_t *)(uintptr_t)instance;
    if (!engine) engine = active_engine;
    if (!engine) return;
    if (message == TTS_MSG_BUFFER) {
        TTS_BUFFER_T *buffer = (TTS_BUFFER_T *)(uintptr_t)(uint32_t)first;
        if (!buffer) (void)TextToSpeechReturnBuffer(engine->handle, &buffer);
        if (!engine->abort_requested) (void)deliver(engine, buffer);
        queue_buffer(engine);
    } else if (message == TTS_MSG_INDEX_MARK && engine->config.marker) {
        engine->config.marker((uint32_t)second, engine->config.context);
    }
}

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 75, 600, 5, 180, false },
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
    if (!install_dictionary()) {
        active_engine = NULL;
        free(engine);
        return NULL;
    }
    if (TextToSpeechStartupEx(&engine->handle, WAVE_MAPPER,
                              DO_NOT_USE_AUDIO_DEVICE, dt43_callback,
                              (LONG)(uintptr_t)engine) != MMSYSERR_NOERROR
        || TextToSpeechOpenInMemory(engine->handle, WAVE_FORMAT_1M16)
               != MMSYSERR_NOERROR) {
        if (engine->handle) TextToSpeechShutdown(engine->handle);
        active_engine = NULL;
        free(engine);
        return NULL;
    }
    queue_buffer(engine);
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    (void)TextToSpeechReset(engine->handle, false);
    (void)TextToSpeechCloseInMemory(engine->handle);
    (void)TextToSpeechShutdown(engine->handle);
    if (active_engine == engine) active_engine = NULL;
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{ (void)engine; return "DECtalk 4.3"; }
const char *speech_engine_storage_id(void) { return "dectalk-43-private"; }
unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{ (void)engine; return 11025; }

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text) return false;
    size_t extra = engine->have_marker ? 40 : 1;
    char *input = malloc(strlen(text) + extra);
    if (!input) return false;
    if (engine->have_marker) {
        snprintf(input, strlen(text) + extra, "[:in %lu]%s",
                 (unsigned long)engine->pending_marker, text);
        engine->have_marker = false;
    } else {
        strcpy(input, text);
    }
    engine->busy = true;
    engine->abort_requested = false;
    engine->delivered_samples = 0;
    if (TextToSpeechSpeak(engine->handle, input, TTS_FORCE)
            != MMSYSERR_NOERROR) {
        free(input);
        engine->busy = false;
        return false;
    }
    free(input);
    for (;;) {
        DWORD ids[1] = { STATUS_SPEAKING };
        DWORD values[1] = { 0 };
        (void)TextToSpeechGetStatus(engine->handle, ids, values, 1);
        if (!values[0]) break;
        if (engine->abort_requested)
            (void)TextToSpeechReset(engine->handle, false);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    TTS_BUFFER_T *partial = NULL;
    (void)TextToSpeechReturnBuffer(engine->handle, &partial);
    if (partial && !engine->abort_requested) (void)deliver(engine, partial);
    queue_buffer(engine);
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
bool speech_engine_busy(speech_engine_t *engine) { return engine && engine->busy; }
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
    if (setting == SPEECH_SETTING_RATE && value >= 75 && value <= 600) {
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
