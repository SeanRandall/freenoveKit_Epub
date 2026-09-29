#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define VOICE_SPEED 6
#define VOICE_VOLUME 7
#define ENGINE_VOLUME_MAX 80
#define REMOTE_PCM_SAMPLES 65536
#define REMOTE_PLAY_SAMPLES 1024
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
static int16_t *remote_pcm;
static SemaphoreHandle_t remote_pcm_mutex;
static volatile bool remote_pcm_session;
static volatile bool remote_pcm_started;
static volatile bool remote_pcm_synthesis_done;
static size_t remote_pcm_read;
static size_t remote_pcm_write;
static size_t remote_pcm_count;
static size_t remote_pcm_prefill;
static unsigned remote_pcm_underflows;

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
    if (is_remote_mode && remote_pcm_mutex) {
        size_t offset = 0;
        while (offset < count && !remote_cancel) {
            xSemaphoreTake(remote_pcm_mutex, portMAX_DELAY);
            size_t space = REMOTE_PCM_SAMPLES - remote_pcm_count;
            size_t copy = count - offset;
            if (copy > space) copy = space;
            size_t tail = REMOTE_PCM_SAMPLES - remote_pcm_write;
            if (copy > tail) copy = tail;
            for (size_t i = 0; i < copy; ++i)
                remote_pcm[remote_pcm_write + i] =
                    (int16_t)(((int32_t)mono[offset + i]
                               * saved_volume) / 100);
            remote_pcm_write = (remote_pcm_write + copy)
                               % REMOTE_PCM_SAMPLES;
            remote_pcm_count += copy;
            if (!remote_pcm_started
                && remote_pcm_count >= remote_pcm_prefill)
                remote_pcm_started = true;
            xSemaphoreGive(remote_pcm_mutex);
            offset += copy;
            if (!copy) vTaskDelay(pdMS_TO_TICKS(2));
        }
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

static void remote_pcm_begin(size_t text_length)
{
    xSemaphoreTake(remote_pcm_mutex, portMAX_DELAY);
    remote_pcm_read = remote_pcm_write = remote_pcm_count = 0;
    remote_pcm_synthesis_done = false;
    remote_pcm_started = false;
    remote_pcm_prefill = text_length <= 24 ? 1
                       : text_length <= 80 ? 2048 : 8192;
    remote_pcm_session = true;
    xSemaphoreGive(remote_pcm_mutex);
}

static void remote_pcm_cancel(void)
{
    if (!remote_pcm_mutex) return;
    xSemaphoreTake(remote_pcm_mutex, portMAX_DELAY);
    remote_pcm_read = remote_pcm_write = remote_pcm_count = 0;
    remote_pcm_synthesis_done = true;
    remote_pcm_started = true;
    remote_pcm_session = false;
    xSemaphoreGive(remote_pcm_mutex);
}

static void remote_playback_task(void *arg)
{
    (void)arg;
    int16_t local[REMOTE_PLAY_SAMPLES];
    i2s_channel_enable(audio);
    for (;;) {
        size_t take = 0;
        bool session, started, done;
        xSemaphoreTake(remote_pcm_mutex, portMAX_DELAY);
        session = remote_pcm_session;
        started = remote_pcm_started;
        done = remote_pcm_synthesis_done;
        if (session && !started && done) {
            remote_pcm_started = started = true;
        }
        if (session && started && remote_pcm_count) {
            take = remote_pcm_count;
            if (take > REMOTE_PLAY_SAMPLES) take = REMOTE_PLAY_SAMPLES;
            size_t tail = REMOTE_PCM_SAMPLES - remote_pcm_read;
            if (take > tail) take = tail;
            memcpy(local, remote_pcm + remote_pcm_read,
                   take * sizeof(*local));
            remote_pcm_read = (remote_pcm_read + take)
                              % REMOTE_PCM_SAMPLES;
            remote_pcm_count -= take;
        } else if (session && started && done && !remote_pcm_count) {
            remote_pcm_session = false;
            session = false;
        } else if (session && started && !done && !remote_pcm_count) {
            ++remote_pcm_underflows;
            remote_pcm_started = started = false;
            remote_pcm_prefill = 4096;
        }
        xSemaphoreGive(remote_pcm_mutex);
        if (take) {
            for (size_t i = 0; i < take; ++i)
                stereo[i * 2] = stereo[i * 2 + 1] = local[i];
            size_t written;
            if (i2s_channel_write(audio, stereo, take * 4, &written,
                                  portMAX_DELAY) != ESP_OK)
                puts("REMOTE_PCM i2s_write_failed");
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

static void speak(OldInst *voice, const char *text)
{
    remote_cancel = false;
    announcement_active = true;
    /* A press detected while speech is playing must not immediately replay
       the same announcement.  In particular, Wi-Fi startup can disturb an
       event queued by the ADC task before transfer mode is ready. */
    if (!is_remote_mode) xQueueReset(presses);
    if (is_remote_mode) remote_pcm_begin(strlen(text));
    else i2s_channel_enable(audio);
    if (et_addText(voice, text) && et_synthesize(voice)) {
        TickType_t limit = xTaskGetTickCount() + pdMS_TO_TICKS(12000);
        while (eo_speaking(voice) && xTaskGetTickCount() < limit)
            vTaskDelay(pdMS_TO_TICKS(5));
        eo_synchronizeSynth(voice);
    }
    if (is_remote_mode) {
        xSemaphoreTake(remote_pcm_mutex, portMAX_DELAY);
        remote_pcm_synthesis_done = true;
        if (!remote_pcm_started) remote_pcm_started = true;
        xSemaphoreGive(remote_pcm_mutex);
        TickType_t drain_limit = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
        while (remote_pcm_session && !remote_cancel
               && xTaskGetTickCount() < drain_limit)
            vTaskDelay(pdMS_TO_TICKS(2));
        printf("REMOTE_PCM text=%u prefill=%u underflows=%u\n",
               (unsigned)strlen(text), (unsigned)remote_pcm_prefill,
               remote_pcm_underflows);
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

/* Extract only top-level strings from NVDA's abstract speech sequence.
   Command objects are deliberately ignored in this first listening build. */
static char *remote_extract_speech(const char *json)
{
    const char *sequence = strstr(json, "\"sequence\"");
    if (!sequence || !(sequence = strchr(sequence, '['))) return NULL;
    char *out = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!out) return NULL;
    size_t used = 0; int object_depth = 0;
    for (const char *p = sequence + 1; *p && used < 1023; ++p) {
        if (*p == ']') break;
        if (*p == '{') { ++object_depth; continue; }
        if (*p == '}') { if (object_depth) --object_depth; continue; }
        if (*p != '"') continue;
        bool keep = object_depth == 0;
        while (*++p && *p != '"') {
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
                    if (keep) append_utf8(out, 1024, &used, value);
                    continue;
                } else value = (unsigned char)*p;
            }
            if (keep && used < 1023) out[used++] = (char)value;
        }
    }
    while (used && (out[used-1] == ' ' || out[used-1] == '\n')) --used;
    out[used] = 0;
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
        else vTaskDelay(pdMS_TO_TICKS(5));
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
    remote_queue_copy("NVDA remote connected");

    char received[8192]; size_t have = 0;
    while (!remote_stop) {
        ssize_t got = esp_tls_conn_read(tls, received + have,
                                        sizeof(received) - have - 1);
        if (got == ESP_TLS_ERR_SSL_WANT_READ
            || got == ESP_TLS_ERR_SSL_WANT_WRITE) {
            vTaskDelay(pdMS_TO_TICKS(5));
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
                remote_pcm_cancel();
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
    TickType_t centre_started = 0;
    bool centre_handled = false;
    for (;;) {
        int raw = 0, mv = 3300;
        if (adc_oneshot_read(buttons->adc, ADC_CHANNEL_8, &raw) == ESP_OK)
            adc_cali_raw_to_voltage(buttons->calibration, raw, &mv);
        int key = key_for_mv(mv);
        if (key == candidate) ++count;
        else { candidate = key; count = 1; }
        if (count >= 3 && key != stable) {
            stable = key;
            if (key == 1) {
                centre_started = xTaskGetTickCount();
                centre_handled = false;
            }
        }
        if (transfer_ready && stable == 1 && !centre_handled
            && xTaskGetTickCount() - centre_started
                   >= pdMS_TO_TICKS(1200)) {
            centre_handled = true;
            int exit_request = 1;
            xQueueOverwrite(presses, &exit_request);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void)
{
    if (REG_READ(RTC_CNTL_STORE0_REG) == BOOT_REQUEST_READER) {
        REG_WRITE(RTC_CNTL_STORE0_REG, 0);
        const esp_partition_t *reader = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY, "reader");
        esp_err_t result = reader
            ? esp_ota_set_boot_partition(reader) : ESP_ERR_NOT_FOUND;
        printf("READER_BOOT_SELECT result=%s\n", esp_err_to_name(result));
        if (result == ESP_OK)
            esp_restart();
    }
    is_remote_mode = REG_READ(RTC_CNTL_STORE1_REG) == 1;
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
    esp_err_t nvs = nvs_flash_init();
    if (nvs == ESP_ERR_NVS_NO_FREE_PAGES || nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else ESP_ERROR_CHECK(nvs);
    load_voice_preferences(voice);

    buttons_t buttons = { 0 };
    adc_oneshot_unit_init_cfg_t unit = {
        .unit_id = ADC_UNIT_2, .ulp_mode = ADC_ULP_MODE_DISABLE };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit, &buttons.adc));
    adc_oneshot_chan_cfg_t channel = {
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(buttons.adc, ADC_CHANNEL_8,
                                                &channel));
    adc_cali_curve_fitting_config_t cal = {
        .unit_id = ADC_UNIT_2, .chan = ADC_CHANNEL_8,
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cal,
                                                          &buttons.calibration));
    presses = xQueueCreate(1, sizeof(int));
    ESP_ERROR_CHECK(presses ? ESP_OK : ESP_ERR_NO_MEM);
    remote_speech = xQueueCreate(8, sizeof(remote_speech_t));
    ESP_ERROR_CHECK(remote_speech ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(scan_buttons, "transfer_buttons", 4096,
                                &buttons, 5, NULL) == pdPASS
                    ? ESP_OK : ESP_ERR_NO_MEM);

    transfer_mode_status_t status = { 0 };
    esp_err_t result = is_remote_mode
        ? transfer_network_start(TRANSFER_WIFI_SSID,
                                 TRANSFER_WIFI_PASSWORD, &status)
        : transfer_mode_start(TRANSFER_WIFI_SSID,
                              TRANSFER_WIFI_PASSWORD, &status);
    is_remote_mode = status.remote_mode;
    if (result != ESP_OK) {
        speak(voice, is_remote_mode ? "NVDA remote could not start"
                                    : "file transfer could not start");
        for (;;) vTaskDelay(portMAX_DELAY);
    }
    if (is_remote_mode) {
        remote_pcm = heap_caps_malloc(REMOTE_PCM_SAMPLES
                                      * sizeof(*remote_pcm),
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        remote_pcm_mutex = xSemaphoreCreateMutex();
        ESP_ERROR_CHECK(remote_pcm && remote_pcm_mutex
                        ? ESP_OK : ESP_ERR_NO_MEM);
        BaseType_t playback_created = xTaskCreatePinnedToCoreWithCaps(
            remote_playback_task, "remote_pcm", 4096, NULL, 6, NULL, 1,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        ESP_ERROR_CHECK(playback_created == pdPASS
                        ? ESP_OK : ESP_ERR_NO_MEM);
        speak(voice, "NVDA remote connecting");
        BaseType_t created = xTaskCreateWithCaps(
            remote_network_task, "nvda_remote", 16384, NULL, 4, NULL,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        ESP_ERROR_CHECK(created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
        transfer_ready = true;
        for (;;) {
            int key;
            if (xQueueReceive(presses, &key, pdMS_TO_TICKS(10)) == pdTRUE) {
                transfer_ready = false;
                remote_stop = true;
                remote_cancel = true;
                remote_pcm_cancel();
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
            remote_speech_t item;
            if (xQueueReceive(remote_speech, &item, 0) == pdTRUE) {
                speak(voice, item.text);
                free(item.text);
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
        if (xQueueReceive(presses, &key, portMAX_DELAY) == pdTRUE) {
            transfer_ready = false;
            speak(voice, "leaving file transfer");
            transfer_mode_stop();
            REG_WRITE(RTC_CNTL_STORE1_REG, 0);
            REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_READER);
            esp_restart();
        }
    }
}
