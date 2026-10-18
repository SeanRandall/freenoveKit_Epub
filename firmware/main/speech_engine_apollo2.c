#include "speech_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "engine/frame_list.h"
#include "engine/native_engine.h"
#include "synth/params.h"
#include "synth/synth.h"

#define APOLLO2_SAMPLE_RATE 10000u
#define APOLLO2_PCM_FRAME_SAMPLES 100u
#define APOLLO2_PITCH_PROBE_FRAMES 32u

struct speech_engine {
    speech_engine_config_t config;
    native_engine *native;
    int rate;
    int volume;
    int pitch;
    int voice;
    int prosody;
    bool busy;
    bool aborted;
    bool marker_pending;
    uint32_t pending_marker;
};

static const char *const voice_labels[] = {
    "male", "non-male"
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 1, 9, 1, 3, false,
      NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 65, false,
      NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 0, 15, 1, 8, false,
      NULL, NULL },
    { SPEECH_SETTING_VOICE, "voice", 0, 1, 1, 0, false,
      NULL, NULL, voice_labels },
    { SPEECH_SETTING_EXPRESSION, "expression", 0, 7, 1, 3, false,
      NULL, NULL },
};

static char hex_digit(int value)
{
    static const char digits[] = "0123456789ABCDEF";
    return digits[value & 15];
}

static char *prepare_text(const speech_engine_t *engine, const char *text)
{
    size_t length = strlen(text);
    char prefix[48];
    int prefix_length = snprintf(prefix, sizeof(prefix),
                                 "@K%d @F%c @R%d @W%d @A%c ",
                                 engine->voice, hex_digit(engine->pitch),
                                 engine->prosody, engine->rate,
                                 hex_digit((engine->volume * 15 + 50) / 100));
    if (prefix_length < 0 || (size_t)prefix_length >= sizeof(prefix)) return NULL;
    char *prepared = malloc((size_t)prefix_length + length + 1);
    if (!prepared) return NULL;
    memcpy(prepared, prefix, (size_t)prefix_length);
    memcpy(prepared + prefix_length, text, length + 1);
    return prepared;
}

typedef struct {
    speech_engine_t *engine;
    synth renderer;
    uint8_t probe[APOLLO2_PITCH_PROBE_FRAMES][ENGINE_FRAME_BYTES];
    size_t probe_count;
    size_t frames_emitted;
    bool renderer_ready;
} apollo_stream_t;

static bool emit_frame(apollo_stream_t *stream,
                       const uint8_t frame[ENGINE_FRAME_BYTES])
{
    int16_t pcm[APOLLO2_PCM_FRAME_SAMPLES];
    synth_frame(&stream->renderer, frame, pcm);
    if (!stream->engine->config.pcm(pcm, APOLLO2_PCM_FRAME_SAMPLES,
                                    stream->engine->config.context)) {
        stream->engine->aborted = true;
        return false;
    }
    /* Rendering into the book ring is deliberately faster than real time and
       otherwise keeps CPU 1 continuously runnable. Let its idle task feed the
       watchdog without materially reducing synthesis throughput. */
    if ((++stream->frames_emitted & 31u) == 0)
        vTaskDelay(1);
    return true;
}

static bool begin_renderer(apollo_stream_t *stream)
{
    synth_options options;
    synth_default_options(&options);
    options.f0_centre_hz = params_f0_centre(&stream->probe[0][0],
                                             stream->probe_count);
    synth_init(&stream->renderer, &options);
    stream->renderer_ready = true;
    if (stream->engine->marker_pending && stream->engine->config.marker) {
        stream->engine->config.marker(stream->engine->pending_marker,
                                      stream->engine->config.context);
        stream->engine->marker_pending = false;
    }
    for (size_t i = 0; i < stream->probe_count; ++i)
        if (!emit_frame(stream, stream->probe[i])) return false;
    stream->probe_count = 0;
    return true;
}

static bool on_apollo_frame(void *context,
                            const uint8_t frame[ENGINE_FRAME_BYTES])
{
    apollo_stream_t *stream = context;
    if (!stream->renderer_ready) {
        memcpy(stream->probe[stream->probe_count++], frame,
               ENGINE_FRAME_BYTES);
        if (stream->probe_count < APOLLO2_PITCH_PROBE_FRAMES) return true;
        return begin_renderer(stream);
    }
    return emit_frame(stream, frame);
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return NULL;
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    engine->native = calloc(1, sizeof(*engine->native));
    if (!engine->native) {
        free(engine);
        return NULL;
    }
    engine->config = *config;
    engine->rate = 3;
    engine->volume = 65;
    engine->pitch = 8;
    engine->voice = 0;
    engine->prosody = 3;
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    free(engine->native);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "Dolphin Apollo II" : "";
}

const char *speech_engine_storage_id(void) { return "apollo2-en-gb"; }

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? APOLLO2_SAMPLE_RATE : 0;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text || !*text) return false;
    char *prepared = prepare_text(engine, text);
    if (!prepared) return false;

    apollo_stream_t stream = { .engine = engine };
    native_watch watch = {
        .user = &stream,
        .on_output = on_apollo_frame,
    };
    engine_input input = {
        .bytes = (const uint8_t *)prepared,
        .n = strlen(prepared),
        .cr = true,
    };
    engine->busy = true;
    engine->aborted = false;
    engine_status status = native_engine_run_input(engine->native, NULL, &input,
                                                   false, NULL, NULL, &watch);
    free(prepared);
    if (!engine->aborted && !stream.renderer_ready && stream.probe_count)
        (void)begin_renderer(&stream);
    engine->busy = false;
    return status == ENGINE_IDLE && !engine->aborted;
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

bool speech_engine_set(speech_engine_t *engine, speech_setting_id_t id, int value)
{
    if (!engine) return false;
    switch (id) {
    case SPEECH_SETTING_RATE: engine->rate = value; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: engine->volume = value; return true;
    case SPEECH_SETTING_PITCH: engine->pitch = value; return true;
    case SPEECH_SETTING_VOICE: engine->voice = value; return true;
    case SPEECH_SETTING_EXPRESSION: engine->prosody = value; return true;
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
    case SPEECH_SETTING_VOICE: *value = engine->voice; return true;
    case SPEECH_SETTING_EXPRESSION: *value = engine->prosody; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 1; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    /* The native Apollo firmware parser accepts embedded _PH phonemes and
       its original @ command language. */
    return true;
}
