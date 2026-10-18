#include "speech_engine.h"

#include <stdlib.h>

#include "rsynth_api.h"

#define RSYNTH_SAMPLE_RATE 16000u

struct speech_engine {
    speech_engine_config_t config;
    rs_engine *rs;
    int rate;
    int pitch;
    int volume;
    bool busy;
    bool aborted;
    bool marker_pending;
    uint32_t pending_marker;
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 50, 400, 5, 160, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 70, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 40, 240, 5, 133, false,
      NULL, NULL, NULL },
};

static void apply_settings(speech_engine_t *engine)
{
    /* RSynth's duration unit is the frame.  Ten milliseconds is its
       historical 100-percent rate; retain every element while changing the
       frame duration rather than dropping short phones with rs_set_speed. */
    rs_set_frame_ms(engine->rs, 1000.0 / (double)engine->rate);
    rs_set_speed(engine->rs, 1.0);
    rs_set_f0(engine->rs, (double)engine->pitch);
    rs_set_volume(engine->rs, (double)engine->volume / 100.0);
    /* The first embedded build intentionally uses RSynth's compact NRL
       letter-to-sound rules.  The much larger CMU dictionaries are not
       presented as selectable voices until they have an indexed SD reader. */
    rs_set_lexicon(engine->rs, RS_LEX_RULES);
}

static int on_audio(void *context, const int16_t *samples, int count)
{
    speech_engine_t *engine = context;
    if (engine->aborted) return 1;
    if (engine->marker_pending && engine->config.marker) {
        engine->config.marker(engine->pending_marker, engine->config.context);
        engine->marker_pending = false;
    }
    if (count <= 0) return 0;
    if (!engine->config.pcm(samples, (size_t)count, engine->config.context)) {
        engine->aborted = true;
        return 1;
    }
    return 0;
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return NULL;
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    engine->config = *config;
    engine->rate = 160;
    engine->pitch = 133;
    engine->volume = 70;
    engine->rs = rs_create(RSYNTH_SAMPLE_RATE);
    if (!engine->rs) {
        free(engine);
        return NULL;
    }
    apply_settings(engine);
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    rs_destroy(engine->rs);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{ return engine ? "RSynth" : ""; }
const char *speech_engine_storage_id(void) { return "rsynth-en"; }
unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{ return engine ? RSYNTH_SAMPLE_RATE : 0; }

void speech_engine_playback_hints(const speech_engine_t *engine,
                                  speech_engine_playback_hints_t *hints)
{
    (void)engine;
    if (!hints) return;
    hints->initial_buffer_ms = 350;
    hints->navigation_buffer_ms = 120;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text || !*text) return false;
    engine->busy = true;
    engine->aborted = false;
    apply_settings(engine);
    int result = rs_speak(engine->rs, text, 0, on_audio, engine);
    engine->busy = false;
    return result >= 0 && !engine->aborted;
}

bool speech_engine_insert_marker(speech_engine_t *engine, uint32_t marker)
{
    if (!engine) return false;
    engine->pending_marker = marker;
    engine->marker_pending = true;
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
    if (!info || index >= sizeof(settings) / sizeof(settings[0])) return false;
    *info = settings[index];
    return true;
}

bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t id, int value)
{
    if (!engine) return false;
    switch (id) {
    case SPEECH_SETTING_RATE: engine->rate = value; break;
    case SPEECH_SETTING_PITCH: engine->pitch = value; break;
    case SPEECH_SETTING_ENGINE_VOLUME: engine->volume = value; break;
    default: return false;
    }
    apply_settings(engine);
    return true;
}

bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t id, int *value)
{
    if (!engine || !value) return false;
    switch (id) {
    case SPEECH_SETTING_RATE: *value = engine->rate; return true;
    case SPEECH_SETTING_PITCH: *value = engine->pitch; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: *value = engine->volume; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    /* Bracketed SAMPA is deliberately escaped by the safe text frontend.
       Direct phoneme substitutions can be exposed later through a separate
       adapter entry point rather than making ordinary book text executable. */
    return false;
}
