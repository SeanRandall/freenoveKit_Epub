#include "speech_engine.h"

#include <stdlib.h>
#include <string.h>

#include "truvoice.h"

#define TRUVOICE_SAMPLE_RATE 11025u
#define TRUVOICE_BUFFER_BYTES (TRUVOICE_SAMPLE_RATE * 2u)
/* The reader's common PCM sink uses a frame-sized stereo scratch buffer.
 * TruVoice normally returns about one second of mono audio per write, so
 * adapt its large writes here instead of imposing engine-specific buffer
 * requirements on the reader. */
#define TRUVOICE_PCM_FRAME_SAMPLES 256u

struct speech_engine {
    speech_engine_config_t config;
    TvSettings settings;
    uint8_t *buffer;
    bool busy;
    bool aborted;
    bool marker_pending;
    uint32_t pending_marker;
};

static const speech_setting_info_t settings[] = {
    { SPEECH_SETTING_RATE, "speaking rate", 60, 250, 5, 150, false,
      NULL, NULL },
    { SPEECH_SETTING_ENGINE_VOLUME, "voice volume", 0, 100, 5, 100,
      false, NULL, NULL },
    { SPEECH_SETTING_PITCH, "pitch", 20, 200, 5, 85, false, NULL, NULL },
    { SPEECH_SETTING_VOICE, "voice", 0, 9, 1, 0, false, NULL, NULL },
};

static int32_t output_write(void *context, const void *data, uint32_t bytes)
{
    speech_engine_t *engine = context;
    if (engine->aborted || (bytes & 1u) != 0) return -1;
    if (engine->marker_pending && engine->config.marker) {
        engine->config.marker(engine->pending_marker, engine->config.context);
        engine->marker_pending = false;
    }
    const int16_t *samples = (const int16_t *)data;
    size_t remaining = bytes / sizeof(*samples);
    while (remaining != 0) {
        size_t count = remaining < TRUVOICE_PCM_FRAME_SAMPLES
                           ? remaining : TRUVOICE_PCM_FRAME_SAMPLES;
        if (!engine->config.pcm(samples, count, engine->config.context)) {
            engine->aborted = true;
            return -1;
        }
        samples += count;
        remaining -= count;
    }
    return (int32_t)bytes;
}

static void output_seek(void *context, int32_t offset, int whence)
{
    (void)context;
    (void)offset;
    (void)whence;
}

static int cp1252(uint32_t codepoint)
{
    static const uint16_t high[32] = {
        0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
        0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
        0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
    };
    if (codepoint < 0x80 || (codepoint >= 0xa0 && codepoint <= 0xff))
        return (int)codepoint;
    for (int i = 0; i < 32; ++i)
        if (high[i] == codepoint) return 0x80 + i;
    return '?';
}

static char *to_cp1252(const char *text)
{
    size_t capacity = strlen(text) + 1;
    char *result = malloc(capacity);
    const unsigned char *p = (const unsigned char *)text;
    size_t used = 0;
    if (!result) return NULL;
    while (*p) {
        uint32_t codepoint = *p++;
        int extra = 0;
        if (codepoint >= 0xf0) { codepoint &= 7; extra = 3; }
        else if (codepoint >= 0xe0) { codepoint &= 15; extra = 2; }
        else if (codepoint >= 0xc0) { codepoint &= 31; extra = 1; }
        else if (codepoint >= 0x80) { codepoint = '?'; }
        while (extra-- > 0 && (*p & 0xc0) == 0x80)
            codepoint = (codepoint << 6) | (*p++ & 0x3f);
        result[used++] = (char)cp1252(codepoint);
    }
    result[used] = 0;
    return result;
}

speech_engine_t *speech_engine_create(const speech_engine_config_t *config)
{
    if (!config || !config->pcm) return NULL;
    speech_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    engine->config = *config;
    tv_settings_default(&engine->settings, 0);
    engine->buffer = malloc(TRUVOICE_BUFFER_BYTES);
    if (!engine->buffer) {
        free(engine);
        return NULL;
    }
    return engine;
}

void speech_engine_destroy(speech_engine_t *engine)
{
    if (!engine) return;
    free(engine->buffer);
    free(engine);
}

const char *speech_engine_name(const speech_engine_t *engine)
{
    return engine ? "TruVoice" : "";
}

const char *speech_engine_storage_id(void) { return "truvoice-en-5.1"; }

unsigned speech_engine_sample_rate(const speech_engine_t *engine)
{
    return engine ? engine->settings.sample_rate : 0;
}

bool speech_engine_synthesize(speech_engine_t *engine, const char *text)
{
    if (!engine || !text || !*text) return false;
    char *encoded = to_cp1252(text);
    if (!encoded) return false;
    char *prepared = tv_prepare_text(&engine->settings, encoded);
    free(encoded);
    if (!prepared) return false;
    TvOut output = { output_write, output_seek, engine };
    engine->busy = true;
    engine->aborted = false;
    int32_t bytes = tv_synthesize(&output, prepared, engine->buffer,
                                  &engine->settings);
    engine->busy = false;
    free(prepared);
    return bytes >= 0 && !engine->aborted;
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
    case SPEECH_SETTING_RATE: engine->settings.rate = (uint16_t)value; return true;
    case SPEECH_SETTING_PITCH: engine->settings.pitch = (uint16_t)value; return true;
    case SPEECH_SETTING_ENGINE_VOLUME:
        engine->settings.volume = (uint16_t)((value * 16 + 50) / 100);
        return true;
    case SPEECH_SETTING_VOICE: engine->settings.voice = (uint32_t)value; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: return true;
    default: return false;
    }
}

bool speech_engine_get(speech_engine_t *engine, speech_setting_id_t id, int *value)
{
    if (!engine || !value) return false;
    switch (id) {
    case SPEECH_SETTING_RATE: *value = engine->settings.rate; return true;
    case SPEECH_SETTING_PITCH: *value = engine->settings.pitch; return true;
    case SPEECH_SETTING_ENGINE_VOLUME:
        *value = (engine->settings.volume * 100 + 8) / 16; return true;
    case SPEECH_SETTING_VOICE: *value = (int)engine->settings.voice; return true;
    case SPEECH_SETTING_ANNOTATED_INPUT: *value = 0; return true;
    default: return false;
    }
}

bool speech_engine_supports_annotations(const speech_engine_t *engine)
{
    (void)engine;
    /* TruVoice accepts & followed by one of its phoneme symbols while it is
       otherwise in normal text mode. */
    return true;
}
