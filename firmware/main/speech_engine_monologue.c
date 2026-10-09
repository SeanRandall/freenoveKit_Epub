#include "speech_engine.h"

#include <stdlib.h>
#include <string.h>

#include "fbv.h"
#include "monologue_port.h"

#define MONOLOGUE_PCM_BYTES 4096

struct speech_engine {
    speech_engine_config_t config;
    monologue_voice_t *voice;
    BYTE *scb;
    int rate;
    int pitch;
    int volume;
    int sentence_pause;
    int dictionary;
    bool busy;
    bool abort;
    bool marker_pending;
    uint32_t pending_marker;
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 60, 140, 5, 100, false,
      NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 100, false,
      NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 0, 100, 5, 50, false, NULL, NULL },
    { SPEECH_SETTING_SENTENCE_PAUSE, "sentence pause", 0, 1000, 50, 0,
      false, NULL, NULL },
    { SPEECH_SETTING_NATIVE_DICTIONARY, "native dictionary", 0, 1, 1, 1,
      true, "off", "on" },
};

static int native_rate(int rate)
{
    int value = (rate - 80) / 4;
    if (value < -5) value = -5;
    if (value > 15) value = 15;
    return value;
}

static int native_pitch(int pitch)
{
    int value = pitch / 5 - 5;
    if (value < -5) value = -5;
    if (value > 15) value = 15;
    return value;
}

static void apply_settings(speech_engine_t *engine)
{
    (void)SetSpeechParameter(engine->scb, 2, native_pitch(engine->pitch));
    (void)SetSpeechParameter(engine->scb, 3, native_rate(engine->rate));
    (void)SetSpeechParameter(engine->scb, 22, engine->sentence_pause);
    SCBF(engine->scb, int, 0x274) = engine->dictionary;
    printf("MONOLOGUE_SETTINGS rate=%d native_rate=%d pitch=%d "
           "native_pitch=%d volume=%d sentence_pause=%d dictionary=%d\n",
           engine->rate, native_rate(engine->rate), engine->pitch,
           native_pitch(engine->pitch), engine->volume,
           engine->sentence_pause, engine->dictionary);
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return NULL;
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    engine->config = *config;
    engine->rate = 100;
    engine->pitch = 50;
    engine->volume = 100;
    engine->sentence_pause = 0;
    engine->dictionary = 1;
    engine->voice = monologue_voice_open();
    if (!engine->voice) {
        free(engine);
        return NULL;
    }
    engine->scb = monologue_voice_scb(engine->voice);
    apply_settings(engine);
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    monologue_voice_close(engine->voice);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "Monologue" : "";
}

const char *speech_engine_storage_id(void) { return "monologue-xen11k8"; }

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? monologue_voice_sample_rate(engine->voice) : 0;
}

static bool emit_pcm(speech_engine_t *engine, const BYTE *bytes, size_t count)
{
    int16_t samples[MONOLOGUE_PCM_BYTES];
    unsigned bits = monologue_voice_bits(engine->voice);
    size_t n;
    if (bits == 8) {
        n = count;
        for (size_t i = 0; i < n; i++) {
            int32_t sample = ((int32_t)bytes[i] - 128) << 8;
            samples[i] = (int16_t)(sample * engine->volume / 100);
        }
    } else if (bits == 16) {
        n = count / 2;
        for (size_t i = 0; i < n; i++) {
            int16_t input = (int16_t)(bytes[i * 2] | bytes[i * 2 + 1] << 8);
            samples[i] = (int16_t)((int32_t)input * engine->volume / 100);
        }
    } else {
        return false;
    }
    return engine->config.pcm(samples, n, engine->config.context);
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !engine->scb || !text || !*text) return false;
    size_t length = strlen(text);
    if (length >= 8192) return false;
    char *writable = malloc(length + 1);
    if (!writable) return false;
    memcpy(writable, text, length + 1);
    engine->busy = true;
    engine->abort = false;
    char *phonetics = TextToPhonetics(engine->scb, writable, 0);
    free(writable);
    if (!phonetics) {
        engine->busy = false;
        return false;
    }
    LONG result = OpenBackend(engine->scb, phonetics);
    bool ok = result == 0;
    if (ok && engine->marker_pending && engine->config.marker) {
        engine->config.marker(engine->pending_marker, engine->config.context);
        engine->marker_pending = false;
    }
    BYTE pcm[MONOLOGUE_PCM_BYTES];
    while (ok && !engine->abort
           && (result = GetPCMdata(engine->scb, pcm, sizeof(pcm))) > 0) {
        ok = emit_pcm(engine, pcm, (size_t)result);
    }
    if (SCBF(engine->scb, int, 0x3b0)) CloseBackend(engine->scb);
    mem_free(phonetics);
    engine->busy = false;
    return ok && !engine->abort && result >= 0;
}

bool speech_engine_insert_marker(speech_engine_t *engine, uint32_t marker)
{
    if (!engine) return false;
    engine->pending_marker = marker;
    engine->marker_pending = true;
    return true;
}

bool speech_engine_busy(speech_engine_t *engine)
{
    return engine && engine->busy;
}

void speech_engine_wait(speech_engine_t *engine) { (void)engine; }

size_t speech_engine_setting_count(const speech_engine_t *engine)
{
    (void)engine;
    return sizeof(settings) / sizeof(settings[0]);
}

bool speech_engine_setting_info(const speech_engine_t *engine, size_t index,
                                speech_setting_info_t *info)
{
    (void)engine;
    if (!info || index >= sizeof(settings) / sizeof(settings[0])) return false;
    *info = settings[index];
    return true;
}

bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t setting,
                       int value)
{
    if (!engine) return false;
    switch (setting) {
    case SPEECH_SETTING_RATE: engine->rate = value; break;
    case SPEECH_SETTING_ENGINE_VOLUME: engine->volume = value; break;
    case SPEECH_SETTING_PITCH: engine->pitch = value; break;
    case SPEECH_SETTING_SENTENCE_PAUSE: engine->sentence_pause = value; break;
    case SPEECH_SETTING_NATIVE_DICTIONARY: engine->dictionary = !!value; break;
    case SPEECH_SETTING_ANNOTATED_INPUT: return true;
    default: return false;
    }
    apply_settings(engine);
    return true;
}

bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t setting,
                       int *value)
{
    if (!engine || !value) return false;
    switch (setting) {
    case SPEECH_SETTING_RATE: *value = engine->rate; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: *value = engine->volume; return true;
    case SPEECH_SETTING_PITCH: *value = engine->pitch; return true;
    case SPEECH_SETTING_SENTENCE_PAUSE: *value = engine->sentence_pause; return true;
    case SPEECH_SETTING_NATIVE_DICTIONARY: *value = engine->dictionary; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 0; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    return false;
}
