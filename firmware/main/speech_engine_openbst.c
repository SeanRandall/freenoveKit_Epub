#include "speech_engine.h"

#include <stdlib.h>
#include <string.h>

#include "bst.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef OPENBST_BUILD_NAME
#define OPENBST_BUILD_NAME "1998ENG"
#endif
#ifndef EVV_ENGINE_SAMPLE_RATE
#define EVV_ENGINE_SAMPLE_RATE 10000
#endif
#define OPENBST_BLOCK_SAMPLES 2048

struct speech_engine {
    speech_engine_config_t config;
    bst *instance;
    int rate;
    int pitch;
    int top;
    int level;
    int voice;
    bool busy;
    bool abort;
    bool marker_pending;
    uint32_t pending_marker;
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 60, 140, 5, 100, false,
      NULL, NULL },
    { SPEECH_SETTING_VOICE, "voice", 1, 6, 1, 1, false, NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 40, 120, 5, 80, false, NULL, NULL },
    { SPEECH_SETTING_HEAD_SIZE, "pitch ceiling", 80, 240, 5, 160, false,
      NULL, NULL },
    /* Present the native -3..10 contour index as a straightforward 0..13
       reader scale. OpenBST does not reliably speak a leading minus sign,
       which made the old values appear to move in the wrong direction. */
    { SPEECH_SETTING_PITCH_FLUCTUATION, "intonation level", 0, 13, 1, 6,
      false, NULL, NULL },
};

static void apply_settings(speech_engine_t *engine)
{
    /* OpenBST's duration adjustment is inverted relative to the reader UI:
       negative values speak faster and positive values speak slower. */
    /* The native extremes collapse or double segment durations and the 1998
       voice becomes unstable near -100. Keep the reader's useful 60..140
       scale, but map it to the safe native +40..-40 interval. */
    int native_rate = 100 - engine->rate;
    if (native_rate < -100) native_rate = -100;
    if (native_rate > 100) native_rate = 100;
    (void)bst_set(engine->instance, "rate", native_rate);
    (void)bst_set(engine->instance, "pitch", engine->pitch);
    (void)bst_set(engine->instance, "top", engine->top);
    (void)bst_set(engine->instance, "level", engine->level - 3);
    (void)bst_set(engine->instance, "voice", engine->voice - 1);
    printf("OPENBST_SETTINGS build=%s ui_rate=%d native_rate=%d voice=%d "
           "pitch=%d top=%d ui_level=%d native_level=%d\n",
           OPENBST_BUILD_NAME, engine->rate, native_rate, engine->voice,
           engine->pitch, engine->top, engine->level, engine->level - 3);
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return NULL;
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    engine->config = *config;
    engine->rate = 100;
    engine->pitch = 80;
    engine->top = 160;
    engine->level = 6;
    engine->voice = 1;
    engine->instance = bst_open(OPENBST_BUILD_NAME);
    if (!engine->instance) {
        free(engine);
        return NULL;
    }
    apply_settings(engine);
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    bst_close(engine->instance);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "Open B S T" : "";
}

const char *speech_engine_storage_id(void)
{
    /* Keep settings for incompatible voice builds separate. */
    return "bst-" OPENBST_BUILD_NAME;
}

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? EVV_ENGINE_SAMPLE_RATE : 0;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !engine->instance || !text) return false;
    engine->busy = true;
    engine->abort = false;
    long wanted = bst_length(engine->instance, text);
    if (wanted < 0 || (size_t)wanted > SIZE_MAX / sizeof(int16_t)) {
        engine->busy = false;
        return false;
    }
    size_t allocation = (size_t)(wanted ? wanted : 1) * sizeof(int16_t);
    int16_t *pcm = heap_caps_malloc(allocation,
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcm) {
        engine->busy = false;
        return false;
    }
    long generated = bst_say(engine->instance, text, pcm, wanted);
    bool ok = generated >= 0;
    if (ok && generated > 0 && engine->marker_pending && engine->config.marker) {
        engine->config.marker(engine->pending_marker, engine->config.context);
        engine->marker_pending = false;
    }
    for (long at = 0; ok && at < generated && !engine->abort;
         at += OPENBST_BLOCK_SAMPLES) {
        size_t count = (size_t)(generated - at);
        if (count > OPENBST_BLOCK_SAMPLES) count = OPENBST_BLOCK_SAMPLES;
        ok = engine->config.pcm(pcm + at, count, engine->config.context);
        if (((unsigned long)at / OPENBST_BLOCK_SAMPLES & 31u) == 31u)
            vTaskDelay(1);
    }
    heap_caps_free(pcm);
    engine->busy = false;
    return ok && !engine->abort;
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
    case SPEECH_SETTING_VOICE: engine->voice = value; break;
    case SPEECH_SETTING_PITCH: engine->pitch = value; break;
    case SPEECH_SETTING_HEAD_SIZE: engine->top = value; break;
    case SPEECH_SETTING_PITCH_FLUCTUATION: engine->level = value; break;
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
    case SPEECH_SETTING_VOICE: *value = engine->voice; return true;
    case SPEECH_SETTING_PITCH: *value = engine->pitch; return true;
    case SPEECH_SETTING_HEAD_SIZE: *value = engine->top; return true;
    case SPEECH_SETTING_PITCH_FLUCTUATION: *value = engine->level; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 0; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    return false;
}
