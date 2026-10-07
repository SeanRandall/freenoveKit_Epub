#ifndef EVV_SPEECH_ENGINE_H
#define EVV_SPEECH_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct speech_engine speech_engine_t;

typedef enum {
    SPEECH_SETTING_GENDER,
    SPEECH_SETTING_HEAD_SIZE,
    SPEECH_SETTING_PITCH,
    SPEECH_SETTING_PITCH_FLUCTUATION,
    SPEECH_SETTING_ROUGHNESS,
    SPEECH_SETTING_BREATHINESS,
    SPEECH_SETTING_RATE,
    SPEECH_SETTING_ENGINE_VOLUME,
    SPEECH_SETTING_NATIVE_DICTIONARY,
    SPEECH_SETTING_PHRASE_PREDICTION,
    SPEECH_SETTING_VOICE,
    SPEECH_SETTING_ANNOTATED_INPUT,
    SPEECH_SETTING_COUNT,
} speech_setting_id_t;

typedef struct {
    speech_setting_id_t id;
    const char *name;
    int minimum;
    int maximum;
    int step;
    int default_value;
    bool boolean_value;
    const char *false_label;
    const char *true_label;
} speech_setting_info_t;

/* Return false to abort the current utterance. PCM is signed mono 16-bit at
   speech_engine_sample_rate(). */
typedef bool (*speech_pcm_callback_t)(const int16_t *pcm, size_t samples,
                                      void *context);
typedef void (*speech_marker_callback_t)(uint32_t marker, void *context);

typedef struct {
    speech_pcm_callback_t pcm;
    speech_marker_callback_t marker;
    void *context;
} speech_engine_config_t;

speech_engine_t *speech_engine_create(const speech_engine_config_t *config);
void speech_engine_destroy(speech_engine_t *engine);

const char *speech_engine_name(const speech_engine_t *engine);
/* Short, stable identifier used to keep persisted settings separate between
   firmware variants. */
const char *speech_engine_storage_id(void);
unsigned speech_engine_sample_rate(const speech_engine_t *engine);

bool speech_engine_synthesize(speech_engine_t *engine, const char *text);
bool speech_engine_insert_marker(speech_engine_t *engine, uint32_t marker);
bool speech_engine_busy(speech_engine_t *engine);
void speech_engine_wait(speech_engine_t *engine);

size_t speech_engine_setting_count(const speech_engine_t *engine);
bool speech_engine_setting_info(const speech_engine_t *engine, size_t index,
                                speech_setting_info_t *info);
bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t setting,
                       int value);
bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t setting,
                       int *value);
bool speech_engine_supports_annotations(const speech_engine_t *engine);

#endif
