#include "speech_engine.h"

#include <stdlib.h>

#include "evv_abi.h"
#include "eci_old.h"

enum ECIMessage { eciWaveformBuffer, eciPhonemeBuffer, eciIndexReply };
enum ECICallbackReturn { eciDataNotProcessed, eciDataProcessed, eciDataAbort };

OldInst *STDCALL eo_new(void);
int STDCALL es_delete(OldInst *h);
int STDCALL et_addText(OldInst *h, const char *text);
int STDCALL et_insertIndex(OldInst *h, int32_t n);
int STDCALL et_synthesize(OldInst *h);
int STDCALL ev_setOutputBuffer(OldInst *h, int32_t n, void *buf);
int STDCALL ev_setParam(OldInst *h, int32_t which, int32_t value);
int STDCALL eo_getParam(OldInst *h, int32_t which);
int STDCALL vc_setVoiceParam(OldInst *h, int32_t voice, int32_t which,
                             int32_t value);
int STDCALL vc_getVoiceParam(OldInst *h, int32_t voice, int32_t which);
int STDCALL vc_copyVoice(OldInst *h, int32_t from, int32_t to);
int32_t STDCALL api_set_param(void *h2, int32_t kind, int32_t param,
                              int32_t value);
void STDCALL eo_registerCallback(OldInst *h, void *cb, void *data);
void STDCALL eo_synchronizeSynth(OldInst *h);
int STDCALL eo_speaking(OldInst *h);
void evvRunStaticInitialisers(void);
void evv_port_start(void);

#define OPENEVV_FRAME_SAMPLES 2048
#define OPENEVV_SAMPLE_RATE 11025
#define PARAM_INPUT_TYPE 1
#define VOICE_GENDER 0
#define VOICE_HEAD_SIZE 1
#define VOICE_PITCH 2
#define VOICE_PITCH_FLUCTUATION 3
#define VOICE_ROUGHNESS 4
#define VOICE_BREATHINESS 5
#define VOICE_SPEED 6
#define VOICE_VOLUME 7
#define ENV_DICTIONARY 3
#define ENV_PHRASE_PREDICTION 11
#define PARAM_PHRASE_PREDICTION 13

struct speech_engine {
    OldInst *instance;
    speech_engine_config_t config;
    int16_t pcm[OPENEVV_FRAME_SAMPLES];
    int rate;
    int volume;
    int native_dictionary;
    int voice;
    int phrase_prediction;
    int annotated_input;
    int voice_params[8];
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_VOICE, "voice", 1, 8, 1, 1, false },
    { SPEECH_SETTING_GENDER, "gender", 0, 1, 1, 0, true, "male", "female" },
    { SPEECH_SETTING_HEAD_SIZE, "head size", 0, 100, 5, 50, false },
    { SPEECH_SETTING_PITCH, "pitch baseline", 0, 100, 5, 50, false },
    { SPEECH_SETTING_PITCH_FLUCTUATION, "pitch fluctuation", 0, 100, 5, 50,
      false },
    { SPEECH_SETTING_ROUGHNESS, "roughness", 0, 100, 5, 50, false },
    { SPEECH_SETTING_BREATHINESS, "breathiness", 0, 100, 5, 50, false },
    { SPEECH_SETTING_RATE, "speaking rate", 60, 140, 5, 80, false },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 80, 5, 80, false },
    { SPEECH_SETTING_NATIVE_DICTIONARY, "Open E V V dictionary", 0, 1, 1, 1,
      true, "off", "on" },
    { SPEECH_SETTING_PHRASE_PREDICTION, "phrase prediction", 0, 1, 1, 0,
      true },
};

static int voice_parameter(speech_setting_id_t setting)
{
    switch (setting) {
    case SPEECH_SETTING_GENDER: return VOICE_GENDER;
    case SPEECH_SETTING_HEAD_SIZE: return VOICE_HEAD_SIZE;
    case SPEECH_SETTING_PITCH: return VOICE_PITCH;
    case SPEECH_SETTING_PITCH_FLUCTUATION: return VOICE_PITCH_FLUCTUATION;
    case SPEECH_SETTING_ROUGHNESS: return VOICE_ROUGHNESS;
    case SPEECH_SETTING_BREATHINESS: return VOICE_BREATHINESS;
    case SPEECH_SETTING_RATE: return VOICE_SPEED;
    case SPEECH_SETTING_ENGINE_VOLUME: return VOICE_VOLUME;
    default: return -1;
    }
}

static enum ECICallbackReturn STDCALL openevv_callback(
    OldInst *instance, enum ECIMessage message, long parameter, void *context)
{
    (void)instance;
    speech_engine_t *engine = context;
    if (message == eciIndexReply) {
        if (engine->config.marker)
            engine->config.marker((uint32_t)parameter,
                                  engine->config.context);
        return eciDataProcessed;
    }
    if (message != eciWaveformBuffer)
        return eciDataProcessed;
    if (!engine->config.pcm)
        return eciDataProcessed;
    return engine->config.pcm(engine->pcm, (size_t)parameter,
                              engine->config.context)
        ? eciDataProcessed : eciDataAbort;
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    static bool runtime_started;
    if (!config || !config->pcm)
        return NULL;
    if (!runtime_started) {
        evv_port_start();
        evvRunStaticInitialisers();
        runtime_started = true;
    }
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine)
        return NULL;
    engine->config = *config;
    engine->rate = 80;
    engine->volume = 80;
    engine->instance = eo_new();
    if (!engine->instance) {
        free(engine);
        return NULL;
    }
    eo_registerCallback(engine->instance, openevv_callback, engine);
    for (int i = 0; i < 8; ++i)
        engine->voice_params[i] = vc_getVoiceParam(engine->instance, 0, i);
    engine->rate = engine->voice_params[VOICE_SPEED];
    engine->volume = engine->voice_params[VOICE_VOLUME];
    engine->native_dictionary = eo_getParam(engine->instance, ENV_DICTIONARY);
    engine->native_dictionary = engine->native_dictionary == 0;
    engine->voice = 1;
    if (!ev_setOutputBuffer(engine->instance, OPENEVV_FRAME_SAMPLES,
                            engine->pcm)
        || !speech_engine_set(engine, SPEECH_SETTING_RATE, engine->rate)
        || !speech_engine_set(engine, SPEECH_SETTING_ENGINE_VOLUME,
                              engine->volume)
        || !speech_engine_set(engine, SPEECH_SETTING_PHRASE_PREDICTION, 0)) {
        speech_engine_destroy(engine);
        return NULL;
    }
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    if (engine->instance) (void)es_delete(engine->instance);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "OpenEVV" : "";
}

const char *speech_engine_storage_id(void) { return "oevv"; }

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? OPENEVV_SAMPLE_RATE : 0;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text) return false;
    (void)ev_setParam(engine->instance, PARAM_INPUT_TYPE,
                      engine->annotated_input ? 1 : 0);
    return et_addText(engine->instance, text)
        && et_synthesize(engine->instance);
}

bool speech_engine_insert_marker(speech_engine_t *engine, uint32_t marker)
{
    return engine && et_insertIndex(engine->instance, (int32_t)marker);
}

bool speech_engine_busy(speech_engine_t *engine)
{
    return engine && eo_speaking(engine->instance);
}

void speech_engine_wait(speech_engine_t *engine)
{
    if (engine) eo_synchronizeSynth(engine->instance);
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
    if (!info || index >= sizeof(settings) / sizeof(settings[0]))
        return false;
    *info = settings[index];
    return true;
}

bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t setting,
                       int value)
{
    if (!engine) return false;
    int voice_param = voice_parameter(setting);
    if (voice_param >= 0) {
        if (vc_setVoiceParam(engine->instance, 0, voice_param, value) < 0)
            return false;
        engine->voice_params[voice_param] = value;
        if (setting == SPEECH_SETTING_RATE) engine->rate = value;
        if (setting == SPEECH_SETTING_ENGINE_VOLUME) engine->volume = value;
        return true;
    }
    switch (setting) {
        case SPEECH_SETTING_PHRASE_PREDICTION:
            OI_ENV(engine->instance)[ENV_PHRASE_PREDICTION] = value ? 1 : 0;
            OI_ENV_SAVED(engine->instance)[ENV_PHRASE_PREDICTION] = 1;
            if (api_set_param(OI_NEW(engine->instance), 0,
                              PARAM_PHRASE_PREDICTION, value ? 1 : 0) != 0)
                return false;
            engine->phrase_prediction = value ? 1 : 0;
            return true;
        case SPEECH_SETTING_NATIVE_DICTIONARY:
            if (ev_setParam(engine->instance, ENV_DICTIONARY,
                            value ? 0 : 1) < 0)
                return false;
            engine->native_dictionary = value ? 1 : 0;
            return true;
        case SPEECH_SETTING_VOICE:
            if (value < 1 || value > 8
                || !vc_copyVoice(engine->instance, value, 0))
                return false;
            engine->voice = value;
            for (int i = 0; i < 8; ++i)
                engine->voice_params[i] = vc_getVoiceParam(engine->instance,
                                                            0, i);
            engine->rate = engine->voice_params[VOICE_SPEED];
            engine->volume = engine->voice_params[VOICE_VOLUME];
            return true;
        case SPEECH_SETTING_ANNOTATED_INPUT:
            engine->annotated_input = value ? 1 : 0;
            return true;
        default:
            return false;
    }
}

bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t setting,
                       int *value)
{
    if (!engine || !value) return false;
    int voice_param = voice_parameter(setting);
    if (voice_param >= 0) {
        *value = engine->voice_params[voice_param];
        return true;
    }
    switch (setting) {
        case SPEECH_SETTING_PHRASE_PREDICTION:
            *value = engine->phrase_prediction; return true;
        case SPEECH_SETTING_NATIVE_DICTIONARY:
            *value = engine->native_dictionary; return true;
        case SPEECH_SETTING_VOICE: *value = engine->voice; return true;
        case SPEECH_SETTING_ANNOTATED_INPUT:
            *value = engine->annotated_input; return true;
        default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    return engine != NULL;
}
