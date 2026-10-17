#include "speech_engine.h"

#include <stdlib.h>
#include <string.h>

#include "prose.h"

#ifndef RR_PROSE_VERSION
#define RR_PROSE_VERSION PROSE_V341
#endif

#define PROSE_PCM_BUFFER_SAMPLES 256u

struct speech_engine {
    speech_engine_config_t config;
    prose_h prose;
    int rate;
    int volume;
    int pitch;
    int voice;
    bool busy;
    bool aborted;
    bool marker_pending;
    uint32_t pending_marker;
    int16_t pcm[PROSE_PCM_BUFFER_SAMPLES];
};

static const char *const voice_labels[] = {
    "voice one", "voice two", "voice three"
};

static const speech_setting_info_t settings_v341[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 50, 250, 10, 150, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 80, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 50, 200, 5, 85, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_VOICE, "voice", 0, 2, 1, 0, false,
      NULL, NULL, voice_labels },
};

static const speech_setting_info_t settings_v11[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 50, 250, 10, 160, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 80, false,
      NULL, NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 50, 200, 5, 75, false,
      NULL, NULL, NULL },
};

static const speech_setting_info_t *engine_settings(size_t *count)
{
    if (RR_PROSE_VERSION == PROSE_V11) {
        *count = sizeof(settings_v11) / sizeof(settings_v11[0]);
        return settings_v11;
    }
    *count = sizeof(settings_v341) / sizeof(settings_v341[0]);
    return settings_v341;
}

static char *ascii_text(const char *text)
{
    size_t length = strlen(text);
    char *result = malloc(length + 1);
    if (!result) return NULL;
    const unsigned char *source = (const unsigned char *)text;
    char *destination = result;
    while (*source) {
        if (source[0] == 0xe2 && source[1] == 0x80) {
            switch (source[2]) {
            case 0x98: case 0x99: *destination++ = '\''; source += 3; continue;
            case 0x9c: case 0x9d: *destination++ = '"'; source += 3; continue;
            case 0x93: case 0x94: *destination++ = '-'; source += 3; continue;
            case 0xa6:
                *destination++ = '.'; *destination++ = '.'; *destination++ = '.';
                source += 3; continue;
            default: break;
            }
        }
        if (*source < 0x80) {
            *destination++ = (char)*source++;
        } else {
            while ((*++source & 0xc0) == 0x80) {}
            *destination++ = ' ';
        }
    }
    *destination = 0;
    return result;
}

static int on_audio(prose_h handle, const int16_t *pcm, size_t count,
                    uint32_t position, void *context)
{
    (void)handle;
    (void)position;
    speech_engine_t *engine = context;
    if (engine->marker_pending && engine->config.marker) {
        engine->config.marker(engine->pending_marker, engine->config.context);
        engine->marker_pending = false;
    }
    if (!engine->config.pcm(pcm, count, engine->config.context)) {
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
    engine->rate = RR_PROSE_VERSION == PROSE_V11 ? 160 : 150;
    engine->volume = 80;
    engine->pitch = RR_PROSE_VERSION == PROSE_V11 ? 75 : 85;
    if (prose_open(&engine->prose, RR_PROSE_VERSION) != PROSE_OK) {
        free(engine);
        return NULL;
    }
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    prose_close(engine->prose);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    if (!engine) return "";
    return RR_PROSE_VERSION == PROSE_V11 ? "Prose 2000 1983" : "Prose 2000 1988";
}

const char *speech_engine_storage_id(void)
{
    return RR_PROSE_VERSION == PROSE_V11 ? "prose2000-1983" : "prose2000-1988";
}

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? PROSE_SAMPLE_RATE : 0;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text || !*text) return false;
    char *prepared = ascii_text(text);
    if (!prepared) return false;
    prose_set_rate(engine->prose, engine->rate);
    prose_set_volume(engine->prose, (engine->volume * 15 + 50) / 100);
    prose_set_pitch(engine->prose, engine->pitch);
    if (RR_PROSE_VERSION == PROSE_V341)
        prose_set_voice(engine->prose, engine->voice);
    engine->busy = true;
    engine->aborted = false;
    int32_t samples = prose_speak_to_buffer(engine->prose, prepared,
                                             engine->pcm,
                                             PROSE_PCM_BUFFER_SAMPLES,
                                             on_audio, engine);
    engine->busy = false;
    free(prepared);
    return samples >= 0 && !engine->aborted;
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
{
    (void)engine;
    size_t count;
    (void)engine_settings(&count);
    return count;
}

bool speech_engine_setting_info(const speech_engine_t *engine, size_t index,
                                speech_setting_info_t *info)
{
    (void)engine;
    size_t count;
    const speech_setting_info_t *settings = engine_settings(&count);
    if (!info || index >= count) return false;
    *info = settings[index];
    return true;
}

bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t id, int value)
{
    if (!engine) return false;
    switch (id) {
    case SPEECH_SETTING_RATE: engine->rate = value; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: engine->volume = value; return true;
    case SPEECH_SETTING_PITCH: engine->pitch = value; return true;
    case SPEECH_SETTING_VOICE:
        if (RR_PROSE_VERSION != PROSE_V341) return false;
        engine->voice = value; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: return true;
    default: return false;
    }
}

bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t id, int *value)
{
    if (!engine || !value) return false;
    switch (id) {
    case SPEECH_SETTING_RATE: *value = engine->rate; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: *value = engine->volume; return true;
    case SPEECH_SETTING_PITCH: *value = engine->pitch; return true;
    case SPEECH_SETTING_VOICE:
        if (RR_PROSE_VERSION != PROSE_V341) return false;
        *value = engine->voice; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 0; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    return false;
}
