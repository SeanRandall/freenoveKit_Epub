#include "speech_engine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dtalk.h"

#define DT_PCM_SAMPLES 2048

extern const unsigned char doubletalk_rom_start[]
    asm("_binary_doubletalkpc_bin_start");
extern const unsigned char doubletalk_rom_end[]
    asm("_binary_doubletalkpc_bin_end");

struct speech_engine {
    speech_engine_config_t config;
    dtalk *native;
    int rate;
    int pitch;
    int volume;
    int voice;
    int rate_boost;
    int tone;
    int articulation;
    int expression;
    bool busy;
    bool marker_pending;
    uint32_t pending_marker;
};

static const char *const tone_labels[] = {"bass", "normal", "treble"};

static const speech_setting_info_t settings[] = {
    {SPEECH_SETTING_RATE, "speaking rate", 60, 140, 5, 100, false, nullptr, nullptr, nullptr},
    {SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 60, false, nullptr, nullptr, nullptr},
    {SPEECH_SETTING_PITCH, "pitch", 0, 100, 5, 50, false, nullptr, nullptr, nullptr},
    {SPEECH_SETTING_VOICE, "voice", 0, 7, 1, 0, false, nullptr, nullptr, nullptr},
    {SPEECH_SETTING_RATE_BOOST, "rate boost", 0, 1, 1, 0, true, "off", "on", nullptr},
    {SPEECH_SETTING_TONE, "tone", 0, 2, 1, 1, false, nullptr, nullptr, tone_labels},
    {SPEECH_SETTING_ARTICULATION, "articulation", 0, 9, 1, 5, false, nullptr, nullptr, nullptr},
    {SPEECH_SETTING_EXPRESSION, "expression", 0, 9, 1, 5, false, nullptr, nullptr, nullptr},
};

struct voice_quality_preset { int articulation, expression, tone; };
static const voice_quality_preset voice_presets[] = {
    {5, 5, 1}, {4, 7, 1}, {4, 6, 0}, {8, 4, 2},
    {5, 5, 1}, {5, 5, 1}, {4, 7, 1}, {4, 6, 0},
};

static int native_rate(int rate)
{
    int value = (rate - 60) * 9 / 80;
    return value < 0 ? 0 : (value > 9 ? 9 : value);
}

static int native_pitch(int pitch)
{
    int value = pitch * 99 / 100;
    return value < 0 ? 0 : (value > 99 ? 99 : value);
}

static int native_volume(int volume)
{
    int value = volume * 9 / 100;
    return value < 0 ? 0 : (value > 9 ? 9 : value);
}

extern "C" speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return nullptr;
    speech_engine *engine = static_cast<speech_engine *>(calloc(1, sizeof(*engine)));
    if (!engine) return nullptr;
    engine->config = *config;
    engine->rate = 100;
    engine->pitch = 50;
    engine->volume = 60;
    engine->tone = 1;
    engine->articulation = 5;
    engine->expression = 5;
    size_t rom_size = static_cast<size_t>(doubletalk_rom_end - doubletalk_rom_start);
    int64_t start = esp_timer_get_time();
    engine->native = dtalk_create(doubletalk_rom_start, rom_size);
    printf("DOUBLETALK_CREATE rom=%u bytes wall_ms=%lld result=%s\n",
           static_cast<unsigned>(rom_size),
           static_cast<long long>((esp_timer_get_time() - start) / 1000),
           engine->native ? "ok" : "failed");
    if (!engine->native) {
        free(engine);
        return nullptr;
    }
    return engine;
}

extern "C" void speech_engine_destroy(speech_engine_t *opaque)
{
    speech_engine *engine = static_cast<speech_engine *>(opaque);
    if (!engine) return;
    dtalk_destroy(engine->native);
    free(engine);
}

extern "C" const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "DoubleTalk PC" : "";
}

extern "C" const char *speech_engine_storage_id(void) { return "doubletalk-pc"; }

extern "C" unsigned speech_engine_sample_rate(const speech_engine_t *opaque)
{
    const speech_engine *engine = static_cast<const speech_engine *>(opaque);
    return engine ? dtalk_sample_rate(engine->native) : 0;
}

extern "C" void speech_engine_playback_hints(
    const speech_engine_t *opaque, speech_engine_playback_hints_t *hints)
{
    (void)opaque;
    if (!hints) return;
    hints->initial_buffer_ms = 500;
    hints->navigation_buffer_ms = 300;
}

extern "C" bool speech_engine_synthesize(speech_engine_t *opaque, const char *text)
{
    speech_engine *engine = static_cast<speech_engine *>(opaque);
    if (!engine || !engine->native || !text || !*text) return false;
    size_t text_length = strlen(text);
    char prefix[96];
    int prefix_length = snprintf(prefix, sizeof(prefix),
        "\x01%dO\x01%dS\x01%dP\x01%dV\x01%dX\x01%dA\x01%dE\x01" "14B", engine->voice,
        native_rate(engine->rate), native_pitch(engine->pitch),
        native_volume(engine->volume), engine->tone,
        engine->articulation, engine->expression);
    char *utterance = static_cast<char *>(malloc(
        static_cast<size_t>(prefix_length) + text_length + 2));
    if (!utterance) return false;
    memcpy(utterance, prefix, static_cast<size_t>(prefix_length));
    size_t output = static_cast<size_t>(prefix_length);
    for (size_t i = 0; i < text_length; ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        /* Ctrl-A is DoubleTalk's command introducer.  It can be written as
         * \\x01 in an engine-specific substitutions file. */
        utterance[output++] = (c == 0x01 || (c >= 0x20 && c <= 0x7e))
            ? static_cast<char>(c) : ' ';
    }
    utterance[output++] = '\r';

    int boost = engine->rate_boost ? dtalk_rate_boost_max() : 0;
    dtalk_set_rate_boost(engine->native, boost);
    engine->busy = true;
    int64_t wall_start = esp_timer_get_time();
    dtalk_queue(engine->native, utterance, output);
    free(utterance);
    /* RetroReader inserts markers only at the boundary between synchronous
       synthesis requests.  No PCM from this request has reached its callback
       yet, so the reader's current sample count is the exact boundary.  The
       authentic DoubleTalk index command is generated by its emulated audio
       path; Hybrid bypasses that path for native acoustic rendering and the
       corresponding hardware event is therefore not reliable. */
    if (engine->marker_pending && engine->config.marker) {
        engine->config.marker(engine->pending_marker, engine->config.context);
        printf("DOUBLETALK_MARKER marker=%u boundary=start\n",
               static_cast<unsigned>(engine->pending_marker));
        engine->marker_pending = false;
    }
    int16_t pcm[DT_PCM_SAMPLES];
    uint64_t total_samples = 0;
    bool ok = true;
    while (ok) {
        size_t count = dtalk_synth16(engine->native, pcm, DT_PCM_SAMPLES);
        if (!count) break;
        total_samples += count;
        ok = engine->config.pcm(pcm, count, engine->config.context);
        // Front-end emulation plus native acoustic rendering can now produce
        // several seconds of audio per second of wall time.  Yield between
        // output blocks so a long chunk does not starve CPU 1's idle task and
        // trip the task watchdog.
        if (ok) vTaskDelay(1);
    }
    if (!ok) dtalk_stop(engine->native);
    int64_t wall_us = esp_timer_get_time() - wall_start;
    uint64_t audio_ms = total_samples * 1000ULL /
        static_cast<uint64_t>(dtalk_sample_rate(engine->native));
    printf("DOUBLETALK_RENDER chars=%u rate=%d native=%d boost=%d audio_ms=%llu wall_ms=%lld x_realtime=%.2f ok=%d\n",
           static_cast<unsigned>(text_length), engine->rate,
           native_rate(engine->rate), boost,
           static_cast<unsigned long long>(audio_ms),
           static_cast<long long>(wall_us / 1000),
           wall_us > 0 ? static_cast<double>(audio_ms) * 1000.0 / static_cast<double>(wall_us) : 0.0,
           ok ? 1 : 0);
    engine->busy = false;
    return ok;
}

extern "C" bool speech_engine_insert_marker(speech_engine_t *opaque, uint32_t marker)
{
    speech_engine *engine = static_cast<speech_engine *>(opaque);
    if (!engine) return false;
    engine->pending_marker = marker;
    engine->marker_pending = true;
    return true;
}

extern "C" bool speech_engine_busy(speech_engine_t *opaque)
{
    speech_engine *engine = static_cast<speech_engine *>(opaque);
    return engine && engine->busy;
}

extern "C" void speech_engine_wait(speech_engine_t *engine) { (void)engine; }

extern "C" size_t speech_engine_setting_count(const speech_engine_t *engine)
{
    (void)engine;
    return sizeof(settings) / sizeof(settings[0]);
}

extern "C" bool speech_engine_setting_info(const speech_engine_t *engine,
                                              size_t index, speech_setting_info_t *info)
{
    (void)engine;
    if (!info || index >= sizeof(settings) / sizeof(settings[0])) return false;
    *info = settings[index];
    return true;
}

extern "C" bool speech_engine_set(speech_engine_t *opaque,
                                    speech_setting_id_t setting, int value)
{
    speech_engine *engine = static_cast<speech_engine *>(opaque);
    if (!engine) return false;
    switch (setting) {
    case SPEECH_SETTING_RATE: engine->rate = value; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: engine->volume = value; return true;
    case SPEECH_SETTING_PITCH: engine->pitch = value; return true;
    case SPEECH_SETTING_VOICE:
        engine->voice = value;
        if (value >= 0 && value < static_cast<int>(sizeof(voice_presets) / sizeof(voice_presets[0]))) {
            engine->articulation = voice_presets[value].articulation;
            engine->expression = voice_presets[value].expression;
            engine->tone = voice_presets[value].tone;
        }
        return true;
    case SPEECH_SETTING_RATE_BOOST: engine->rate_boost = !!value; return true;
    case SPEECH_SETTING_TONE: engine->tone = value; return true;
    case SPEECH_SETTING_ARTICULATION: engine->articulation = value; return true;
    case SPEECH_SETTING_EXPRESSION: engine->expression = value; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: return true;
    default: return false;
    }
}

extern "C" bool speech_engine_get(speech_engine_t *opaque,
                                    speech_setting_id_t setting, int *value)
{
    speech_engine *engine = static_cast<speech_engine *>(opaque);
    if (!engine || !value) return false;
    switch (setting) {
    case SPEECH_SETTING_RATE: *value = engine->rate; return true;
    case SPEECH_SETTING_ENGINE_VOLUME: *value = engine->volume; return true;
    case SPEECH_SETTING_PITCH: *value = engine->pitch; return true;
    case SPEECH_SETTING_VOICE: *value = engine->voice; return true;
    case SPEECH_SETTING_RATE_BOOST: *value = engine->rate_boost; return true;
    case SPEECH_SETTING_TONE: *value = engine->tone; return true;
    case SPEECH_SETTING_ARTICULATION: *value = engine->articulation; return true;
    case SPEECH_SETTING_EXPRESSION: *value = engine->expression; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 1; return true;
    default: return false;
    }
}

extern "C" bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    return true;
}
