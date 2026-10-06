#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver/i2s_std.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_sntp.h"
#include "evv_abi.h"
#include "transfer_mode.h"
#include "remsound_receiver.h"

#ifndef TRANSFER_WIFI_SSID
#define TRANSFER_WIFI_SSID ""
#endif
#ifndef TRANSFER_WIFI_PASSWORD
#define TRANSFER_WIFI_PASSWORD ""
#endif

typedef struct OldInst OldInst;
enum ECIMessage { eciWaveformBuffer, eciPhonemeBuffer, eciIndexReply };
enum ECICallbackReturn { eciDataNotProcessed, eciDataProcessed, eciDataAbort };
OldInst *STDCALL eo_new(void);
int STDCALL et_addText(OldInst *, const char *);
int STDCALL et_synthesize(OldInst *);
int STDCALL ev_setOutputBuffer(OldInst *, int32_t, void *);
int STDCALL vc_setVoiceParam(OldInst *, int32_t, int32_t, int32_t);
void STDCALL eo_registerCallback(OldInst *, void *, void *);
void STDCALL eo_synchronizeSynth(OldInst *);
int STDCALL eo_speaking(OldInst *);
void evvRunStaticInitialisers(void);
void evv_port_start(void);

#define FRAME_SAMPLES 2048
#define BOOT_REQUEST_READER UINT32_C(0x45565652)
#define BOOT_REQUEST_TRANSFER UINT32_C(0x45565654)
#define VOICE_SPEED 6
#define VOICE_VOLUME 7
#define ENGINE_VOLUME_MAX 80
#define REMOTE_SILENCE_SAMPLES 128
#define REMOTE_FIFO_SAMPLES 16384
#define REMOTE_PREFILL_CHARACTER_SAMPLES 512
#define REMOTE_PREFILL_SHORT_SAMPLES 1536
#define REMOTE_PREFILL_SENTENCE_SAMPLES 4096
#define REMOTE_PREFILL_LONG_SAMPLES 8192
#define REMOTE_PREFILL_PASSAGE_SAMPLES 12288
#define BOOT_STATE_NAMESPACE "evv_boot"
#define REMSOUND_HANDOFF_KEY "remsound"
static int16_t *mono;
static int16_t *stereo;
static i2s_chan_handle_t audio;
static QueueHandle_t presses;
static volatile bool announcement_active;
static volatile bool transfer_ready;
static volatile bool remote_stop;
static volatile bool remote_task_running;
static volatile bool remote_cancel;
static QueueHandle_t remote_speech;
static int saved_rate = 105;
static int saved_volume = 20;
static bool is_remote_mode;
static SemaphoreHandle_t remote_i2s_mutex;
static int16_t *remote_fifo;
static size_t remote_fifo_read;
static size_t remote_fifo_write;
static size_t remote_fifo_count;
static size_t remote_fifo_maximum;
static size_t remote_fifo_prefill = REMOTE_PREFILL_SHORT_SAMPLES;
static bool remote_fifo_ready;
static volatile bool remote_stream_active;
static unsigned remote_stream_writes;
static unsigned remote_silence_writes;
static unsigned remote_fifo_underflows;

static void initialise_nvs(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES
        || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);
}

static esp_err_t save_remsound_handoff(void)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(BOOT_STATE_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK)
        return result;
    result = nvs_set_u8(handle, REMSOUND_HANDOFF_KEY, 1);
    if (result == ESP_OK)
        result = nvs_commit(handle);
    nvs_close(handle);
    return result;
}

static bool take_remsound_handoff(void)
{
    nvs_handle_t handle;
    if (nvs_open(BOOT_STATE_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
        return false;
    uint8_t requested = 0;
    esp_err_t result = nvs_get_u8(handle, REMSOUND_HANDOFF_KEY, &requested);
    if (result == ESP_OK) {
        (void)nvs_erase_key(handle, REMSOUND_HANDOFF_KEY);
        (void)nvs_commit(handle);
    }
    nvs_close(handle);
    return result == ESP_OK && requested == 1;
}

typedef struct {
    char *text;
} remote_speech_t;

typedef struct {
    adc_oneshot_unit_handle_t adc;
    adc_cali_handle_t calibration;
} buttons_t;

static enum ECICallbackReturn STDCALL waveform(OldInst *voice,
    enum ECIMessage message, long parameter, void *context)
{
    (void)voice; (void)context;
    if (message != eciWaveformBuffer) return eciDataProcessed;
    size_t count = (size_t)parameter;
    if (remote_cancel) return eciDataAbort;
    if (is_remote_mode && remote_i2s_mutex) {
        size_t offset = 0;
        remote_stream_active = true;
        while (offset < count && !remote_cancel) {
            xSemaphoreTake(remote_i2s_mutex, portMAX_DELAY);
            size_t space = REMOTE_FIFO_SAMPLES - remote_fifo_count;
            size_t copy = count - offset;
            if (copy > space) copy = space;
            size_t tail = REMOTE_FIFO_SAMPLES - remote_fifo_write;
            if (copy > tail) copy = tail;
            for (size_t i = 0; i < copy; ++i)
                remote_fifo[remote_fifo_write + i] =
                    (int16_t)(((int32_t)mono[offset + i] * saved_volume)
                              / 100);
            remote_fifo_write = (remote_fifo_write + copy)
                                % REMOTE_FIFO_SAMPLES;
            remote_fifo_count += copy;
            if (remote_fifo_count > remote_fifo_maximum)
                remote_fifo_maximum = remote_fifo_count;
            if (!remote_fifo_ready
                && remote_fifo_count >= remote_fifo_prefill)
                remote_fifo_ready = true;
            xSemaphoreGive(remote_i2s_mutex);
            offset += copy;
            if (!copy)
                vTaskDelay(pdMS_TO_TICKS(1));
        }
        ++remote_stream_writes;
        return remote_cancel ? eciDataAbort : eciDataProcessed;
    }
    for (size_t i = 0; i < count; ++i) {
        int32_t sample = ((int32_t)mono[i] * saved_volume) / 100;
        stereo[i * 2] = stereo[i * 2 + 1] = (int16_t)sample;
    }
    size_t written;
    return i2s_channel_write(audio, stereo, count * 4, &written,
                             portMAX_DELAY) == ESP_OK
        ? eciDataProcessed : eciDataAbort;
}

static void remote_playback_task(void *arg)
{
    (void)arg;
    int16_t local[REMOTE_SILENCE_SAMPLES];
    static const int16_t silence[REMOTE_SILENCE_SAMPLES * 2];
    size_t preloaded = 0;
    /* The TX descriptors are circular.  Populate them with silence before
       enabling the channel so an empty remote queue can never expose stale
       RAM or repeat audio from the preceding announcement. */
    (void)i2s_channel_preload_data(audio, silence, sizeof(silence),
                                   &preloaded);
    ESP_ERROR_CHECK(i2s_channel_enable(audio));
    printf("REMOTE_STREAM i2s_silence_preloaded=%u frame_samples=%u\n",
           (unsigned)preloaded, (unsigned)REMOTE_SILENCE_SAMPLES);
    for (;;) {
        size_t take = 0;
        bool underflow = false;
        xSemaphoreTake(remote_i2s_mutex, portMAX_DELAY);
        if (remote_fifo_ready && remote_fifo_count) {
            take = remote_fifo_count;
            if (take > REMOTE_SILENCE_SAMPLES)
                take = REMOTE_SILENCE_SAMPLES;
            size_t tail = REMOTE_FIFO_SAMPLES - remote_fifo_read;
            if (take > tail) take = tail;
            memcpy(local, remote_fifo + remote_fifo_read,
                   take * sizeof(*local));
            remote_fifo_read = (remote_fifo_read + take)
                               % REMOTE_FIFO_SAMPLES;
            remote_fifo_count -= take;
            if (remote_fifo_count == 0 && take < REMOTE_SILENCE_SAMPLES) {
                remote_fifo_ready = false;
                underflow = remote_stream_active;
            }
        } else if (remote_fifo_ready && !remote_fifo_count) {
            remote_fifo_ready = false;
            underflow = remote_stream_active;
        }
        if (underflow)
            ++remote_fifo_underflows;
        xSemaphoreGive(remote_i2s_mutex);

        for (size_t i = 0; i < REMOTE_SILENCE_SAMPLES; ++i) {
            int16_t sample = i < take ? local[i] : 0;
            stereo[i * 2] = stereo[i * 2 + 1] = sample;
        }
        size_t written = 0;
        esp_err_t result = i2s_channel_write(
            audio, stereo, sizeof(silence), &written, portMAX_DELAY);
        if (result != ESP_OK || written != sizeof(silence))
            printf("REMOTE_STREAM output_failed written=%u result=%s\n",
                   (unsigned)written, esp_err_to_name(result));
        else if (!take)
            ++remote_silence_writes;
    }
}

static void remote_fifo_clear(void)
{
    if (!remote_i2s_mutex) return;
    xSemaphoreTake(remote_i2s_mutex, portMAX_DELAY);
    remote_fifo_read = remote_fifo_write = remote_fifo_count = 0;
    remote_fifo_ready = false;
    xSemaphoreGive(remote_i2s_mutex);
}

static void speak(OldInst *voice, const char *text)
{
    remote_cancel = false;
    announcement_active = true;
    /* A press detected while speech is playing must not immediately replay
       the same announcement.  In particular, Wi-Fi startup can disturb an
       event queued by the ADC task before transfer mode is ready. */
    if (!is_remote_mode) xQueueReset(presses);
    if (!is_remote_mode) i2s_channel_enable(audio);
    else {
        size_t text_length = strlen(text);
        size_t prefill = REMOTE_PREFILL_PASSAGE_SAMPLES;
        if (text_length <= 12)
            prefill = REMOTE_PREFILL_CHARACTER_SAMPLES;
        else if (text_length <= 40)
            prefill = REMOTE_PREFILL_SHORT_SAMPLES;
        else if (text_length <= 120)
            prefill = REMOTE_PREFILL_SENTENCE_SAMPLES;
        else if (text_length <= 300)
            prefill = REMOTE_PREFILL_LONG_SAMPLES;
        xSemaphoreTake(remote_i2s_mutex, portMAX_DELAY);
        remote_fifo_prefill = prefill;
        remote_fifo_maximum = remote_fifo_count;
        xSemaphoreGive(remote_i2s_mutex);
        remote_stream_active = true;
    }
    TickType_t speech_started = xTaskGetTickCount();
    if (et_addText(voice, text) && et_synthesize(voice)) {
        TickType_t limit = xTaskGetTickCount() + pdMS_TO_TICKS(12000);
        while (eo_speaking(voice) && xTaskGetTickCount() < limit)
            vTaskDelay(pdMS_TO_TICKS(1));
        eo_synchronizeSynth(voice);
    }
    if (is_remote_mode) {
        remote_stream_active = false;
        xSemaphoreTake(remote_i2s_mutex, portMAX_DELAY);
        /* Short announcements may finish before reaching the normal jitter
           prefill.  Release whatever they produced once synthesis is done. */
        if (remote_fifo_count)
            remote_fifo_ready = true;
        size_t queued = remote_fifo_count;
        size_t maximum = remote_fifo_maximum;
        xSemaphoreGive(remote_i2s_mutex);
        printf("REMOTE_STREAM text=%u synth_ms=%u prefill_ms=%u queued_ms=%u maximum_ms=%u callbacks=%u underflows=%u cancel=%u\n",
               (unsigned)strlen(text),
               (unsigned)((xTaskGetTickCount() - speech_started)
                          * portTICK_PERIOD_MS),
               (unsigned)(remote_fifo_prefill * 1000 / 11025),
               (unsigned)(queued * 1000 / 11025),
               (unsigned)(maximum * 1000 / 11025),
               remote_stream_writes, remote_fifo_underflows,
               remote_cancel ? 1u : 0u);
        announcement_active = false;
        return;
    }
    static const int16_t silence[1024];
    size_t written;
    i2s_channel_write(audio, silence, sizeof(silence), &written, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2s_channel_disable(audio);
    if (!is_remote_mode) xQueueReset(presses);
    announcement_active = false;
}

static void remote_queue_copy(const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = heap_caps_malloc(length,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!copy) return;
    memcpy(copy, text, length);
    remote_speech_t item = { .text = copy };
    if (xQueueSend(remote_speech, &item, 0) != pdTRUE) free(copy);
}

static void remote_clear_queue(void)
{
    remote_speech_t item;
    while (xQueueReceive(remote_speech, &item, 0) == pdTRUE)
        free(item.text);
}

static void load_voice_preferences(OldInst *voice)
{
    nvs_handle_t prefs;
    if (nvs_open("reader", NVS_READONLY, &prefs) == ESP_OK) {
        int32_t value;
        if (nvs_get_i32(prefs, "rate", &value) == ESP_OK
            && value >= 60 && value <= 180)
            saved_rate = value;
        if (nvs_get_i32(prefs, "volume", &value) == ESP_OK
            && value >= 2 && value <= 100)
            saved_volume = value;
        nvs_close(prefs);
    }
    ESP_ERROR_CHECK(vc_setVoiceParam(voice, 0, VOICE_SPEED, saved_rate)
                    ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(vc_setVoiceParam(voice, 0, VOICE_VOLUME,
                                     ENGINE_VOLUME_MAX) >= 0
                    ? ESP_OK : ESP_FAIL);
    printf("NETWORK_VOICE rate=%d volume=%d engine_volume=%d\n",
           saved_rate, saved_volume, ENGINE_VOLUME_MAX);
}

static bool read_remote_config(char *host, size_t host_size, int *port,
                               char *key, size_t key_size)
{
    FILE *file = fopen("/sdcard/.evv/nvdaremote.ini", "rb");
    if (!file) return false;
    char line[384];
    while (fgets(line, sizeof(line), file)) {
        char *end = line + strlen(line);
        while (end > line && (end[-1] == '\r' || end[-1] == '\n'
                              || end[-1] == ' ' || end[-1] == '\t'))
            *--end = 0;
        if (!strncasecmp(line, "host=", 5))
            strlcpy(host, line + 5, host_size);
        else if (!strncasecmp(line, "port=", 5))
            *port = atoi(line + 5);
        else if (!strncasecmp(line, "key=", 4))
            strlcpy(key, line + 4, key_size);
    }
    fclose(file);
    return host[0] && key[0] && *port > 0 && *port <= 65535;
}

static void append_utf8(char *out, size_t capacity, size_t *used,
                        unsigned codepoint)
{
    unsigned char bytes[4]; size_t count = 0;
    if (codepoint < 0x80) bytes[count++] = codepoint;
    else if (codepoint < 0x800) {
        bytes[count++] = 0xc0 | (codepoint >> 6);
        bytes[count++] = 0x80 | (codepoint & 0x3f);
    } else {
        bytes[count++] = 0xe0 | (codepoint >> 12);
        bytes[count++] = 0x80 | ((codepoint >> 6) & 0x3f);
        bytes[count++] = 0x80 | (codepoint & 0x3f);
    }
    if (*used + count >= capacity) return;
    memcpy(out + *used, bytes, count); *used += count;
}

static size_t remote_decode_json_string(const char *start, const char *limit,
                                        char *out, size_t capacity,
                                        const char **after)
{
    size_t used = 0;
    const char *p = start;
    if (p >= limit || *p != '"') return 0;
    while (++p < limit && *p && *p != '"') {
            unsigned value = (unsigned char)*p;
            if (*p == '\\' && p[1]) {
                ++p;
                if (*p == 'n') value = '\n';
                else if (*p == 'r') value = '\r';
                else if (*p == 't') value = '\t';
                else if (*p == 'u') {
                    value = 0;
                    for (int i = 0; i < 4 && p[1]; ++i) {
                        char c = *++p;
                        value = value * 16 + (c >= '0' && c <= '9' ? c-'0'
                            : c >= 'a' && c <= 'f' ? c-'a'+10
                            : c >= 'A' && c <= 'F' ? c-'A'+10 : 0);
                    }
                    append_utf8(out, capacity, &used, value);
                    continue;
                } else value = (unsigned char)*p;
            }
            if (used + 1 < capacity) out[used++] = (char)value;
    }
    out[used] = 0;
    if (after) *after = p < limit ? p + 1 : p;
    return used;
}

static const char *remote_character_name(unsigned char character)
{
    static const char *const letters[] = {
        "ay", "bee", "see", "dee", "ee", "eff", "gee", "aitch",
        "eye", "jay", "kay", "ell", "em", "en", "oh", "pee",
        "cue", "are", "ess", "tee", "you", "vee", "double you",
        "ex", "why", "zed",
    };
    static const char *const digits[] = {
        "zero", "one", "two", "three", "four",
        "five", "six", "seven", "eight", "nine",
    };
    unsigned char lower = (unsigned char)tolower(character);
    if (lower >= 'a' && lower <= 'z') return letters[lower - 'a'];
    if (character >= '0' && character <= '9') return digits[character - '0'];
    switch (character) {
        case ' ': return "space"; case '.': return "dot";
        case ',': return "comma"; case ':': return "colon";
        case ';': return "semicolon"; case '!': return "exclamation";
        case '?': return "question mark"; case '-': return "dash";
        case '_': return "underscore"; case '/': return "slash";
        case '\\': return "backslash"; case '@': return "at";
        case '#': return "hash"; case '&': return "and";
        case '*': return "star"; case '+': return "plus";
        case '=': return "equals"; case '(': return "left parenthesis";
        case ')': return "right parenthesis"; case '[': return "left bracket";
        case ']': return "right bracket"; case '"': return "quote";
        case '\'': return "apostrophe"; default: return NULL;
    }
}

static void remote_append_sequence_text(char *out, size_t capacity,
                                        size_t *used, const char *value,
                                        size_t length, bool character_mode)
{
    const char *replacement = length == 1 && character_mode
        ? remote_character_name((unsigned char)value[0]) : NULL;
    if (replacement) { value = replacement; length = strlen(replacement); }
    if (*used && out[*used - 1] != ' ' && length
        && value[0] != ' ' && *used + 1 < capacity)
        out[(*used)++] = ' ';
    if (length > capacity - *used - 1)
        length = capacity - *used - 1;
    memcpy(out + *used, value, length);
    *used += length;
    out[*used] = 0;
}

static bool remote_is_command_string(const char *value)
{
    while (*value && isspace((unsigned char)*value)) ++value;
    static const char *const commands[] = {
        "CharacterModeCommand", "LangChangeCommand", "PitchCommand",
        "RateCommand", "VolumeCommand", "BreakCommand",
        "EndUtteranceCommand", "IndexCommand", "CancellableSpeech",
        "SpeechCommand", "BeepCommand", "WaveFileCommand",
    };
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
        if (!strncmp(value, commands[i], strlen(commands[i])))
            return true;
    return *value == '{' && strstr(value, "Command")
           && (strstr(value, "\"type\"") || strstr(value, "'type'"));
}

/* Preserve NVDA's character-mode intent and accept both the original
   sequence form (literal strings plus command objects) and the newer
   {type:"str", value:"..."} representation. */
static char *remote_extract_speech(const char *json)
{
    const char *sequence = strstr(json, "\"sequence\"");
    if (!sequence || !(sequence = strchr(sequence, '['))) return NULL;
    const char *limit = strrchr(sequence, ']');
    if (!limit) return NULL;
    /* Decoded text cannot be longer than its JSON representation.  Sizing
       both buffers from the received sequence avoids the former 1,023-byte
       utterance truncation and 511-byte single-string truncation. */
    size_t capacity = (size_t)(limit - sequence) + 1;
    char *out = heap_caps_malloc(capacity,
                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char *value = heap_caps_malloc(capacity,
                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!out || !value) {
        free(out);
        free(value);
        return NULL;
    }
    size_t used = 0;
    bool character_mode = false;
    for (const char *p = sequence + 1; p < limit && used + 1 < capacity;) {
        while (p < limit && (isspace((unsigned char)*p) || *p == ',')) ++p;
        if (p >= limit) break;
        if (*p == '"') {
            const char *after = p + 1;
            size_t length = remote_decode_json_string(
                p, limit, value, capacity, &after);
            if (!remote_is_command_string(value))
                remote_append_sequence_text(out, capacity, &used, value, length,
                                            character_mode);
            else
                printf("REMOTE_COMMAND_STRING dropped=%.*s\n", 64, value);
            p = after;
            continue;
        }
        if (*p == '{') {
            const char *end = strchr(p, '}');
            if (!end || end > limit) break;
            size_t object_length = (size_t)(end - p + 1);
            char object[512];
            if (object_length >= sizeof(object)) object_length = sizeof(object)-1;
            memcpy(object, p, object_length); object[object_length] = 0;
            if (strstr(object, "CharacterModeCommand"))
                character_mode = strstr(object, "\"state\": true")
                              || strstr(object, "\"state\":true");
            else if (strstr(object, "\"type\": \"str\"")
                     || strstr(object, "\"type\":\"str\"")) {
                const char *value_key = strstr(p, "\"value\"");
                if (value_key && value_key < end) {
                    const char *quote = strchr(value_key + 7, '"');
                    if (quote && quote < end) {
                        const char *after = quote + 1;
                        size_t length = remote_decode_json_string(
                            quote, end, value, capacity, &after);
                        remote_append_sequence_text(out, capacity, &used,
                            value, length, character_mode);
                    }
                }
            }
            p = end + 1;
            continue;
        }
        ++p;
    }
    while (used && (out[used-1] == ' ' || out[used-1] == '\n')) --used;
    out[used] = 0;
    free(value);
    if (!used) { free(out); return NULL; }
    return out;
}

static bool tls_write_all(esp_tls_t *tls, const char *text)
{
    size_t length = strlen(text), sent = 0;
    while (sent < length && !remote_stop) {
        ssize_t result = esp_tls_conn_write(tls, text + sent, length - sent);
        if (result > 0) sent += result;
        else if (result != ESP_TLS_ERR_SSL_WANT_READ
                 && result != ESP_TLS_ERR_SSL_WANT_WRITE) return false;
        else vTaskDelay(pdMS_TO_TICKS(1));
    }
    return sent == length;
}

static void remote_network_task(void *arg)
{
    (void)arg; remote_task_running = true;
    char host[256] = {0}, key[257] = {0}; int port = 6837;
    if (!read_remote_config(host, sizeof(host), &port, key, sizeof(key))) {
        puts("REMOTE_CONFIG invalid");
        remote_queue_copy("NVDA remote configuration invalid");
        remote_task_running = false; vTaskDeleteWithCaps(NULL); return;
    }
    /* Certificate validation needs a plausible wall clock. */
    if (time(NULL) < 1704067200 && !esp_sntp_enabled()) {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();
    }
    for (int i = 0; i < 50 && time(NULL) < 1704067200; ++i)
        vTaskDelay(pdMS_TO_TICKS(200));

    esp_tls_cfg_t cfg = {
        .timeout_ms = 3000,
        .non_block = true,
    };
    esp_tls_t *tls = esp_tls_init();
    if (!tls || esp_tls_conn_new_sync(host, strlen(host), port, &cfg, tls) != 1) {
        printf("REMOTE_CONNECT failed host=%s port=%d\n", host, port);
        remote_queue_copy("NVDA remote could not connect");
        if (tls) esp_tls_conn_destroy(tls);
        remote_task_running = false; vTaskDeleteWithCaps(NULL); return;
    }
    char join[640];
    snprintf(join, sizeof(join),
        "{\"version\":2,\"type\":\"protocol_version\"}\n"
        "{\"channel\":\"%s\",\"connection_type\":\"master\",\"type\":\"join\"}\n",
        key);
    if (!tls_write_all(tls, join)) remote_stop = true;
    puts("REMOTE_CONNECT handshake_sent");
    remote_queue_copy("NVDA remote connected. Hold centre to leave.");

    char received[8192]; size_t have = 0;
    while (!remote_stop) {
        ssize_t got = esp_tls_conn_read(tls, received + have,
                                        sizeof(received) - have - 1);
        if (got == ESP_TLS_ERR_SSL_WANT_READ
            || got == ESP_TLS_ERR_SSL_WANT_WRITE) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        if (got <= 0) break;
        have += got; received[have] = 0;
        char *start = received, *newline;
        while ((newline = memchr(start, '\n', received + have - start))) {
            *newline = 0;
            if (strstr(start, "\"type\": \"cancel\"")
                || strstr(start, "\"type\":\"cancel\"")) {
                remote_cancel = true;
                remote_stream_active = false;
                remote_fifo_clear();
                remote_clear_queue();
            } else if ((strstr(start, "\"type\": \"speak\"")
                      || strstr(start, "\"type\":\"speak\""))) {
                char *text = remote_extract_speech(start);
                if (text) {
                    remote_speech_t item = { .text = text };
                    if (xQueueSend(remote_speech, &item, 0) != pdTRUE)
                        free(text);
                }
            }
            start = newline + 1;
        }
        size_t remaining = received + have - start;
        memmove(received, start, remaining); have = remaining;
        if (have == sizeof(received) - 1) have = 0;
    }
    esp_tls_conn_destroy(tls);
    puts("REMOTE_CONNECT disconnected");
    remote_task_running = false;
    vTaskDeleteWithCaps(NULL);
}

static int key_for_mv(int mv)
{
    static const int centre[] = { 0, 0, 660, 1320, 1980, 2640 };
    int key = 0, distance = 220;
    if (mv >= 2700) return 0;
    for (int i = 1; i <= 5; ++i) {
        int d = abs(mv - centre[i]);
        if (d < distance) { distance = d; key = i; }
    }
    return key;
}

static void scan_buttons(void *arg)
{
    buttons_t *buttons = arg;
    int candidate = 0, count = 0, stable = 0;
    TickType_t key_started = 0;
    bool long_handled = false;
    for (;;) {
        int raw = 0, mv = 3300;
        if (adc_oneshot_read(buttons->adc, ADC_CHANNEL_8, &raw) == ESP_OK)
            adc_cali_raw_to_voltage(buttons->calibration, raw, &mv);
        int key = key_for_mv(mv);
        if (key == candidate) ++count;
        else { candidate = key; count = 1; }
        if (count >= 3 && key != stable) {
            int previous = stable;
            stable = key;
            if (key) {
                key_started = xTaskGetTickCount();
                long_handled = false;
            } else if (transfer_ready && previous && !long_handled) {
                xQueueSend(presses, &previous, 0);
            }
        }
        if (transfer_ready && stable && !long_handled
            && xTaskGetTickCount() - key_started
                   >= pdMS_TO_TICKS(1200)) {
            long_handled = true;
            int event = 0x100 | stable;
            xQueueSend(presses, &event, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void start_button_scanner(buttons_t *buttons)
{
    adc_oneshot_unit_init_cfg_t unit = {
        .unit_id = ADC_UNIT_2, .ulp_mode = ADC_ULP_MODE_DISABLE };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit, &buttons->adc));
    adc_oneshot_chan_cfg_t channel = {
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(buttons->adc, ADC_CHANNEL_8,
                                                &channel));
    adc_cali_curve_fitting_config_t cal = {
        .unit_id = ADC_UNIT_2, .chan = ADC_CHANNEL_8,
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
        &cal, &buttons->calibration));
    presses = xQueueCreate(8, sizeof(int));
    ESP_ERROR_CHECK(presses ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(scan_buttons, "transfer_buttons", 4096,
                                buttons, 5, NULL) == pdPASS
                    ? ESP_OK : ESP_ERR_NO_MEM);
}

void app_main(void)
{
    uint32_t boot_request = REG_READ(RTC_CNTL_STORE0_REG);
    if (boot_request == BOOT_REQUEST_READER) {
        REG_WRITE(RTC_CNTL_STORE0_REG, 0);
        const esp_partition_t *reader = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY, "reader");
        esp_err_t result = reader
            ? esp_ota_set_boot_partition(reader) : ESP_ERR_NOT_FOUND;
        printf("READER_BOOT_SELECT result=%s\n", esp_err_to_name(result));
        if (result == ESP_OK)
            esp_restart();
    } else if (boot_request == BOOT_REQUEST_TRANSFER) {
        /* Consume the one-shot request.  If this network application later
           resets or loses power, its next boot has no request and returns to
           the reader instead of trapping the user in NVDA/file transfer. */
        REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    } else {
        const esp_partition_t *reader = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY, "reader");
        esp_err_t result = reader
            ? esp_ota_set_boot_partition(reader) : ESP_ERR_NOT_FOUND;
        printf("NETWORK_RESET_RETURN result=%s\n", esp_err_to_name(result));
        if (result == ESP_OK)
            esp_restart();
    }
    /* RTC scratch registers are cleared during this board's staged OTA boot.
       Use a consumed-on-read NVS value for the second RemSound restart. */
    initialise_nvs();
    bool remsound_handoff = take_remsound_handoff();
    uint32_t mode_hint = remsound_handoff
        ? 3 : REG_READ(RTC_CNTL_STORE1_REG);
    printf("NETWORK_BOOT mode_hint=%lu remsound_handoff=%u\n",
           (unsigned long)mode_hint, remsound_handoff);
    is_remote_mode = mode_hint == 1;
    if (mode_hint == 3) {
        heap_caps_malloc_extmem_enable(1024);
        buttons_t buttons = { 0 };
        start_button_scanner(&buttons);
        transfer_mode_status_t status = { 0 };
        esp_err_t result = transfer_network_start(
            TRANSFER_WIFI_SSID, TRANSFER_WIFI_PASSWORD, &status);
        if (result == ESP_OK && status.remsound_mode) {
            transfer_ready = true;
            result = remsound_receiver_run(presses, status.ip_address);
            transfer_ready = false;
            if (result != ESP_OK) {
                ESP_LOGE("remsound", "receiver stopped during startup: %s",
                         esp_err_to_name(result));
                /* Let queued diagnostic audio reach the codec before the
                   partition handoff resets the I2S peripheral. */
                vTaskDelay(pdMS_TO_TICKS(3000));
            } else {
                ESP_LOGI("remsound", "receiver stopped by user");
            }
        } else {
            ESP_LOGE("remsound", "network startup failed: result=%s mode=%u",
                     esp_err_to_name(result), status.remsound_mode);
            remsound_receiver_announce_wifi_error();
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
        transfer_mode_stop();
        REG_WRITE(RTC_CNTL_STORE1_REG, 0);
        REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_READER);
        esp_restart();
    }
    /* Keep the speech buffers and medium-sized engine allocations out of the
       limited internal SRAM; Wi-Fi needs that SRAM for its control objects. */
    heap_caps_malloc_extmem_enable(1024);
    mono = heap_caps_malloc(FRAME_SAMPLES * sizeof(*mono),
                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    stereo = heap_caps_malloc(FRAME_SAMPLES * 2 * sizeof(*stereo),
                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(mono && stereo ? ESP_OK : ESP_ERR_NO_MEM);

    i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,
                                                        I2S_ROLE_MASTER);
    /* Keep only a short hardware runway.  Remote speech is written directly
       from OpenEVV's callback, so a large silence-filled DMA queue would add
       avoidable latency before its first sample. */
    chan.dma_desc_num = 4;
    chan.dma_frame_num = 128;
    ESP_ERROR_CHECK(i2s_new_channel(&chan, &audio, NULL));
    i2s_std_config_t std = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(11025),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = { .mclk = I2S_GPIO_UNUSED, .bclk = GPIO_NUM_42,
                      .ws = GPIO_NUM_41, .dout = GPIO_NUM_1,
                      .din = I2S_GPIO_UNUSED },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(audio, &std));
    evv_port_start();
    evvRunStaticInitialisers();
    OldInst *voice = eo_new();
    ESP_ERROR_CHECK(voice ? ESP_OK : ESP_ERR_NO_MEM);
    eo_registerCallback(voice, waveform, NULL);
    ESP_ERROR_CHECK(ev_setOutputBuffer(voice, FRAME_SAMPLES, mono)
                    ? ESP_OK : ESP_FAIL);

    /* OpenEVV creates several task stacks in PSRAM but its RTOS control
       objects still require scarce internal RAM.  Initialise NVS only after
       the voice so its page cache cannot starve voice construction. */
    load_voice_preferences(voice);

    buttons_t buttons = { 0 };
    start_button_scanner(&buttons);
    remote_speech = xQueueCreate(8, sizeof(remote_speech_t));
    ESP_ERROR_CHECK(remote_speech ? ESP_OK : ESP_ERR_NO_MEM);

    transfer_mode_status_t status = { 0 };
    esp_err_t result = is_remote_mode
        ? transfer_network_start(TRANSFER_WIFI_SSID,
                                 TRANSFER_WIFI_PASSWORD, &status)
        : transfer_mode_start(TRANSFER_WIFI_SSID,
                              TRANSFER_WIFI_PASSWORD, &status);
    /* The RTC mode hint may be lost during the reader's staged OTA handoff.
       If the SD-card marker resolved to RemSound only after OpenEVV was
       initialized, restart once with a fresh one-shot request so the early
       path above can enter without carrying the synthesizer in runtime RAM. */
    if (status.remsound_mode) {
        transfer_mode_stop();
        ESP_ERROR_CHECK(save_remsound_handoff());
        REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_TRANSFER);
        esp_restart();
    }
    if (status.clock_mode) {
        time_t before = time(NULL);
        for (int i = 0; i < 300 && time(NULL) < 1704067200; ++i)
            vTaskDelay(pdMS_TO_TICKS(100));
        time_t after = time(NULL);
        bool synced = after >= 1704067200;
        printf("CLOCK_SYNC result=%s synced=%d before=%lld after=%lld\n",
               esp_err_to_name(result), synced,
               (long long)before, (long long)after);
        speak(voice, synced ? "clock set" : "clock could not be set");
        vTaskDelay(pdMS_TO_TICKS(600));
        transfer_mode_stop();
        REG_WRITE(RTC_CNTL_STORE1_REG, 0);
        REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_READER);
        esp_restart();
    }
    is_remote_mode = status.remote_mode;
    /* The RTC hint can be lost during the reader's staged partition switch;
       network.mode on the SD card is authoritative.  Therefore initialise
       the continuous remote output path only after transfer_mode has resolved
       the requested mode, but before any remote success/error announcement. */
    if (is_remote_mode) {
        remote_fifo = heap_caps_malloc(REMOTE_FIFO_SAMPLES
                                       * sizeof(*remote_fifo),
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        remote_i2s_mutex = xSemaphoreCreateMutex();
        ESP_ERROR_CHECK(remote_fifo && remote_i2s_mutex
                        ? ESP_OK : ESP_ERR_NO_MEM);
        BaseType_t playback_created = xTaskCreatePinnedToCoreWithCaps(
            remote_playback_task, "remote_stream", 8192, NULL, 6, NULL, 1,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        ESP_ERROR_CHECK(playback_created == pdPASS
                        ? ESP_OK : ESP_ERR_NO_MEM);
    }
    if (result != ESP_OK) {
        speak(voice, status.clock_mode ? "clock could not be set"
                    : is_remote_mode ? "NVDA remote could not start"
                    : "file transfer could not start");
        for (;;) vTaskDelay(portMAX_DELAY);
    }
    if (is_remote_mode) {
        speak(voice, "NVDA remote connecting");
        BaseType_t created = xTaskCreateWithCaps(
            remote_network_task, "nvda_remote", 16384, NULL, 4, NULL,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        ESP_ERROR_CHECK(created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
        transfer_ready = true;
        for (;;) {
            remote_speech_t item;
            if (xQueueReceive(remote_speech, &item, pdMS_TO_TICKS(1))
                    == pdTRUE) {
                speak(voice, item.text);
                free(item.text);
            }
            int key;
            if (xQueueReceive(presses, &key, 0) == pdTRUE && key == 0x101) {
                transfer_ready = false;
                remote_stop = true;
                remote_cancel = true;
                remote_stream_active = false;
                remote_fifo_clear();
                speak(voice, "leaving NVDA remote");
                TickType_t deadline = xTaskGetTickCount()
                    + pdMS_TO_TICKS(4000);
                while (remote_task_running
                       && xTaskGetTickCount() < deadline)
                    vTaskDelay(pdMS_TO_TICKS(20));
                transfer_mode_stop();
                REG_WRITE(RTC_CNTL_STORE1_REG, 0);
                REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_READER);
                esp_restart();
            }
        }
    }
    char announcement[128];
    snprintf(announcement, sizeof(announcement),
             "file transfer ready. device %s. I P address %s",
             status.hostname, status.ip_address);
    speak(voice, announcement);
    transfer_ready = true;
    for (;;) {
        int key;
        if (xQueueReceive(presses, &key, portMAX_DELAY) == pdTRUE
            && key == 0x101) {
            transfer_ready = false;
            speak(voice, "leaving file transfer");
            transfer_mode_stop();
            REG_WRITE(RTC_CNTL_STORE1_REG, 0);
            REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_READER);
            esp_restart();
        }
    }
}
