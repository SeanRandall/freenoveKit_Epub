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
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "evv_abi.h"

typedef struct OldInst OldInst;

enum ECIMessage { eciWaveformBuffer, eciPhonemeBuffer, eciIndexReply };
enum ECICallbackReturn { eciDataNotProcessed, eciDataProcessed, eciDataAbort };

OldInst *STDCALL eo_new(void);
int STDCALL es_delete(OldInst *h);
int STDCALL es_pause(OldInst *h, int32_t on);
int STDCALL et_addText(OldInst *h, const char *text);
int STDCALL et_synthesize(OldInst *h);
int STDCALL ev_setOutputBuffer(OldInst *h, int32_t n, void *buf);
int STDCALL vc_setVoiceParam(OldInst *h, int32_t voice, int32_t which,
                             int32_t value);
void STDCALL eo_registerCallback(OldInst *h, void *cb, void *data);
void STDCALL eo_synchronizeSynth(OldInst *h);
int STDCALL eo_speaking(OldInst *h);
void evvRunStaticInitialisers(void);
void evv_port_start(void);

#define SAMPLE_RATE 11025
#define FRAME_SAMPLES 2048
#define VOICE_SPEED 6
#define I2S_BCLK GPIO_NUM_42
#define I2S_LRCLK GPIO_NUM_41
#define I2S_DATA GPIO_NUM_1
#define BUTTON_ADC_UNIT ADC_UNIT_2
#define BUTTON_ADC_CHANNEL ADC_CHANNEL_8

static const char *const TAG = "paragraph_reader";
static const char *const paragraph_chunks[] = {
    "This is a test.",
    "A reader should respond to a small number of controls and explain every change clearly.",
    "It should let the listener concentrate on the words rather than the machine.",
    "This tiny computer is demonstrating that idea using only five directions and a loudspeaker.",
    "The centre control starts and pauses this paragraph.",
    "Left and right change the listening volume, while up and down change the speaking rate.",
    "While reading continues, each change takes effect without an announcement.",
    "It is only an early hardware experiment, but the same simple interface can grow.",
    "It could become a pocket sized talking library.",
    "That library can remember books, bookmarks, preferences, and the listener's exact place.",
    "Good accessibility is not an extra layer added at the end.",
    "It is a design decision made from the very first button press.",
};
#define PARAGRAPH_CHUNK_COUNT \
    (sizeof(paragraph_chunks) / sizeof(paragraph_chunks[0]))

static int16_t mono_frame[FRAME_SAMPLES];
static int16_t stereo_frame[FRAME_SAMPLES * 2];
static i2s_chan_handle_t tx_channel;
static volatile int listening_volume = 70;
static volatile int speech_rate = 105;
static volatile size_t emitted_samples;
static bool channel_enabled;
static size_t paragraph_chunk;
static bool capture_mode;
static int16_t *capture_target;
static size_t capture_capacity;
static size_t capture_used;

typedef struct {
    int16_t *samples;
    size_t count;
} SpeechClip;

#define VOLUME_CLIP_COUNT 10
#define RATE_CLIP_COUNT 9
#define CLIP_CAPACITY_SAMPLES 49152
static SpeechClip volume_clips[VOLUME_CLIP_COUNT];
static SpeechClip rate_clips[RATE_CLIP_COUNT];

enum PlaybackState { STATE_READY, STATE_PLAYING, STATE_PAUSED, STATE_FINISHED };
static volatile enum PlaybackState playback_state = STATE_READY;

typedef struct {
    adc_oneshot_unit_handle_t adc;
    adc_cali_handle_t calibration;
} ButtonContext;

static QueueHandle_t button_queue;

static enum ECICallbackReturn STDCALL on_message(OldInst *instance,
                                                  enum ECIMessage message,
                                                  long parameter,
                                                  void *context)
{
    (void)instance;
    (void)context;
    if (message != eciWaveformBuffer)
        return eciDataProcessed;

    size_t count = (size_t)parameter;
    if (emitted_samples == 0)
        printf("WAVEFORM first_count=%u capture=%d\n", (unsigned)count,
               capture_mode);
    if (capture_mode) {
        size_t room = capture_capacity - capture_used;
        size_t copy = count < room ? count : room;
        if (copy > 0) {
            memcpy(capture_target + capture_used, mono_frame,
                   copy * sizeof(int16_t));
            capture_used += copy;
        }
        return copy == count ? eciDataProcessed : eciDataAbort;
    }

    int volume = listening_volume;
    for (size_t i = 0; i < count; ++i) {
        int32_t scaled = ((int32_t)mono_frame[i] * volume) / 100;
        stereo_frame[i * 2] = (int16_t)scaled;
        stereo_frame[i * 2 + 1] = (int16_t)scaled;
    }

    size_t bytes_written = 0;
    size_t wanted = count * 2 * sizeof(int16_t);
    esp_err_t err = i2s_channel_write(tx_channel, stereo_frame, wanted,
                                      &bytes_written, portMAX_DELAY);
    if (err != ESP_OK || bytes_written != wanted) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(err));
        return eciDataAbort;
    }
    emitted_samples += count;
    return eciDataProcessed;
}

static esp_err_t init_audio(void)
{
    i2s_chan_config_t channel_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    channel_cfg.dma_desc_num = 8;
    channel_cfg.dma_frame_num = 256;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel_cfg, &tx_channel, NULL),
                        TAG, "create I2S channel");

    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK,
            .ws = I2S_LRCLK,
            .dout = I2S_DATA,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = { false, false, false },
        },
    };
    return i2s_channel_init_std_mode(tx_channel, &config);
}

static bool start_channel(void)
{
    if (channel_enabled)
        return true;
    if (i2s_channel_enable(tx_channel) != ESP_OK)
        return false;
    channel_enabled = true;
    return true;
}

static void silence_and_stop_channel(void)
{
    static const int16_t silence[FRAME_SAMPLES * 2];
    if (!channel_enabled)
        return;
    size_t written = 0;
    (void)i2s_channel_write(tx_channel, silence, sizeof(silence), &written,
                            portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(220));
    (void)i2s_channel_disable(tx_channel);
    channel_enabled = false;
}

static bool speak_once(OldInst *instance, const char *text)
{
    if (!start_channel())
        return false;
    emitted_samples = 0;
    printf("ANNOUNCE %s\n", text);
    if (!et_addText(instance, text) || !et_synthesize(instance)) {
        silence_and_stop_channel();
        return false;
    }
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
    while (xTaskGetTickCount() < deadline) {
        if (emitted_samples > 0 && !eo_speaking(instance))
            break;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    eo_synchronizeSynth(instance);
    silence_and_stop_channel();
    return emitted_samples > 0;
}

static bool render_clip(OldInst *instance, SpeechClip *clip, const char *text)
{
    clip->samples = heap_caps_malloc(CLIP_CAPACITY_SAMPLES * sizeof(int16_t),
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (clip->samples == NULL)
        return false;

    capture_target = clip->samples;
    capture_capacity = CLIP_CAPACITY_SAMPLES;
    capture_used = 0;
    capture_mode = true;
    bool submitted = et_addText(instance, text) && et_synthesize(instance);
    if (submitted) {
        TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
        while (xTaskGetTickCount() < deadline) {
            if (capture_used > 0 && !eo_speaking(instance))
                break;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        eo_synchronizeSynth(instance);
    }
    capture_mode = false;
    clip->count = capture_used;
    printf("CLIP text='%s' samples=%u\n", text, (unsigned)clip->count);
    return submitted && clip->count > 0 && clip->count < capture_capacity;
}

static bool play_clip(const SpeechClip *clip)
{
    if (clip->samples == NULL || clip->count == 0 || !start_channel())
        return false;
    for (size_t at = 0; at < clip->count;) {
        size_t count = clip->count - at;
        if (count > FRAME_SAMPLES)
            count = FRAME_SAMPLES;
        int volume = listening_volume;
        for (size_t i = 0; i < count; ++i) {
            int32_t scaled = ((int32_t)clip->samples[at + i] * volume) / 100;
            stereo_frame[i * 2] = (int16_t)scaled;
            stereo_frame[i * 2 + 1] = (int16_t)scaled;
        }
        size_t written = 0;
        if (i2s_channel_write(tx_channel, stereo_frame,
                              count * 2 * sizeof(int16_t), &written,
                              portMAX_DELAY) != ESP_OK)
            break;
        at += count;
    }
    silence_and_stop_channel();
    return true;
}

static bool render_setting_clips(OldInst *instance)
{
    char text[40];
    for (int i = 0; i < VOLUME_CLIP_COUNT; ++i) {
        snprintf(text, sizeof(text), "volume %d percent", (i + 1) * 10);
        if (!render_clip(instance, &volume_clips[i], text))
            return false;
    }
    for (int i = 0; i < RATE_CLIP_COUNT; ++i) {
        snprintf(text, sizeof(text), "speech rate %d", 60 + i * 15);
        if (!render_clip(instance, &rate_clips[i], text))
            return false;
    }
    return true;
}

static int classify_button_mv(int millivolts)
{
    static const int centres_mv[] = { 0, 0, 660, 1320, 1980, 2640 };
    int best_key = 0;
    int best_distance = 220;
    if (millivolts >= 2700)
        return 0;
    for (int key = 1; key <= 5; ++key) {
        int distance = abs(millivolts - centres_mv[key]);
        if (distance < best_distance) {
            best_distance = distance;
            best_key = key;
        }
    }
    return best_key;
}

static void queue_latest_key(int key)
{
    xQueueOverwrite(button_queue, &key);
}

static void button_scan_task(void *argument)
{
    const ButtonContext *buttons = argument;
    int stable_key = 0;
    int candidate_key = 0;
    int candidate_count = 0;

    for (;;) {
        int raw = 0;
        int millivolts = 3300;
        if (adc_oneshot_read(buttons->adc, BUTTON_ADC_CHANNEL, &raw) == ESP_OK)
            (void)adc_cali_raw_to_voltage(buttons->calibration, raw,
                                          &millivolts);
        int key = classify_button_mv(millivolts);
        if (key == candidate_key)
            ++candidate_count;
        else {
            candidate_key = key;
            candidate_count = 1;
        }
        if (candidate_count >= 3 && candidate_key != stable_key) {
            stable_key = candidate_key;
            if (stable_key != 0) {
                printf("BUTTON key=%d mv=%d\n", stable_key, millivolts);
                queue_latest_key(stable_key);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static OldInst *new_voice(void)
{
    OldInst *instance = eo_new();
    if (instance == NULL)
        return NULL;
    eo_registerCallback(instance, on_message, NULL);
    if (!ev_setOutputBuffer(instance, FRAME_SAMPLES, mono_frame))
        return NULL;
    return instance;
}

static void announce_setting(bool volume)
{
    if (volume) {
        int index = listening_volume / 10 - 1;
        (void)play_clip(&volume_clips[index]);
    } else {
        int index = (speech_rate - 60) / 15;
        (void)play_clip(&rate_clips[index]);
    }
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc;
    adc_cali_handle_t calibration = NULL;
    ESP_ERROR_CHECK(init_audio());

    adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
        .unit_id = BUTTON_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc));
    adc_oneshot_chan_cfg_t adc_channel_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc, BUTTON_ADC_CHANNEL, &adc_channel_cfg));
    adc_cali_curve_fitting_config_t cal_cfg = {
        .unit_id = BUTTON_ADC_UNIT,
        .chan = BUTTON_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cal_cfg,
                                                          &calibration));

    evv_port_start();
    evvRunStaticInitialisers();
    OldInst *reader = new_voice();
    ESP_ERROR_CHECK(reader ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(vc_setVoiceParam(reader, 0, VOICE_SPEED, speech_rate) >= 0
                        ? ESP_OK : ESP_FAIL);

    button_queue = xQueueCreate(1, sizeof(int));
    ESP_ERROR_CHECK(button_queue ? ESP_OK : ESP_ERR_NO_MEM);
    static ButtonContext buttons;
    buttons.adc = adc;
    buttons.calibration = calibration;
    ESP_ERROR_CHECK(xTaskCreate(button_scan_task, "buttons", 4096, &buttons, 5,
                                NULL) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);

    puts("PARAGRAPH_READER_READY centre=play_pause left_right=volume up_down=rate");
    puts("EVENT_LOOP_READY");

    for (;;) {
        int key = 0;
        if (xQueueReceive(button_queue, &key, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (key == 1) {
                if (playback_state == STATE_PLAYING) {
                    (void)es_pause(reader, 1);
                    silence_and_stop_channel();
                    playback_state = STATE_PAUSED;
                    puts("PLAYBACK paused");
                } else if (playback_state == STATE_PAUSED) {
                    if (start_channel()) {
                        (void)es_pause(reader, 0);
                        playback_state = STATE_PLAYING;
                        puts("PLAYBACK resumed");
                    }
                } else {
                    (void)vc_setVoiceParam(reader, 0, VOICE_SPEED,
                                            speech_rate);
                    emitted_samples = 0;
                    paragraph_chunk = 0;
                    if (start_channel()
                        && et_addText(reader, paragraph_chunks[paragraph_chunk])
                        && et_synthesize(reader)) {
                        playback_state = STATE_PLAYING;
                        puts("PLAYBACK started");
                    }
                }
            } else if (key == 2 || key == 4) {
                int change = key == 2 ? 15 : -15;
                int next = speech_rate + change;
                if (next < 60) next = 60;
                if (next > 180) next = 180;
                speech_rate = next;
                (void)vc_setVoiceParam(reader, 0, VOICE_SPEED, speech_rate);
                printf("RATE value=%d playing=%d\n", speech_rate,
                       playback_state == STATE_PLAYING);
            } else if (key == 3 || key == 5) {
                int change = key == 5 ? 10 : -10;
                int next = listening_volume + change;
                if (next < 10) next = 10;
                if (next > 100) next = 100;
                listening_volume = next;
                printf("VOLUME value=%d playing=%d\n", listening_volume,
                       playback_state == STATE_PLAYING);
            }
        }

        if (playback_state == STATE_PLAYING && emitted_samples > 0
            && !eo_speaking(reader)) {
            eo_synchronizeSynth(reader);
            if (++paragraph_chunk < PARAGRAPH_CHUNK_COUNT) {
                emitted_samples = 0;
                if (!et_addText(reader, paragraph_chunks[paragraph_chunk])
                    || !et_synthesize(reader)) {
                    silence_and_stop_channel();
                    playback_state = STATE_FINISHED;
                    puts("PLAYBACK submission_failed");
                } else {
                    printf("PLAYBACK chunk=%u\n", (unsigned)paragraph_chunk);
                }
            } else {
                silence_and_stop_channel();
                playback_state = STATE_FINISHED;
                puts("PLAYBACK finished centre_restarts=1");
            }
        }
    }
}
