#include "speech_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "picoapi.h"
#include "picodefs.h"

#define PICO_MEMORY_BYTES 2500000u
#define PICO_OUTPUT_BYTES 4096
#define PICO_SAMPLE_RATE 16000
#ifndef PICO_VOICE_TAG
#define PICO_VOICE_TAG "en-US"
#define PICO_TA_FILENAME "en-US_ta.bin"
#define PICO_SG_FILENAME "en-US_lh0_sg.bin"
#define PICO_TA_SYMBOL _binary_en_US_ta_bin
#define PICO_SG_SYMBOL _binary_en_US_lh0_sg_bin
#endif
#define PICO_RESOURCE_DIR "/sdcard/.evv/pico"
#define PICO_TA_PATH PICO_RESOURCE_DIR "/" PICO_TA_FILENAME
#define PICO_SG_PATH PICO_RESOURCE_DIR "/" PICO_SG_FILENAME
#define PICO_VOICE_NAME "EVVPico"

#define STRINGIFY_INNER(value) #value
#define STRINGIFY(value) STRINGIFY_INNER(value)
extern const unsigned char pico_ta_start[] asm(STRINGIFY(PICO_TA_SYMBOL) "_start");
extern const unsigned char pico_ta_end[] asm(STRINGIFY(PICO_TA_SYMBOL) "_end");
extern const unsigned char pico_sg_start[] asm(STRINGIFY(PICO_SG_SYMBOL) "_start");
extern const unsigned char pico_sg_end[] asm(STRINGIFY(PICO_SG_SYMBOL) "_end");

struct speech_engine {
    speech_engine_config_t config;
    void *memory;
    pico_System system;
    pico_Resource ta_resource;
    pico_Resource sg_resource;
    pico_Engine instance;
    int rate;
    int pitch;
    int volume;
    bool busy;
    bool abort;
    bool marker_pending;
    uint32_t pending_marker;
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 20, 500, 5, 100, false,
      NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 50, 200, 5, 100, false,
      NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 100, false,
      NULL, NULL },
};

static bool install_resource(const char *path, const unsigned char *start,
                             const unsigned char *end)
{
    struct stat st;
    size_t expected = (size_t)(end - start);
    if (stat(path, &st) == 0 && (size_t)st.st_size == expected)
        return true;
    (void)mkdir("/sdcard/.evv", 0775);
    (void)mkdir(PICO_RESOURCE_DIR, 0775);
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fwrite(start, 1, expected, file) == expected;
    ok = fclose(file) == 0 && ok;
    return ok;
}

static bool add_resource(speech_engine_t *engine, const char *path,
                         pico_Resource *resource)
{
    pico_Retstring name;
    return pico_loadResource(engine->system, (const pico_Char *)path,
                             resource) == PICO_OK
        && pico_getResourceName(engine->system, *resource, name) == PICO_OK
        && pico_addResourceToVoiceDefinition(
            engine->system, (const pico_Char *)PICO_VOICE_NAME,
            (const pico_Char *)name) == PICO_OK;
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return NULL;
    if (!install_resource(PICO_TA_PATH, pico_ta_start, pico_ta_end)
        || !install_resource(PICO_SG_PATH, pico_sg_start, pico_sg_end))
        return NULL;

    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    engine->config = *config;
    engine->rate = 100;
    engine->pitch = 100;
    engine->volume = 100;
    engine->memory = heap_caps_malloc(PICO_MEMORY_BYTES,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!engine->memory
        || pico_initialize(engine->memory, PICO_MEMORY_BYTES,
                           &engine->system) != PICO_OK
        || pico_createVoiceDefinition(
            engine->system, (const pico_Char *)PICO_VOICE_NAME) != PICO_OK
        || !add_resource(engine, PICO_TA_PATH, &engine->ta_resource)
        || !add_resource(engine, PICO_SG_PATH, &engine->sg_resource)
        || pico_newEngine(engine->system,
                          (const pico_Char *)PICO_VOICE_NAME,
                          &engine->instance) != PICO_OK) {
        speech_engine_destroy(engine);
        return NULL;
    }
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    if (engine->instance)
        (void)pico_disposeEngine(engine->system, &engine->instance);
    if (engine->system)
        (void)pico_terminate(&engine->system);
    free(engine->memory);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "Pico" : "";
}

const char *speech_engine_storage_id(void) { return "pico"; }

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? PICO_SAMPLE_RATE : 0;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !engine->instance || !text) return false;
    size_t wrapped_size = strlen(text) + 128;
    char *wrapped = malloc(wrapped_size);
    if (!wrapped) return false;
    snprintf(wrapped, wrapped_size,
             "<pitch level='%d'><speed level='%d'><volume level='%d'>%s"
             "</volume></speed></pitch>",
             engine->pitch, engine->rate, engine->volume, text);

    engine->busy = true;
    engine->abort = false;
    const pico_Char *at = (const pico_Char *)wrapped;
    size_t remaining = strlen(wrapped) + 1;
    int16_t output[PICO_OUTPUT_BYTES / sizeof(int16_t)];
    bool ok = true;
    unsigned work_slices = 0;
    while (remaining && ok && !engine->abort) {
        pico_Int16 request = remaining > 32767 ? 32767 : (pico_Int16)remaining;
        pico_Int16 accepted = 0;
        pico_Status status = pico_putTextUtf8(engine->instance, at, request,
                                               &accepted);
        if (status != PICO_OK || accepted <= 0) {
            ok = false;
            break;
        }
        at += accepted;
        remaining -= (size_t)accepted;
        do {
            pico_Int16 received = 0;
            pico_Int16 data_type = 0;
            status = pico_getData(engine->instance, output,
                                  sizeof(output), &received, &data_type);
            /* Pico is synchronous and can otherwise keep this core runnable
               for several seconds.  Briefly unblock the idle task without
               materially reducing synthesis throughput. */
            if ((++work_slices & 31u) == 0)
                vTaskDelay(1);
            if (status != PICO_STEP_BUSY && status != PICO_STEP_IDLE) {
                ok = false;
                break;
            }
            if (received > 0) {
                if (engine->marker_pending && engine->config.marker) {
                    engine->config.marker(engine->pending_marker,
                                          engine->config.context);
                    engine->marker_pending = false;
                }
                if (!engine->config.pcm(output,
                        (size_t)received / sizeof(int16_t),
                        engine->config.context)) {
                    engine->abort = true;
                    ok = false;
                    break;
                }
            }
        } while (status == PICO_STEP_BUSY);
    }
    free(wrapped);
    engine->busy = false;
    return ok;
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

void speech_engine_wait(speech_engine_t *engine)
{
    (void)engine;
}

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
    case SPEECH_SETTING_RATE: engine->rate = value; return true;
    case SPEECH_SETTING_PITCH: engine->pitch = value; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: engine->volume = value; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: return true;
    default: return false;
    }
}

bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t setting,
                       int *value)
{
    if (!engine || !value) return false;
    switch (setting) {
    case SPEECH_SETTING_RATE: *value = engine->rate; return true;
    case SPEECH_SETTING_PITCH: *value = engine->pitch; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: *value = engine->volume; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 0; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    return false;
}
