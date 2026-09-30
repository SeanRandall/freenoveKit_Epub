#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "driver/i2s_std.h"
#include "driver/temperature_sensor.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "evv_abi.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "epub_text.h"

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
int STDCALL vc_setVoiceParam(OldInst *h, int32_t voice, int32_t which,
                             int32_t value);
int32_t STDCALL api_set_param(void *h2, int32_t kind, int32_t param,
                              int32_t value);
void STDCALL eo_registerCallback(OldInst *h, void *cb, void *data);
void STDCALL eo_synchronizeSynth(OldInst *h);
int STDCALL eo_speaking(OldInst *h);
void evvRunStaticInitialisers(void);
void evv_port_start(void);

#define SAMPLE_RATE 11025
#define FRAME_SAMPLES 2048
#define PAUSE_REWIND_MS 500
#define ROLLING_START_MS 4000
#define OPENING_BOOK_SOUND_MS 500
#define READER_PCM_BYTES ((7u * 1024u * 1024u) / 2u)
#define READER_RETAIN_SAMPLES (SAMPLE_RATE * 8)
#define READER_RING_LOW_SAMPLES (SAMPLE_RATE * 30)
#define READER_RING_HIGH_SAMPLES (SAMPLE_RATE * 60)
#define READER_SYNTH_RESERVE_SAMPLES (SAMPLE_RATE * 10)
#define VOICE_SPEED 6
#define VOICE_VOLUME 7
#define ENV_PHRASE_PREDICTION 11
#define PARAM_PHRASE_PREDICTION 13
#define ENGINE_VOLUME_MAX 80
#define PARAM_INPUT_TYPE 1
#define READER_MENU_LAST 13
#define I2S_BCLK GPIO_NUM_42
#define I2S_LRCLK GPIO_NUM_41
#define I2S_DATA GPIO_NUM_1
#define BUTTON_ADC_UNIT ADC_UNIT_2
#define BUTTON_ADC_CHANNEL ADC_CHANNEL_8 /* GPIO19 on ESP32-S3 */

static const char *const TAG = "button_speech";
static int16_t *mono_frame;
static int16_t *stereo_frame;
static int16_t *reader_playback_frame;
static i2s_chan_handle_t tx_channel;
static temperature_sensor_handle_t temperature_sensor;
static volatile size_t speech_samples;
static volatile uint32_t requested_generation;
static uint32_t active_generation;
static QueueHandle_t audio_queue;

#ifdef PARAGRAPH_READER_MODE
static volatile bool reader_paused = true;
static volatile bool reader_started;
static volatile bool reader_finished;
static volatile int reader_volume = 50;
static volatile int reader_rate = 80;
static volatile bool reader_loaded;
static volatile size_t reader_play_offset;
static volatile bool reader_audio_active;
static volatile bool reader_rendering;
static volatile bool reader_render_abort;
static volatile bool reader_render_stop_requested;
static volatile bool reader_production_complete;
static volatile bool reader_play_task_running;
static volatile bool reader_opening_tone_running;
static volatile size_t reader_keypress_sample = SIZE_MAX;
static volatile int32_t reader_seek_samples;
static volatile int reader_sentence_seek;
static volatile bool reader_controls_ready;
static volatile bool reader_ui_busy;
static volatile bool reader_ui_tone_pending;
static volatile bool reader_resume_after_status;
static int16_t *reader_pcm;
static volatile size_t reader_pcm_samples;
static volatile uint64_t reader_total_pcm_generated;
static size_t reader_pcm_capacity;
static SemaphoreHandle_t reader_pcm_mutex;
static size_t reader_discarded_text_offset;
static bool reader_ring_fill_active;
static size_t reader_text_offset;
static bool reader_sentence_continuation;
static char reader_text_chunk[768];
static char reader_substituted_chunk[768];
static bool reader_render_overflow;
static int64_t reader_chunk_started_us;
static size_t reader_chunk_started_samples;
static size_t reader_chunk_started_bytes;
static bool reader_pcm_valid;
static int reader_pcm_rate;
static nvs_handle_t reader_prefs;
static char reader_announcement[320];
static epub_document_t reader_document;
static bool reader_library = true;
static bool reader_open_pending;
static char reader_library_names[16][256];
static size_t reader_library_count;
static size_t reader_library_index;
static bool reader_library_transition_announcement;
static char reader_selected_path[320];
static size_t reader_section_index;
static uint32_t reader_saved_section;
static uint32_t reader_saved_text_offset;
static size_t reader_section_base_offset;
static size_t reader_stream_base_absolute;
static size_t reader_book_length;
static bool reader_resume_after_load;
static bool reader_section_announcement;
static bool reader_section_change_pending;
static volatile bool reader_continue_pending;
static volatile bool reader_menu;
static bool reader_status_menu;
static size_t reader_menu_index;
static volatile bool reader_menu_announcement_pending;
static TickType_t reader_menu_changed_at;
static volatile bool reader_preferences_dirty;
static TickType_t reader_preferences_changed_at;
static bool reader_dictionary_commands = true;
static bool reader_substitutions_enabled = true;
static bool reader_interface_sounds = true;
static bool reader_remaining_scope_section;
static bool reader_pause_between_sections;
static bool reader_startup_resume;
static uint8_t reader_sleep_timer_choice;
static bool reader_sleep_timer_reset_on_key;
static bool reader_sleep_timer_start_on_boot;
static volatile bool reader_sleep_timer_active;
static TickType_t reader_sleep_timer_deadline;
static volatile bool reader_boundary_paused;
static size_t reader_next_pause_section;
static size_t reader_status_index;
static TickType_t reader_status_last_press;
enum ReaderStatusField {
    READER_STATUS_TIME,
    READER_STATUS_DATE,
    READER_STATUS_SECTION_REMAINING,
    READER_STATUS_FILE_REMAINING,
    READER_STATUS_BATTERY_LEVEL,
    READER_STATUS_BATTERY_TIME,
    READER_STATUS_SLEEP_REMAINING,
    READER_STATUS_SD_SPACE,
    READER_STATUS_DEVICE_TEMPERATURE,
    READER_STATUS_COUNT,
};
static uint8_t reader_status_order[READER_STATUS_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
static uint32_t reader_status_enabled =
    (1U << READER_STATUS_TIME) | (1U << READER_STATUS_FILE_REMAINING)
    | (1U << READER_STATUS_BATTERY_LEVEL);
static size_t reader_status_menu_index;
static volatile bool reader_recording;
static volatile bool reader_recording_stop;
static TaskHandle_t reader_recording_task_handle;
typedef struct { char *from; char *to; } ReaderSubstitution;
static ReaderSubstitution *reader_substitutions;
static size_t reader_substitution_count;
static const char reader_paragraph[] =
    "This is a paragraph reader test. "
    "Accessible technology is most useful when it fades into the background. "
    "A reader should respond to a small number of controls and explain every change clearly. "
    "It should let the listener concentrate on the words rather than the machine. "
    "This tiny computer is demonstrating that idea using only five directions and a loudspeaker. "
    "The centre control starts and stops this paragraph. "
    "Left and right change the listening volume, while up and down change the speaking rate. "
    "Good accessibility is a design decision made from the very first button press.";
static const char *const reader_text[] = {
    "This is a paragraph reader test.",
    "Accessible technology is most useful when it fades into the background.",
    "A reader should respond to a small number of controls and explain every change clearly.",
    "It should let the listener concentrate on the words rather than the machine.",
    "This tiny computer is demonstrating that idea using only five directions and a loudspeaker.",
    "The centre control starts and pauses this paragraph.",
    "Left and right change the listening volume, while up and down change the speaking rate.",
    "Good accessibility is a design decision made from the very first button press.",
};
#define READER_CHUNK_COUNT (sizeof(reader_text) / sizeof(reader_text[0]))
/* Public-domain test text: Lewis Carroll, Alice's Adventures in Wonderland,
   Chapter I, Project Gutenberg ebook 11. Normalised to ASCII for the engine. */
static const char reader_demo_text[] =
    "Chapter one. Down the Rabbit Hole. "
    "Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do. "
    "Once or twice she had peeped into the book her sister was reading, but it had no pictures or conversations in it. "
    "And what is the use of a book, thought Alice, without pictures or conversations? "
    "So she was considering in her own mind, as well as she could, for the hot day made her feel very sleepy and stupid, "
    "whether the pleasure of making a daisy chain would be worth the trouble of getting up and picking the daisies, "
    "when suddenly a White Rabbit with pink eyes ran close by her. "
    "There was nothing so very remarkable in that, nor did Alice think it so very much out of the way to hear the Rabbit say to itself, "
    "Oh dear! Oh dear! I shall be late! "
    "When she thought it over afterwards, it occurred to her that she ought to have wondered at this, "
    "but at the time it all seemed quite natural. "
    "But when the Rabbit actually took a watch out of its waistcoat pocket, and looked at it, and then hurried on, "
    "Alice started to her feet. It flashed across her mind that she had never before seen a rabbit with either a waistcoat pocket, "
    "or a watch to take out of it. Burning with curiosity, she ran across the field after it, "
    "and fortunately was just in time to see it pop down a large rabbit hole under the hedge. "
    "In another moment down went Alice after it, never once considering how in the world she was to get out again. "
    "The rabbit hole went straight on like a tunnel for some way, and then dipped suddenly down. "
    "So suddenly that Alice had not a moment to think about stopping herself before she found herself falling down a very deep well.";
static const char *reader_book_text = reader_demo_text;

typedef struct {
    size_t text_offset;
    size_t sample_offset;
} ReaderMarker;
static ReaderMarker *reader_markers;
static size_t reader_marker_capacity;
static volatile size_t reader_marker_count;
#endif

enum AudioEventKind {
    AUDIO_SPEECH,
    AUDIO_NAVIGATION_SPEECH,
    AUDIO_LOAD,
    AUDIO_PARAGRAPH,
    AUDIO_LOCKED,
    AUDIO_UNLOCKED,
    AUDIO_VOLUME_MINIMUM,
    AUDIO_VOLUME_MAXIMUM,
    AUDIO_SWITCH_TRANSFER,
    AUDIO_SWITCH_REMOTE,
    AUDIO_RECORDING_STARTED,
    AUDIO_RECORDING_STOPPED,
};
typedef struct {
    enum AudioEventKind kind;
    const char *text;
    uint32_t generation;
} AudioEvent;

static void queue_audio(enum AudioEventKind kind, const char *text);
#ifdef PARAGRAPH_READER_MODE
static void reader_save_section(void);
#endif

typedef struct {
    adc_oneshot_unit_handle_t adc;
    adc_cali_handle_t calibration;
} ButtonContext;

extern const uint8_t locked_wav_start[]
    asm("_binary_locked_wav_start");
extern const uint8_t locked_wav_end[]
    asm("_binary_locked_wav_end");
extern const uint8_t unlocked_wav_start[]
    asm("_binary_unlocked_wav_start");
extern const uint8_t unlocked_wav_end[]
    asm("_binary_unlocked_wav_end");
extern const uint8_t volume_minimum_wav_start[]
    asm("_binary_volume_minimum_wav_start");
extern const uint8_t volume_minimum_wav_end[]
    asm("_binary_volume_minimum_wav_end");
extern const uint8_t volume_maximum_wav_start[]
    asm("_binary_volume_maximum_wav_start");
extern const uint8_t volume_maximum_wav_end[]
    asm("_binary_volume_maximum_wav_end");
extern const uint8_t ui_wav_start[] asm("_binary_UI_wav_start");
extern const uint8_t ui_wav_end[] asm("_binary_UI_wav_end");
extern const uint8_t keypress_wav_start[] asm("_binary_keypress_wav_start");
extern const uint8_t keypress_wav_end[] asm("_binary_keypress_wav_end");
extern const uint8_t opening_book_wav_start[]
    asm("_binary_opening_book_wav_start");
extern const uint8_t opening_book_wav_end[]
    asm("_binary_opening_book_wav_end");
extern const uint8_t recording_started_wav_start[]
    asm("_binary_recording_started_wav_start");
extern const uint8_t recording_started_wav_end[]
    asm("_binary_recording_started_wav_end");
extern const uint8_t recording_stopped_wav_start[]
    asm("_binary_recording_stopped_wav_start");
extern const uint8_t recording_stopped_wav_end[]
    asm("_binary_recording_stopped_wav_end");

#ifdef PARAGRAPH_READER_MODE
static void reader_mix_keypress(int16_t *output, size_t frames);
#endif

static enum ECICallbackReturn STDCALL on_message(OldInst *instance,
                                                  enum ECIMessage message,
                                                  long parameter,
                                                  void *context)
{
    (void)instance;
    (void)context;
    if (message == eciIndexReply) {
#ifdef PARAGRAPH_READER_MODE
        size_t index = (size_t)parameter;
        xSemaphoreTake(reader_pcm_mutex, portMAX_DELAY);
        if (reader_rendering && index < reader_marker_count
            && index < reader_marker_capacity)
            reader_markers[index].sample_offset = reader_pcm_samples;
        xSemaphoreGive(reader_pcm_mutex);
#endif
        return eciDataProcessed;
    }
    if (message != eciWaveformBuffer)
        return eciDataProcessed;
    size_t count = (size_t)parameter;
#ifdef PARAGRAPH_READER_MODE
    if (reader_rendering) {
        if (reader_render_abort)
            return eciDataAbort;
        xSemaphoreTake(reader_pcm_mutex, portMAX_DELAY);
        size_t retain_floor = reader_play_offset > READER_RETAIN_SAMPLES
            ? reader_play_offset - READER_RETAIN_SAMPLES : 0;
        size_t used = reader_pcm_samples - retain_floor;
        size_t room = used < reader_pcm_capacity
            ? reader_pcm_capacity - used : 0;
        if (count > room) {
            count = room;
            reader_render_overflow = true;
        }
        if (count) {
            size_t write_at = reader_pcm_samples % reader_pcm_capacity;
            size_t first = reader_pcm_capacity - write_at;
            if (first > count) first = count;
            if (first < count)
                printf("PCM_RING_WRITE_WRAP absolute=%u physical=%u count=%u\n",
                       (unsigned)reader_pcm_samples, (unsigned)write_at,
                       (unsigned)count);
            memcpy(reader_pcm + write_at, mono_frame,
                   first * sizeof(int16_t));
            if (first < count)
                memcpy(reader_pcm, mono_frame + first,
                       (count - first) * sizeof(int16_t));
        }
        reader_pcm_samples += count;
        reader_total_pcm_generated += count;
        xSemaphoreGive(reader_pcm_mutex);
        return reader_render_overflow ? eciDataAbort : eciDataProcessed;
    }
#endif
    if (active_generation != requested_generation)
        return eciDataAbort;
    for (size_t i = 0; i < count; ++i) {
        /* The amplifier is mono, but the standard-I2S peripheral sends slots
           in pairs. Duplicating the sample also makes either board revision
           and either headphone channel audible. */
#ifdef PARAGRAPH_READER_MODE
        int32_t sample = ((int32_t)mono_frame[i] * reader_volume) / 100;
        stereo_frame[i * 2] = (int16_t)sample;
        stereo_frame[i * 2 + 1] = (int16_t)sample;
#else
        stereo_frame[i * 2] = mono_frame[i];
        stereo_frame[i * 2 + 1] = mono_frame[i];
#endif
    }
#ifdef PARAGRAPH_READER_MODE
    reader_mix_keypress(stereo_frame, count);
#endif

    size_t bytes_written = 0;
    esp_err_t err = i2s_channel_write(tx_channel, stereo_frame,
                                      count * 2 * sizeof(int16_t),
                                      &bytes_written, portMAX_DELAY);
    if (err != ESP_OK || bytes_written != count * 2 * sizeof(int16_t)) {
        ESP_LOGE(TAG, "I2S write failed: %s (%u/%u bytes)",
                 esp_err_to_name(err), (unsigned)bytes_written,
                 (unsigned)(count * 2 * sizeof(int16_t)));
        return eciDataAbort;
    }
    speech_samples += count;
    return eciDataProcessed;
}

static esp_err_t init_audio(void)
{
    i2s_chan_config_t channel_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    /* Keep roughly 140 ms queued in hardware: the PSRAM PCM ring supplies
       the longer lead, while a smaller DMA queue preserves internal RAM for
       OpenEVV's deferred locks with the 32 KB instruction cache.
       playback-task wakeup without taking so much internal DMA RAM that
       OpenEVV's own mutex allocation fails. */
    channel_cfg.dma_desc_num = 6;
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
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_channel, &config),
                        TAG, "configure I2S");
    return ESP_OK;
}

static bool speak(OldInst *instance, const char *text, uint32_t generation)
{
    static const int16_t silence[FRAME_SAMPLES * 2];
    size_t silence_written = 0;

#ifdef PARAGRAPH_READER_MODE
    if (!reader_audio_active) {
        /* A stopped TX channel retains its old DMA descriptors. Replace the
           whole ring with silence before enabling it, otherwise pause/resume
           can replay the tail of the previous sentence. */
        size_t preloaded = 0;
        (void)i2s_channel_preload_data(tx_channel, silence, sizeof(silence),
                                       &preloaded);
        if (i2s_channel_enable(tx_channel) != ESP_OK)
            return false;
        reader_audio_active = true;
    }
#else
    if (i2s_channel_enable(tx_channel) != ESP_OK)
        return false;
#endif
    speech_samples = 0;
    (void)ev_setParam(instance, PARAM_INPUT_TYPE, 0);
    active_generation = generation;
    printf("SPEAK %s\n", text);
    if (!et_addText(instance, text) || !et_synthesize(instance)) {
        (void)i2s_channel_disable(tx_channel);
#ifdef PARAGRAPH_READER_MODE
        reader_audio_active = false;
#endif
        return false;
    }

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
    while (xTaskGetTickCount() < deadline) {
        int speaking = eo_speaking(instance);
        if (speech_samples > 0 && !speaking)
            break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    eo_synchronizeSynth(instance);

    if (generation != requested_generation) {
        (void)i2s_channel_disable(tx_channel);
#ifdef PARAGRAPH_READER_MODE
        reader_audio_active = false;
#endif
        return false;
    }

#ifndef PARAGRAPH_READER_MODE
    /* The TX DMA descriptors form a ring. If the producer simply stops, this
       amplifier/driver combination repeats the last populated descriptors.
       Queue a full silent frame behind the speech, let it reach the wire, and
       then stop the channel so every utterance is genuinely one-shot. */
    (void)i2s_channel_write(tx_channel, silence, sizeof(silence),
                            &silence_written, portMAX_DELAY);
    for (int i = 0; i < 13 && generation == requested_generation; ++i)
        vTaskDelay(pdMS_TO_TICKS(20));
    (void)i2s_channel_disable(tx_channel);
#else
    printf("READER_WAVE samples=%u completed=%d\n", (unsigned)speech_samples,
           speech_samples > 0);
#endif
    return speech_samples > 0;
}

#ifdef PARAGRAPH_READER_MODE
static uint32_t reader_dither_state = UINT32_C(0x6d2b79f5);

static int16_t reader_scale_sample(int16_t input, int volume)
{
    if (volume >= 100)
        return input;

    /* TPDF dither prevents the severe zero-crossing quantisation heard when
       16-bit speech is digitally attenuated to headphone levels. */
    reader_dither_state ^= reader_dither_state << 13;
    reader_dither_state ^= reader_dither_state >> 17;
    reader_dither_state ^= reader_dither_state << 5;
    int32_t a = (int32_t)(reader_dither_state % 100);
    reader_dither_state ^= reader_dither_state << 13;
    reader_dither_state ^= reader_dither_state >> 17;
    reader_dither_state ^= reader_dither_state << 5;
    int32_t b = (int32_t)(reader_dither_state % 100);
    int32_t scaled = (int32_t)input * volume + a - b;
    return (int16_t)(scaled / 100);
}

/* Mix the short keypad cue into whichever PCM stream already owns I2S.  This
   avoids interrupting speech or racing a second writer against the shared
   transmitter.  The converted asset is mono, 16-bit and already at 11025 Hz. */
static void reader_mix_keypress(int16_t *output, size_t frames)
{
    if (!reader_interface_sounds)
        return;
    size_t position = __atomic_load_n(&reader_keypress_sample,
                                      __ATOMIC_ACQUIRE);
    if (position == SIZE_MAX || keypress_wav_end - keypress_wav_start < 44)
        return;
    size_t samples = ((size_t)keypress_wav_start[40]
                      | ((size_t)keypress_wav_start[41] << 8)
                      | ((size_t)keypress_wav_start[42] << 16)
                      | ((size_t)keypress_wav_start[43] << 24))
                     / sizeof(int16_t);
    const int16_t *cue = (const int16_t *)(keypress_wav_start + 44);
    size_t made = 0;
    while (made < frames && position + made < samples) {
        int32_t key = reader_scale_sample(cue[position + made], reader_volume);
        for (unsigned channel = 0; channel < 2; ++channel) {
            int32_t mixed = output[made * 2 + channel] + key;
            if (mixed > INT16_MAX) mixed = INT16_MAX;
            if (mixed < INT16_MIN) mixed = INT16_MIN;
            output[made * 2 + channel] = (int16_t)mixed;
        }
        ++made;
    }
    size_t expected = position;
    size_t next = position + made >= samples ? SIZE_MAX : position + made;
    (void)__atomic_compare_exchange_n(&reader_keypress_sample, &expected, next,
                                      false, __ATOMIC_RELEASE,
                                      __ATOMIC_RELAXED);
}

static void reader_keypress(void)
{
    if (reader_interface_sounds)
        __atomic_store_n(&reader_keypress_sample, 0, __ATOMIC_RELEASE);
}

static void reader_stop_audio(void)
{
    static const int16_t silence[FRAME_SAMPLES * 2];
    size_t written = 0;
    if (!reader_audio_active)
        return;
    (void)i2s_channel_write(tx_channel, silence, sizeof(silence), &written,
                            portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(40));
    (void)i2s_channel_disable(tx_channel);
    reader_audio_active = false;
}

static unsigned char reader_dictionary_cp1252(uint32_t cp)
{
    if (cp <= 0x7f || (cp >= 0xa0 && cp <= 0xff)) return (unsigned char)cp;
    switch (cp) {
        case 0x2018: return 0x91; case 0x2019: return 0x92;
        case 0x201c: return 0x93; case 0x201d: return 0x94;
        case 0x2013: return 0x96; case 0x2014: return 0x97;
        case 0x2026: return 0x85; case 0x2015: return '-';
        default: return '?';
    }
}

static void reader_dictionary_decode(char *text)
{
    unsigned char *src = (unsigned char *)text;
    unsigned char *dst = (unsigned char *)text;
    while (*src) {
        uint32_t cp;
        if (*src < 0x80) cp = *src++;
        else if ((*src & 0xe0) == 0xc0 && (src[1] & 0xc0) == 0x80) {
            cp = ((uint32_t)(src[0] & 0x1f) << 6) | (src[1] & 0x3f);
            src += 2;
        } else if ((*src & 0xf0) == 0xe0
                   && (src[1] & 0xc0) == 0x80
                   && (src[2] & 0xc0) == 0x80) {
            cp = ((uint32_t)(src[0] & 0x0f) << 12)
                | ((uint32_t)(src[1] & 0x3f) << 6) | (src[2] & 0x3f);
            src += 3;
        } else { cp = '?'; ++src; }
        *dst++ = reader_dictionary_cp1252(cp);
    }
    *dst = 0;
}

static void reader_dictionary_strip_commands(char *text)
{
    char *src = text, *dst = text;
    while (*src) {
        if (*src != '`') { *dst++ = *src++; continue; }
        ++src;
        while (isalnum((unsigned char)*src)) ++src;
        while (*src == ' ') ++src;
        if (dst > text && dst[-1] != ' ') *dst++ = ' ';
    }
    *dst = 0;
}

static char *reader_psram_strdup(const char *text)
{
    size_t bytes = strlen(text) + 1;
    char *copy = heap_caps_malloc(bytes,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (copy) memcpy(copy, text, bytes);
    return copy;
}

static void reader_load_substitutions(void)
{
    FILE *file = fopen("/sdcard/.evv/substitutions.tsv", "rb");
    if (!file) {
        puts("SUBSTITUTIONS none path=/sdcard/.evv/substitutions.tsv");
        return;
    }
    reader_substitutions = heap_caps_calloc(160,
        sizeof(*reader_substitutions), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!reader_substitutions) { fclose(file); return; }
    char line[512];
    while (reader_substitution_count < 160
           && fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *tab = strchr(line, '\t');
        if (!tab || tab == line) continue;
        *tab++ = 0;
        reader_dictionary_decode(line);
        reader_dictionary_decode(tab);
        /* Punctuation-dash substitutions from the desktop dictionary change
           the way OpenEVV parses otherwise normal compounds.  Word-specific
           entries such as Ha-Shan remain useful, but rules beginning with a
           dash are inappropriate for the reader's prose pipeline. */
        if (line[0] == '-' || (unsigned char)line[0] == 0x96
            || (unsigned char)line[0] == 0x97) {
            puts("SUBSTITUTION skipped=punctuation_dash_rule");
            continue;
        }
        ReaderSubstitution *rule =
            &reader_substitutions[reader_substitution_count];
        rule->from = reader_psram_strdup(line);
        rule->to = reader_psram_strdup(tab);
        if (!rule->from || !rule->to) break;
        ++reader_substitution_count;
    }
    fclose(file);
    printf("SUBSTITUTIONS loaded=%u path=/sdcard/.evv/substitutions.tsv\n",
           (unsigned)reader_substitution_count);
}

static const char *reader_apply_substitutions(const char *input)
{
    if (!reader_substitutions_enabled)
        return input;
    strlcpy(reader_substituted_chunk, input,
            sizeof(reader_substituted_chunk));
    char temporary[sizeof(reader_substituted_chunk)];
    for (size_t r = 0; r < reader_substitution_count; ++r) {
        const char *from = reader_substitutions[r].from;
        const char *to = reader_substitutions[r].to;
        char plain_replacement[512];
        if (!reader_dictionary_commands) {
            strlcpy(plain_replacement, to, sizeof(plain_replacement));
            reader_dictionary_strip_commands(plain_replacement);
            to = plain_replacement;
        }
        size_t from_n = strlen(from), to_n = strlen(to), used = 0;
        if (!from_n || !strstr(reader_substituted_chunk, from)) continue;
        for (const char *at = reader_substituted_chunk; *at;) {
            if (!strncmp(at, from, from_n)) {
                if (used + to_n >= sizeof(temporary)) break;
                memcpy(temporary + used, to, to_n); used += to_n; at += from_n;
            } else {
                if (used + 1 >= sizeof(temporary)) break;
                temporary[used++] = *at++;
            }
        }
        temporary[used] = 0;
        strlcpy(reader_substituted_chunk, temporary,
                sizeof(reader_substituted_chunk));
    }
    return reader_substituted_chunk;
}

static void reader_log_dash_context(const char *stage, const char *text)
{
    for (size_t i = 0; text[i]; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c != '-' && c != 0x96 && c != 0x97)
            continue;
        size_t start = i > 20 ? i - 20 : 0;
        size_t end = strlen(text);
        if (end > i + 21) end = i + 21;
        printf("DASH_%s byte=%02X context=", stage, c);
        for (size_t p = start; p < end; ++p) {
            unsigned char b = (unsigned char)text[p];
            if (b >= 32 && b < 127) putchar((char)b);
            else printf("<%02X>", b);
        }
        putchar('\n');
    }
}

/* Pausing deliberately retains reader_pcm and reader_play_offset.  UI speech
   must not, however, inherit already-populated I2S DMA descriptors from book
   playback.  Recreating this small channel is the only reliable flush offered
   by the standard driver and does not discard the resumable book cache. */
static bool reader_discard_dma_audio(void)
{
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(500);
    while (reader_play_task_running && xTaskGetTickCount() < deadline)
        vTaskDelay(pdMS_TO_TICKS(1));
    if (reader_audio_active) {
        (void)i2s_channel_disable(tx_channel);
        reader_audio_active = false;
    }
    if (tx_channel) {
        if (i2s_del_channel(tx_channel) != ESP_OK)
            return false;
        tx_channel = NULL;
    }
    return init_audio() == ESP_OK;
}

static bool reader_start_next_text_chunk(OldInst *instance)
{
    size_t text_len = reader_book_length;
    if (reader_text_offset >= text_len)
        return false;

    /* Generate in batches: refill from thirty to sixty seconds ahead, then
       leave both cores and PSRAM quiet while playback consumes the batch. */
    while (reader_play_task_running) {
        if (reader_render_abort || reader_render_stop_requested)
            return false;
        size_t lead = reader_pcm_samples >= reader_play_offset
            ? reader_pcm_samples - reader_play_offset : 0;
        if (reader_ring_fill_active) {
            if (lead < READER_RING_HIGH_SAMPLES)
                break;
            reader_ring_fill_active = false;
            printf("RING_FILL idle lead_ms=%u\n",
                   (unsigned)(lead * 1000 / SAMPLE_RATE));
        } else if (lead <= READER_RING_LOW_SAMPLES) {
            reader_ring_fill_active = true;
            printf("RING_FILL active lead_ms=%u\n",
                   (unsigned)(lead * 1000 / SAMPLE_RATE));
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Absolute read/write counters address a physical circular buffer.  Old
       samples fall out naturally; no audio is ever moved in PSRAM. */
    for (;;) {
        if (reader_render_abort || reader_render_stop_requested)
            return false;
        size_t retain_floor = reader_play_offset > READER_RETAIN_SAMPLES
            ? reader_play_offset - READER_RETAIN_SAMPLES : 0;
        size_t used = reader_pcm_samples - retain_floor;
        size_t free_samples = used < reader_pcm_capacity
            ? reader_pcm_capacity - used : 0;
        if (free_samples >= READER_SYNTH_RESERVE_SAMPLES) {
            xSemaphoreTake(reader_pcm_mutex, portMAX_DELAY);
            size_t out = 0;
            for (size_t i = 0; i < reader_marker_count; ++i) {
                size_t sample = reader_markers[i].sample_offset;
                if (sample != SIZE_MAX && sample < retain_floor) {
                    if (reader_markers[i].text_offset
                        > reader_discarded_text_offset)
                        reader_discarded_text_offset =
                            reader_markers[i].text_offset;
                    continue;
                }
                reader_markers[out++] = reader_markers[i];
            }
            reader_marker_count = out;
            xSemaphoreGive(reader_pcm_mutex);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    size_t remaining = text_len - reader_text_offset;
    size_t chunk_start = reader_text_offset;
    /* Never synthesize across an EPUB section boundary. This gives playback
       an exact PCM marker at which it can optionally pause without throwing
       away or regenerating the following audio. */
    size_t until_section = remaining;
    size_t chunk_absolute = reader_stream_base_absolute + chunk_start;
    for (size_t i = 0; i < reader_document.section_count; ++i) {
        size_t boundary = reader_document.sections[i].text_offset;
        if (boundary > chunk_absolute) {
            size_t distance = boundary - chunk_absolute;
            if (distance < until_section) until_section = distance;
            break;
        }
    }
    /* Prefer one complete sentence. Long sentences still split at a safe word
       boundary so OpenEVV regularly yields its core. */
    /* At faster voices a fixed amount of text yields fewer seconds of PCM,
       while OpenEVV still pays its per-request setup cost.  Give it more text
       per request so synthesis can stay ahead without increasing the cold
       six-second playback delay.  The buffers below have room for 767 bytes. */
    /* Minimise time-to-first-audio.  The first request only needs enough PCM
       to bridge into the normal rolling producer; subsequent requests retain
       the larger high-rate batches that keep playback comfortably ahead. */
    bool cold_chunk = reader_pcm_samples == 0;
    size_t chunk_target = cold_chunk ? 320
        : reader_rate <= 115 ? 180
        : reader_rate <= 120 ? 320
        : reader_rate <= 130 ? 480 : 640;
    size_t take = until_section < chunk_target ? until_section : chunk_target;
    bool sentence_ended = until_section <= take;
    if (take < until_section) {
        /* Batch as many complete sentences as fit.  Previously assigning
           `take = after` inside this loop also shortened the loop bound, so
           every request stopped at the *first* sentence despite the larger
           target.  Keep the scan limit fixed and remember the last boundary. */
        const size_t scan_limit = take;
        size_t last_sentence_boundary = 0;
        for (size_t at = 0; at < scan_limit; ++at) {
            unsigned char c = (unsigned char)reader_book_text[chunk_start + at];
            if (c != '.' && c != '?' && c != '!') continue;
            size_t after = at + 1;
            while (after < until_section && after < scan_limit
                   && strchr("\"')\x92\x94", reader_book_text[chunk_start + after]))
                ++after;
            if (after == until_section
                || reader_book_text[chunk_start + after] == ' ') {
                last_sentence_boundary = after;
            }
        }
        if (last_sentence_boundary != 0
            && (!cold_chunk
                || last_sentence_boundary >= chunk_target * 3 / 4)) {
            take = last_sentence_boundary;
            sentence_ended = true;
        } else {
            size_t boundary = take;
            while (boundary > 90
                   && reader_book_text[chunk_start + boundary] != ' ')
                --boundary;
            if (boundary > 90) take = boundary;
        }
    }
    memcpy(reader_text_chunk, reader_book_text + reader_text_offset, take);
    reader_text_chunk[take] = '\0';
    reader_text_offset += take;
    while (reader_text_offset < text_len
           && reader_book_text[reader_text_offset] == ' ')
        ++reader_text_offset;

    reader_rendering = true;
    size_t lead_before = reader_pcm_samples >= reader_play_offset
        ? reader_pcm_samples - reader_play_offset : 0;
    printf("SYNTH_CHUNK rate=%d target=%u bytes=%u text_offset=%u lead_ms=%u\n",
           reader_rate, (unsigned)chunk_target, (unsigned)take,
           (unsigned)reader_text_offset,
           (unsigned)(lead_before * 1000 / SAMPLE_RATE));
    if (!reader_sentence_continuation
        && reader_marker_count < reader_marker_capacity) {
        size_t marker = reader_marker_count++;
        reader_markers[marker].text_offset = chunk_start;
        reader_markers[marker].sample_offset = SIZE_MAX;
        if (!et_insertIndex(instance, (int32_t)marker))
            reader_markers[marker].sample_offset = reader_pcm_samples;
    }
    reader_sentence_continuation = !sentence_ended;
    const char *synthesis_text = reader_apply_substitutions(reader_text_chunk);
    reader_log_dash_context("SOURCE", reader_text_chunk);
    reader_log_dash_context("RESULT", synthesis_text);
    (void)ev_setParam(instance, PARAM_INPUT_TYPE,
                      reader_dictionary_commands ? 1 : 0);
    reader_chunk_started_us = esp_timer_get_time();
    reader_chunk_started_samples = reader_pcm_samples;
    reader_chunk_started_bytes = take;
    if (!et_addText(instance, synthesis_text) || !et_synthesize(instance)) {
        reader_rendering = false;
        return false;
    }
    return true;
}

static void reader_abort_render(OldInst *instance)
{
    reader_render_abort = true;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(10000);
    while (eo_speaking(instance) && xTaskGetTickCount() < deadline)
        vTaskDelay(pdMS_TO_TICKS(1));
    reader_rendering = false;
}

static bool reader_speak_buffered(OldInst *instance, const char *text,
                                  uint32_t generation)
{
    reader_pcm_samples = 0;
    reader_pcm_valid = false;
    reader_render_overflow = false;
    reader_render_abort = false;
    reader_rendering = true;
    (void)ev_setParam(instance, PARAM_INPUT_TYPE, 0);
    active_generation = generation;
    if (!et_addText(instance, text) || !et_synthesize(instance)) {
        reader_rendering = false;
        return false;
    }
    while (eo_speaking(instance) && generation == requested_generation)
        vTaskDelay(pdMS_TO_TICKS(1));
    reader_rendering = false;
    if (generation != requested_generation || !reader_pcm_samples)
        return false;

    if (i2s_channel_enable(tx_channel) != ESP_OK)
        return false;
    reader_audio_active = true;
    for (size_t at = 0; at < reader_pcm_samples;) {
        if (generation != requested_generation) {
            reader_stop_audio();
            return false;
        }
        size_t made = reader_pcm_samples - at;
        if (made > FRAME_SAMPLES) made = FRAME_SAMPLES;
        for (size_t i = 0; i < made; ++i) {
            int16_t sample = reader_scale_sample(
                                                 reader_pcm[(at + i)
                                                    % reader_pcm_capacity],
                                                 reader_volume);
            stereo_frame[i * 2] = sample;
            stereo_frame[i * 2 + 1] = sample;
        }
        size_t written = 0;
        if (i2s_channel_write(tx_channel, stereo_frame,
                              made * 2 * sizeof(int16_t), &written,
                              portMAX_DELAY) != ESP_OK) {
            reader_stop_audio();
            return false;
        }
        at += made;
    }
    reader_stop_audio();
    reader_pcm_samples = 0;
    return true;
}

static bool reader_load_book(OldInst *instance, uint32_t generation)
{
    int64_t started_us = esp_timer_get_time();
    reader_pcm_samples = 0;
    /* Absolute ring counters belong to the discarded cache. Reset the read
       counter at the same time as the write counter; retaining an offset from
       the previous rate makes the unsigned free-space calculation underflow
       and regeneration wait forever. */
    reader_play_offset = 0;
    reader_render_overflow = false;
    reader_pcm_valid = false;
    reader_loaded = false;
    reader_render_abort = false;
    reader_rendering = false;
    reader_production_complete = false;
    reader_text_offset = 0;
    reader_sentence_continuation = false;
    reader_marker_count = 0;
    reader_total_pcm_generated = 0;
    reader_discarded_text_offset = 0;
    reader_ring_fill_active = true;
    reader_boundary_paused = false;
    reader_next_pause_section = reader_document.section_count;
    for (size_t i = 0; i < reader_document.section_count; ++i) {
        if (reader_document.sections[i].text_offset
                > reader_stream_base_absolute) {
            reader_next_pause_section = i;
            break;
        }
    }
    active_generation = generation;
    printf("ROLLING_BEGIN text_bytes=%u rate=%d target_lead_ms=%d\n",
           (unsigned)reader_book_length, reader_rate, ROLLING_START_MS);
    if (!reader_start_next_text_chunk(instance))
        return false;

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(60000);
    while (eo_speaking(instance) && xTaskGetTickCount() < deadline) {
        if (reader_render_abort)
            return false;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    eo_synchronizeSynth(instance);
    reader_rendering = false;
    if (reader_pcm_samples == 0)
        return false;

    /* A single cold chunk gave only about three seconds of separation from
       the concurrently running synthesizer.  Build a materially larger lead
       before enabling playback, while retaining rolling generation after it
       starts. */
    while (reader_pcm_samples * 1000 / SAMPLE_RATE < ROLLING_START_MS
           && reader_text_offset < reader_book_length) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (!reader_start_next_text_chunk(instance))
            break;
        while (eo_speaking(instance) && xTaskGetTickCount() < deadline) {
            if (reader_render_abort)
                return false;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        eo_synchronizeSynth(instance);
        reader_rendering = false;
    }

    reader_pcm_valid = true;
    reader_pcm_rate = reader_rate;
    reader_loaded = true;
    reader_play_offset = 0;
    printf("BOOK_READY buffered_samples=%u buffered_ms=%u ready_ms=%u speaking=%d\n",
           (unsigned)reader_pcm_samples,
           (unsigned)(reader_pcm_samples * 1000 / SAMPLE_RATE),
           (unsigned)((esp_timer_get_time() - started_us) / 1000),
           eo_speaking(instance));
    (void)reader_start_next_text_chunk(instance);
    return true;
}

static bool reader_play_pcm(uint32_t generation)
{
    static const int16_t silence[FRAME_SAMPLES * 2];
    size_t preloaded = 0;
    (void)i2s_channel_preload_data(tx_channel, silence, sizeof(silence),
                                   &preloaded);
    if (i2s_channel_enable(tx_channel) != ESP_OK)
        return false;
    reader_audio_active = true;

    unsigned underruns = 0;
    unsigned feed_stalls = 0;
    bool low_buffer_reported = false;
    size_t minimum_rolling_lead = SIZE_MAX;
    int64_t worst_feed_gap_us = 0;
    int64_t previous_write_return_us = esp_timer_get_time();
    for (size_t at = reader_play_offset;;) {
        size_t section_pause_at = SIZE_MAX;
        if (reader_pause_between_sections
            && reader_next_pause_section < reader_document.section_count) {
            size_t boundary_absolute =
                reader_document.sections[reader_next_pause_section].text_offset;
            if (boundary_absolute >= reader_stream_base_absolute) {
                size_t boundary_text =
                    boundary_absolute - reader_stream_base_absolute;
                xSemaphoreTake(reader_pcm_mutex, portMAX_DELAY);
                for (size_t i = 0; i < reader_marker_count; ++i) {
                    if (reader_markers[i].text_offset < boundary_text)
                        continue;
                    if (reader_markers[i].sample_offset != SIZE_MAX)
                        section_pause_at = reader_markers[i].sample_offset;
                    break;
                }
                xSemaphoreGive(reader_pcm_mutex);
            }
        }
        if (section_pause_at != SIZE_MAX && at >= section_pause_at) {
            reader_play_offset = section_pause_at;
            reader_paused = true;
            reader_boundary_paused = true;
            ++reader_next_pause_section;
            (void)i2s_channel_disable(tx_channel);
            reader_audio_active = false;
            printf("READER section_pause section=%u sample=%u\n",
                   (unsigned)reader_next_pause_section,
                   (unsigned)section_pause_at);
            while (reader_paused && generation == requested_generation)
                vTaskDelay(pdMS_TO_TICKS(10));
            if (generation != requested_generation)
                return false;
            if (i2s_channel_enable(tx_channel) != ESP_OK)
                return false;
            reader_audio_active = true;
            at = reader_play_offset;
            continue;
        }
        if (generation != requested_generation || reader_paused) {
            size_t resume_at = at;
            if (reader_paused) {
                size_t rewind = (SAMPLE_RATE * PAUSE_REWIND_MS) / 1000;
                resume_at = resume_at > rewind ? resume_at - rewind : 0;
                size_t retain_floor = reader_pcm_samples > reader_pcm_capacity
                    ? reader_pcm_samples - reader_pcm_capacity : 0;
                if (resume_at < retain_floor) resume_at = retain_floor;
                printf("READER pause_rewind_ms=%d from=%u to=%u\n",
                       PAUSE_REWIND_MS, (unsigned)at,
                       (unsigned)resume_at);
            }
            reader_play_offset = resume_at;
            (void)i2s_channel_disable(tx_channel);
            reader_audio_active = false;
            return false;
        }
        int sentence_seek = reader_sentence_seek;
        if (sentence_seek) {
            reader_sentence_seek = 0;
            size_t count = reader_marker_count;
            size_t target = SIZE_MAX;
            if (sentence_seek < 0) {
                size_t current = SIZE_MAX;
                for (size_t i = 0; i < count; ++i) {
                    size_t sample = reader_markers[i].sample_offset;
                    size_t retain_floor = reader_pcm_samples
                        > reader_pcm_capacity
                        ? reader_pcm_samples - reader_pcm_capacity : 0;
                    if (sample != SIZE_MAX && sample < retain_floor) continue;
                    if (sample == SIZE_MAX || sample > at) break;
                    current = i;
                }
                if (current != SIZE_MAX) {
                    size_t sample = reader_markers[current].sample_offset;
                    if (at > sample + SAMPLE_RATE)
                        target = current;
                    else if (current > 0)
                        target = current - 1;
                    else
                        target = 0;
                }
            } else {
                for (size_t i = 0; i < count; ++i) {
                    size_t sample = reader_markers[i].sample_offset;
                    if (sample != SIZE_MAX && sample > at + SAMPLE_RATE / 4) {
                        target = i;
                        break;
                    }
                }
            }
            if (target != SIZE_MAX
                && reader_markers[target].sample_offset != SIZE_MAX) {
                at = reader_markers[target].sample_offset;
                reader_play_offset = at;
                printf("READER sentence_seek=%d marker=%u position_samples=%u\n",
                       sentence_seek, (unsigned)target, (unsigned)at);
            } else {
                printf("READER sentence_seek_unavailable=%d markers=%u\n",
                       sentence_seek, (unsigned)count);
            }
        }
        int32_t seek = reader_seek_samples;
        if (seek) {
            reader_seek_samples = 0;
            size_t available_now = reader_pcm_samples;
            if (seek < 0) {
                size_t back = (size_t)(-seek);
                at = at > back ? at - back : 0;
            } else {
                size_t limit = available_now;
                size_t reserve = SAMPLE_RATE / 2;
                if (!reader_production_complete && limit > reserve)
                    limit -= reserve;
                size_t forward = (size_t)seek;
                at = forward < limit - (at < limit ? at : limit)
                    ? at + forward : limit;
            }
            reader_play_offset = at;
            printf("READER seek_ms=%ld position_samples=%u\n",
                   (long)((int64_t)seek * 1000 / SAMPLE_RATE),
                   (unsigned)at);
        }
        xSemaphoreTake(reader_pcm_mutex, portMAX_DELAY);
        size_t available = reader_pcm_samples;
        size_t lead = available >= at ? available - at : 0;
        if (!reader_production_complete && lead < SAMPLE_RATE) {
            if (!low_buffer_reported) {
                printf("BUFFER_LOW lead_ms=%u produced=%u played=%u\n",
                       (unsigned)(lead * 1000 / SAMPLE_RATE),
                       (unsigned)available, (unsigned)at);
                low_buffer_reported = true;
            }
        } else if (lead > SAMPLE_RATE * 2) {
            low_buffer_reported = false;
        }
        if (!reader_production_complete && available >= at
            && available - at < minimum_rolling_lead)
            minimum_rolling_lead = available - at;
        if (at >= available) {
            xSemaphoreGive(reader_pcm_mutex);
            if (!reader_production_complete) {
                ++underruns;
                vTaskDelay(pdMS_TO_TICKS(2));
                continue;
            }
            break;
        }
        size_t made = available - at;
        if (made > FRAME_SAMPLES)
            made = FRAME_SAMPLES;
        if (section_pause_at != SIZE_MAX && at < section_pause_at
            && made > section_pause_at - at)
            made = section_pause_at - at;
        size_t physical_at = at % reader_pcm_capacity;
        if (made > reader_pcm_capacity - physical_at)
            printf("PCM_RING_READ_WRAP absolute=%u physical=%u count=%u lead_ms=%u\n",
                   (unsigned)at, (unsigned)physical_at, (unsigned)made,
                   (unsigned)(lead * 1000 / SAMPLE_RATE));
        /* This frame belongs only to book playback.  UI tones and short
           announcements use stereo_frame, so they cannot alter data while
           the I2S driver is consuming a book frame.  Clear the unused tail as
           well: if a future driver/configuration queues whole DMA frames,
           silence is emitted instead of samples left by an earlier write. */
        memset(reader_playback_frame, 0,
               FRAME_SAMPLES * 2 * sizeof(*reader_playback_frame));
        for (size_t i = 0; i < made; ++i) {
            int32_t combined = reader_pcm[(at + i) % reader_pcm_capacity];
            if (combined > INT16_MAX) combined = INT16_MAX;
            if (combined < INT16_MIN) combined = INT16_MIN;
            int16_t sample = reader_scale_sample((int16_t)combined,
                                                 reader_volume);
            reader_playback_frame[i * 2] = sample;
            reader_playback_frame[i * 2 + 1] = sample;
        }
        reader_mix_keypress(reader_playback_frame, made);
        xSemaphoreGive(reader_pcm_mutex);
        size_t written = 0;
        int64_t before_write_us = esp_timer_get_time();
        int64_t feed_gap_us = before_write_us - previous_write_return_us;
        if (feed_gap_us > worst_feed_gap_us)
            worst_feed_gap_us = feed_gap_us;
        if (feed_gap_us > 10000)
            ++feed_stalls;
        size_t expected_bytes = made * 2 * sizeof(int16_t);
        if (i2s_channel_write(tx_channel, reader_playback_frame,
                              made * 2 * sizeof(int16_t), &written,
                              portMAX_DELAY) != ESP_OK
            || written != expected_bytes) {
            printf("I2S_BOOK_WRITE_ERROR expected=%u written=%u\n",
                   (unsigned)expected_bytes, (unsigned)written);
            reader_stop_audio();
            return false;
        }
        previous_write_return_us = esp_timer_get_time();
        at += made;
        reader_play_offset = at;
    }
    reader_stop_audio();
    printf("ROLLING_END samples=%u underrun_waits=%u feed_stalls=%u worst_feed_gap_ms=%u min_lead_ms=%u overflow=%d\n",
           (unsigned)reader_pcm_samples, underruns, feed_stalls,
           (unsigned)(worst_feed_gap_us / 1000),
           minimum_rolling_lead == SIZE_MAX ? 0U
               : (unsigned)(minimum_rolling_lead * 1000 / SAMPLE_RATE),
           reader_render_overflow);
    if (reader_play_offset >= reader_pcm_samples)
        reader_play_offset = 0;
    return generation == requested_generation && !reader_paused;
}

static void reader_play_task(void *arg)
{
    uint32_t generation = (uint32_t)(uintptr_t)arg;
    reader_play_task_running = true;
    bool completed = reader_play_pcm(generation);
    reader_play_task_running = false;
    if (completed) {
        reader_finished = true;
        reader_paused = true;
        puts("READER book finished");
    }
    vTaskDeleteWithCaps(NULL);
}

static void reader_finish_book_render(OldInst *instance)
{
    while (!reader_render_abort) {
        if (reader_rendering) {
            size_t before = reader_pcm_samples;
            while (eo_speaking(instance))
                vTaskDelay(pdMS_TO_TICKS(1));
            eo_synchronizeSynth(instance);
            reader_rendering = false;
            size_t added = reader_pcm_samples - reader_chunk_started_samples;
            int64_t elapsed_us = esp_timer_get_time() - reader_chunk_started_us;
            size_t lead = reader_pcm_samples >= reader_play_offset
                ? reader_pcm_samples - reader_play_offset : 0;
            printf("SYNTH_RESULT bytes=%u added_samples=%u audio_ms=%u synth_ms=%u lead_ms=%u total_samples=%u observed_before=%u\n",
                   (unsigned)reader_chunk_started_bytes, (unsigned)added,
                   (unsigned)(added * 1000 / SAMPLE_RATE),
                   (unsigned)(elapsed_us / 1000),
                   (unsigned)(lead * 1000 / SAMPLE_RATE),
                   (unsigned)reader_pcm_samples, (unsigned)before);
            /* Let core 1's idle task service its watchdog between the
               engine's CPU-heavy synthesis requests. Playback has several
               seconds of lead on core 0, so this does not affect audio. */
            vTaskDelay(pdMS_TO_TICKS(10));
            if (reader_render_stop_requested)
                break;
        }
        if (reader_text_offset >= reader_book_length)
            break;
        if (!reader_start_next_text_chunk(instance))
            break;
    }
    reader_rendering = false;
    reader_production_complete = reader_text_offset >= reader_book_length;
    reader_render_stop_requested = false;
    printf("SYNTH_COMPLETE samples=%u text_offset=%u aborted=%d\n",
           (unsigned)reader_pcm_samples, (unsigned)reader_text_offset,
           reader_render_abort);
}
#endif

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static bool play_wav_tone_limited(const uint8_t *wav_start,
                                  const uint8_t *wav_end,
                                  uint32_t generation,
                                  uint32_t maximum_ms)
{
    static const int16_t silence[FRAME_SAMPLES * 2];
    const size_t wav_size = (size_t)(wav_end - wav_start);
    size_t silence_written = 0;

    if (wav_size < 44 || read_le32(wav_start) != UINT32_C(0x46464952)
        || read_le32(wav_start + 8) != UINT32_C(0x45564157)) {
        ESP_LOGE(TAG, "invalid embedded WAV");
        return false;
    }

    uint16_t format = read_le16(wav_start + 20);
    uint16_t channels = read_le16(wav_start + 22);
    uint32_t source_rate = read_le32(wav_start + 24);
    uint16_t bits = read_le16(wav_start + 34);
    uint32_t data_bytes = read_le32(wav_start + 40);
    if ((size_t)data_bytes > wav_size - 44)
        return false;
    if (format != 1 || !channels || channels > 2 || bits != 16
        || !source_rate)
        return false;
    const int16_t *pcm = (const int16_t *)(wav_start + 44);
    size_t input_frames = data_bytes / (channels * sizeof(int16_t));
    if (maximum_ms) {
        size_t maximum_frames = (size_t)source_rate * maximum_ms / 1000;
        if (input_frames > maximum_frames)
            input_frames = maximum_frames;
    }

    if (i2s_channel_enable(tx_channel) != ESP_OK)
        return false;
    active_generation = generation;

    /* Accept both the original 22.05 kHz mono tones and user-supplied PCM
       WAVs such as the 48 kHz stereo opening-book sound. */
    uint64_t phase = 0;
    const uint64_t phase_step = ((uint64_t)source_rate << 32) / SAMPLE_RATE;
    while ((phase >> 32) < input_frames) {
        if (generation != requested_generation) {
            (void)i2s_channel_disable(tx_channel);
            return false;
        }
        size_t made = 0;
        while ((phase >> 32) < input_frames && made < FRAME_SAMPLES) {
            size_t frame = (size_t)(phase >> 32);
            int32_t mixed = pcm[frame * channels];
            if (channels == 2)
                mixed = (mixed + pcm[frame * channels + 1]) / 2;
#ifdef PARAGRAPH_READER_MODE
            int16_t sample = reader_scale_sample((int16_t)mixed,
                                                 reader_volume);
            stereo_frame[made * 2] = sample;
            stereo_frame[made * 2 + 1] = sample;
#else
            stereo_frame[made * 2] = (int16_t)mixed;
            stereo_frame[made * 2 + 1] = (int16_t)mixed;
#endif
            phase += phase_step;
            ++made;
        }
        reader_mix_keypress(stereo_frame, made);
        size_t bytes_written = 0;
        if (i2s_channel_write(tx_channel, stereo_frame,
                              made * 2 * sizeof(int16_t), &bytes_written,
                              portMAX_DELAY) != ESP_OK) {
            (void)i2s_channel_disable(tx_channel);
            return false;
        }
    }

    (void)i2s_channel_write(tx_channel, silence, sizeof(silence),
                            &silence_written, portMAX_DELAY);
    for (int i = 0; i < 13 && generation == requested_generation; ++i)
        vTaskDelay(pdMS_TO_TICKS(20));
    (void)i2s_channel_disable(tx_channel);
    return true;
}

static bool play_wav_tone(const uint8_t *wav_start, const uint8_t *wav_end,
                          uint32_t generation)
{
    return play_wav_tone_limited(wav_start, wav_end, generation, 0);
}

static void reader_opening_tone_task(void *arg)
{
    uint32_t generation = (uint32_t)(uintptr_t)arg;
    (void)play_wav_tone_limited(opening_book_wav_start,
                                opening_book_wav_end, generation,
                                OPENING_BOOK_SOUND_MS);
    reader_opening_tone_running = false;
    vTaskDeleteWithCaps(NULL);
}

static bool reader_start_opening_tone(uint32_t generation)
{
    if (reader_opening_tone_running)
        return true;
    reader_opening_tone_running = true;
    BaseType_t created = xTaskCreatePinnedToCoreWithCaps(
        reader_opening_tone_task, "opening_tone", 4096,
        (void *)(uintptr_t)generation, 5, NULL, 0,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (created == pdPASS)
        return true;
    reader_opening_tone_running = false;
    return false;
}

static int classify_button_mv(int millivolts)
{
    static const int centres_mv[] = { 0, 0, 660, 1320, 1980, 2640 };
    int best_key = 0;
    int best_distance = 220;

    /* With 12 dB attenuation the ESP32-S3 cannot accurately represent the
       ladder's 3.3 V idle level. Treat the saturated top end explicitly as
       released rather than letting it fall into the 2.64 V key window. */
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

static void queue_audio(enum AudioEventKind kind, const char *text)
{
#ifdef PARAGRAPH_READER_MODE
    if (kind != AUDIO_PARAGRAPH) {
        reader_ui_busy = true;
        if (reader_rendering)
            reader_render_stop_requested = true;
    }
#endif
    AudioEvent event = {
        .kind = kind,
        .text = text,
        .generation = ++requested_generation,
    };
    xQueueOverwrite(audio_queue, &event);
}

#ifdef PARAGRAPH_READER_MODE
static void reader_save_preferences(void)
{
    if (!reader_prefs)
        return;
    (void)nvs_set_i32(reader_prefs, "rate", reader_rate);
    (void)nvs_set_i32(reader_prefs, "volume", reader_volume);
    (void)nvs_set_i32(reader_prefs, "dictcmd",
                      reader_dictionary_commands ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "subdict",
                      reader_substitutions_enabled ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "uisound",
                      reader_interface_sounds ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "remscope",
                      reader_remaining_scope_section ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "sectpause",
                      reader_pause_between_sections ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "startup",
                      reader_startup_resume ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "sleeptime",
                      reader_sleep_timer_choice);
    (void)nvs_set_i32(reader_prefs, "sleepkey",
                      reader_sleep_timer_reset_on_key ? 1 : 0);
    (void)nvs_set_i32(reader_prefs, "sleepboot",
                      reader_sleep_timer_start_on_boot ? 1 : 0);
    (void)nvs_set_u32(reader_prefs, "statusmask", reader_status_enabled);
    (void)nvs_set_blob(reader_prefs, "statusorder", reader_status_order,
                       sizeof(reader_status_order));
    (void)nvs_commit(reader_prefs);
}

static void reader_schedule_preferences_save(void)
{
    reader_preferences_dirty = true;
    reader_preferences_changed_at = xTaskGetTickCount();
}

static void reader_load_preferences(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES
        || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(nvs_open("reader", NVS_READWRITE, &reader_prefs));
    int32_t value;
    if (nvs_get_i32(reader_prefs, "rate", &value) == ESP_OK
        && value >= 60 && value <= 180)
        reader_rate = value;
    if (nvs_get_i32(reader_prefs, "volume", &value) == ESP_OK
        && value >= 2 && value <= 100)
        reader_volume = value;
    if (nvs_get_i32(reader_prefs, "dictcmd", &value) == ESP_OK)
        reader_dictionary_commands = value != 0;
    if (nvs_get_i32(reader_prefs, "subdict", &value) == ESP_OK)
        reader_substitutions_enabled = value != 0;
    if (nvs_get_i32(reader_prefs, "uisound", &value) == ESP_OK)
        reader_interface_sounds = value != 0;
    if (nvs_get_i32(reader_prefs, "remscope", &value) == ESP_OK)
        reader_remaining_scope_section = value != 0;
    if (nvs_get_i32(reader_prefs, "sectpause", &value) == ESP_OK)
        reader_pause_between_sections = value != 0;
    if (nvs_get_i32(reader_prefs, "startup", &value) == ESP_OK)
        reader_startup_resume = value != 0;
    if (nvs_get_i32(reader_prefs, "sleeptime", &value) == ESP_OK
        && value >= 0 && value <= 7)
        reader_sleep_timer_choice = value;
    if (nvs_get_i32(reader_prefs, "sleepkey", &value) == ESP_OK)
        reader_sleep_timer_reset_on_key = value != 0;
    if (nvs_get_i32(reader_prefs, "sleepboot", &value) == ESP_OK)
        reader_sleep_timer_start_on_boot = value != 0;
    uint32_t status_mask;
    if (nvs_get_u32(reader_prefs, "statusmask", &status_mask) == ESP_OK)
        reader_status_enabled = status_mask & ((1U << READER_STATUS_COUNT) - 1);
    uint8_t saved_order[READER_STATUS_COUNT];
    size_t saved_order_size = sizeof(saved_order);
    if (nvs_get_blob(reader_prefs, "statusorder", saved_order,
                     &saved_order_size) == ESP_OK
        && saved_order_size == sizeof(saved_order)) {
        uint32_t seen = 0;
        bool valid = true;
        for (size_t i = 0; i < READER_STATUS_COUNT; ++i) {
            if (saved_order[i] >= READER_STATUS_COUNT
                || (seen & (1U << saved_order[i]))) {
                valid = false;
                break;
            }
            seen |= 1U << saved_order[i];
        }
        if (valid)
            memcpy(reader_status_order, saved_order, sizeof(saved_order));
    }
    printf("PREFERENCES rate=%d volume=%d substitutions=%d dictionary_commands=%d interface_sounds=%d remaining_scope=%s section_pause=%d startup_resume=%d sleep_choice=%u sleep_key=%d sleep_boot=%d\n",
           reader_rate, reader_volume, reader_substitutions_enabled,
           reader_dictionary_commands,
           reader_interface_sounds,
           reader_remaining_scope_section ? "section" : "document",
           reader_pause_between_sections, reader_startup_resume,
           reader_sleep_timer_choice, reader_sleep_timer_reset_on_key,
           reader_sleep_timer_start_on_boot);
}

static uint32_t reader_path_hash(const char *path)
{
    uint32_t hash = UINT32_C(2166136261);
    for (const unsigned char *p = (const unsigned char *)path; *p; ++p) {
        hash ^= *p;
        hash *= UINT32_C(16777619);
    }
    return hash;
}

static void reader_section_key(char key[16])
{
    snprintf(key, 16, "pos%08x", (unsigned)reader_path_hash(reader_selected_path));
}

static void reader_offset_key(char key[16])
{
    snprintf(key, 16, "off%08x", (unsigned)reader_path_hash(reader_selected_path));
}

static void reader_save_section(void)
{
    if (!reader_prefs || !reader_selected_path[0]) return;
    char key[16]; reader_section_key(key);
    (void)nvs_set_u32(reader_prefs, key, (uint32_t)reader_section_index);
    reader_offset_key(key);
    (void)nvs_set_u32(reader_prefs, key, reader_saved_text_offset);
    (void)nvs_commit(reader_prefs);
}

static size_t reader_position_text_offset(void)
{
    size_t position = reader_section_base_offset + reader_discarded_text_offset;
    xSemaphoreTake(reader_pcm_mutex, portMAX_DELAY);
    size_t count = reader_marker_count;
    for (size_t i = 0; i < count; ++i) {
        size_t sample = reader_markers[i].sample_offset;
        if (sample == SIZE_MAX || sample > reader_play_offset) break;
        position = reader_section_base_offset + reader_markers[i].text_offset;
    }
    xSemaphoreGive(reader_pcm_mutex);
    return position;
}

static void reader_map_absolute_position(size_t absolute)
{
    for (size_t i = 0; i < reader_document.section_count; ++i) {
        epub_section_t *section = &reader_document.sections[i];
        size_t end = section->text_offset + section->text_length;
        if (absolute < end || i + 1 == reader_document.section_count) {
            reader_section_index = i;
            reader_saved_text_offset = absolute > section->text_offset
                ? (uint32_t)(absolute - section->text_offset) : 0;
            if (reader_saved_text_offset > section->text_length)
                reader_saved_text_offset = section->text_length;
            return;
        }
    }
}

static void reader_save_position(void)
{
    if (!reader_prefs || !reader_selected_path[0]) return;
    size_t relative = reader_position_text_offset();
    size_t absolute = reader_stream_base_absolute
        + (relative - reader_section_base_offset);
    reader_map_absolute_position(absolute);
    char section_key[16], offset_key[16];
    reader_section_key(section_key); reader_offset_key(offset_key);
    (void)nvs_set_u32(reader_prefs, section_key,
                      (uint32_t)reader_section_index);
    (void)nvs_set_u32(reader_prefs, offset_key, reader_saved_text_offset);
    (void)nvs_commit(reader_prefs);
    printf("POSITION_SAVED section=%u text_offset=%u\n",
           (unsigned)reader_section_index + 1,
           (unsigned)reader_saved_text_offset);
}

static void reader_read_saved_section(void)
{
    reader_saved_section = 0;
    reader_saved_text_offset = 0;
    if (!reader_prefs || !reader_selected_path[0]) return;
    char key[16]; reader_section_key(key);
    (void)nvs_get_u32(reader_prefs, key, &reader_saved_section);
    reader_offset_key(key);
    (void)nvs_get_u32(reader_prefs, key, &reader_saved_text_offset);
}

static void reader_select_section_text(void)
{
    if (!reader_document.section_count) return;
    epub_section_t *section = &reader_document.sections[reader_section_index];
    reader_section_base_offset = reader_saved_text_offset < section->text_length
        ? reader_saved_text_offset : 0;
    reader_stream_base_absolute = section->text_offset
        + reader_section_base_offset;
    reader_book_text = reader_document.text + reader_stream_base_absolute;
    /* Stream continuously from the chosen section through the end of the
       document. Section boundaries remain available for paused navigation,
       but no longer create a six-second synthesis gap during linear reading. */
    reader_book_length = reader_document.text_length
        - reader_stream_base_absolute;
    printf("SECTION selected=%u count=%u stream_bytes=%u\n",
           (unsigned)reader_section_index + 1,
           (unsigned)reader_document.section_count,
           (unsigned)reader_book_length);
}

static void reader_move_section(int direction)
{
    if (!reader_document.section_count) return;
    if ((direction < 0 && reader_section_index == 0)
        || (direction > 0
            && reader_section_index + 1 >= reader_document.section_count)) {
        strlcpy(reader_announcement,
                direction < 0 ? "first section" : "last section",
                sizeof(reader_announcement));
        queue_audio(AUDIO_SPEECH, reader_announcement);
        return;
    }
    reader_finished = false;
    reader_section_index = direction < 0
        ? reader_section_index - 1 : reader_section_index + 1;
    reader_saved_text_offset = 0;
    reader_save_section();
    reader_section_change_pending = true;
    reader_section_announcement = false;
    reader_loaded = false;
    reader_pcm_valid = false;
    const char *heading =
        reader_document.sections[reader_section_index].name;
    if (heading[0])
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "%s. %u of %u", heading,
                 (unsigned)reader_section_index + 1,
                 (unsigned)reader_document.section_count);
    else
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "%u of %u", (unsigned)reader_section_index + 1,
                 (unsigned)reader_document.section_count);
    queue_audio(AUDIO_NAVIGATION_SPEECH, reader_announcement);
}

static bool reader_has_epub_extension(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && !strcasecmp(dot, ".epub");
}

static void reader_refresh_library(void)
{
    reader_library_count = 0;
    reader_library_index = 0;
    DIR *directory = opendir("/sdcard");
    if (!directory) return;
    struct dirent *entry;
    while (reader_library_count < 16 && (entry = readdir(directory))) {
        if (!reader_has_epub_extension(entry->d_name)) continue;
        strlcpy(reader_library_names[reader_library_count++], entry->d_name,
                sizeof(reader_library_names[0]));
    }
    closedir(directory);
    printf("LIBRARY_REFRESH documents=%u\n", (unsigned)reader_library_count);
}

static esp_err_t reader_mount_library(void)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    /* Keep one sector of DMA-capable memory permanently attached to the card.
       OpenEVV heavily fragments internal RAM after startup; without this the
       SDMMC driver may be unable to allocate its temporary bounce buffer when
       a different EPUB is opened. */
    host.flags |= SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF;
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = GPIO_NUM_39;
    slot.cmd = GPIO_NUM_38;
    slot.d0 = GPIO_NUM_40;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *mounted_card = NULL;
    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot,
                                                &mount, &mounted_card),
                        TAG, "mount reader card");
    reader_refresh_library();
    return ESP_OK;
}

static void reader_speak_library_item(void)
{
    if (!reader_library_count) {
        strlcpy(reader_announcement, "library empty", sizeof(reader_announcement));
    } else {
        snprintf(reader_announcement, sizeof(reader_announcement), "%s%s",
                 reader_library_transition_announcement ? "library. " : "",
                 reader_library_names[reader_library_index]);
        char *extension = strrchr(reader_announcement, '.');
        if (extension && !strcasecmp(extension, ".epub"))
            *extension = 0;
    }
    reader_library_transition_announcement = false;
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_format_menu_item(void)
{
    static const unsigned sleep_minutes[] = {0, 5, 10, 15, 30, 45, 60, 90};
    if (reader_menu_index == 0)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "speaking rate. %d", reader_rate);
    else if (reader_menu_index == 1)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "volume. %d percent", reader_volume);
    else if (reader_menu_index == 2)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "substitution dictionary. %s",
                 reader_substitutions_enabled ? "on" : "off");
    else if (reader_menu_index == 3)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "dictionary voice commands. %s",
                 reader_dictionary_commands ? "on" : "off");
    else if (reader_menu_index == 4)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "interface sounds. %s",
                 reader_interface_sounds ? "on" : "off");
    else if (reader_menu_index == 5)
        strlcpy(reader_announcement, "playing screen status items",
                sizeof(reader_announcement));
    else if (reader_menu_index == 6)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "pause between sections. %s",
                 reader_pause_between_sections ? "on" : "off");
    else if (reader_menu_index == 7)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "on startup. %s",
                 reader_startup_resume ? "resume reading" : "library");
    else if (reader_menu_index == 8) {
        if (reader_sleep_timer_choice)
            snprintf(reader_announcement, sizeof(reader_announcement),
                     "sleep timer. %u minutes%s",
                     sleep_minutes[reader_sleep_timer_choice],
                     reader_sleep_timer_active ? ". running" : "");
        else
            strlcpy(reader_announcement, "sleep timer. off",
                    sizeof(reader_announcement));
    }
    else if (reader_menu_index == 9)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "keypress resets sleep timer. %s",
                 reader_sleep_timer_reset_on_key ? "on" : "off");
    else if (reader_menu_index == 10)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "start sleep timer on boot. %s",
                 reader_sleep_timer_start_on_boot ? "on" : "off");
    else if (reader_menu_index == 11)
        strlcpy(reader_announcement, "file transfer",
                sizeof(reader_announcement));
    else if (reader_menu_index == 12)
        strlcpy(reader_announcement, "NVDA remote",
                sizeof(reader_announcement));
    else if (reader_menu_index == 12)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "volume. %d percent", reader_volume);
    else
        strlcpy(reader_announcement, "close menu",
                sizeof(reader_announcement));
}

static void reader_speak_menu_item(void)
{
    reader_menu_announcement_pending = true;
    reader_menu_changed_at = xTaskGetTickCount();
}

static void reader_toggle_dictionary_commands(void)
{
    reader_dictionary_commands = !reader_dictionary_commands;
    reader_render_abort = true;
    reader_loaded = false;
    reader_pcm_valid = false;
    reader_save_preferences();
    snprintf(reader_announcement, sizeof(reader_announcement),
             "dictionary voice commands. %s",
             reader_dictionary_commands ? "on" : "off");
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_toggle_substitutions(void)
{
    reader_substitutions_enabled = !reader_substitutions_enabled;
    reader_render_abort = true;
    reader_loaded = false;
    reader_pcm_valid = false;
    reader_save_preferences();
    snprintf(reader_announcement, sizeof(reader_announcement),
             "substitution dictionary. %s",
             reader_substitutions_enabled ? "on" : "off");
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_toggle_interface_sounds(void)
{
    reader_interface_sounds = !reader_interface_sounds;
    reader_save_preferences();
    snprintf(reader_announcement, sizeof(reader_announcement),
             "interface sounds. %s",
             reader_interface_sounds ? "on" : "off");
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_toggle_remaining_scope(void)
{
    reader_remaining_scope_section = !reader_remaining_scope_section;
    reader_save_preferences();
    snprintf(reader_announcement, sizeof(reader_announcement),
             "time remaining. %s",
             reader_remaining_scope_section ? "current section"
                                            : "current document");
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static const char *reader_status_name(unsigned field)
{
    static const char *const names[READER_STATUS_COUNT] = {
        "time", "date", "remaining section duration",
        "remaining file duration", "battery level",
        "battery time estimate", "sleep time remaining",
        "remaining SD card space", "device temperature",
    };
    return field < READER_STATUS_COUNT ? names[field] : "status";
}

static void reader_format_status_menu_item(void)
{
    unsigned field = reader_status_order[reader_status_menu_index];
    snprintf(reader_announcement, sizeof(reader_announcement),
             "%s. %s. %u of %u", reader_status_name(field),
             reader_status_enabled & (1U << field) ? "on" : "off",
             (unsigned)reader_status_menu_index + 1, READER_STATUS_COUNT);
}

static void reader_speak_status_menu_item(void)
{
    reader_menu_announcement_pending = true;
    reader_menu_changed_at = xTaskGetTickCount();
}

static void reader_open_status_menu(void)
{
    reader_status_menu = true;
    reader_status_menu_index = 0;
    reader_speak_status_menu_item();
}

static void reader_close_status_menu(void)
{
    reader_status_menu = false;
    reader_speak_menu_item();
}

static void reader_toggle_status_item(void)
{
    unsigned field = reader_status_order[reader_status_menu_index];
    reader_status_enabled ^= 1U << field;
    reader_save_preferences();
    reader_speak_status_menu_item();
}

static void reader_move_status_item(int direction)
{
    size_t other;
    if (direction < 0) {
        if (!reader_status_menu_index) {
            reader_speak_status_menu_item();
            return;
        }
        other = reader_status_menu_index - 1;
    } else {
        if (reader_status_menu_index + 1 >= READER_STATUS_COUNT) {
            reader_speak_status_menu_item();
            return;
        }
        other = reader_status_menu_index + 1;
    }
    uint8_t item = reader_status_order[reader_status_menu_index];
    reader_status_order[reader_status_menu_index] = reader_status_order[other];
    reader_status_order[other] = item;
    reader_status_menu_index = other;
    reader_save_preferences();
    reader_speak_status_menu_item();
}

static void reader_toggle_section_pause(void)
{
    reader_pause_between_sections = !reader_pause_between_sections;
    reader_save_preferences();
    snprintf(reader_announcement, sizeof(reader_announcement),
             "pause between sections. %s",
             reader_pause_between_sections ? "on" : "off");
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_toggle_startup(void)
{
    reader_startup_resume = !reader_startup_resume;
    reader_save_preferences();
    snprintf(reader_announcement, sizeof(reader_announcement),
             "on startup. %s",
             reader_startup_resume ? "resume reading" : "library");
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static const unsigned reader_sleep_minutes[] = {0, 5, 10, 15, 30, 45, 60, 90};

static void reader_arm_sleep_timer(void)
{
    unsigned minutes = reader_sleep_minutes[reader_sleep_timer_choice];
    reader_sleep_timer_active = minutes != 0;
    if (reader_sleep_timer_active)
        reader_sleep_timer_deadline = xTaskGetTickCount()
            + pdMS_TO_TICKS(minutes * 60U * 1000U);
    printf("SLEEP_TIMER active=%d minutes=%u\n",
           reader_sleep_timer_active, minutes);
}

static void reader_change_sleep_timer(int direction)
{
    int choice = (int)reader_sleep_timer_choice + direction;
    if (choice < 0) choice = 7;
    if (choice > 7) choice = 0;
    reader_sleep_timer_choice = (uint8_t)choice;
    if (!reader_sleep_timer_choice)
        reader_sleep_timer_active = false;
    reader_save_preferences();
    reader_speak_menu_item();
}

static void reader_start_sleep_timer(void)
{
    reader_arm_sleep_timer();
    if (!reader_sleep_timer_choice) {
        strlcpy(reader_announcement, "sleep timer off",
                sizeof(reader_announcement));
    } else {
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "sleep timer started. %u minutes",
                 reader_sleep_minutes[reader_sleep_timer_choice]);
    }
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_toggle_sleep_key_reset(void)
{
    reader_sleep_timer_reset_on_key = !reader_sleep_timer_reset_on_key;
    reader_save_preferences();
    reader_speak_menu_item();
}

static void reader_toggle_sleep_start_on_boot(void)
{
    reader_sleep_timer_start_on_boot = !reader_sleep_timer_start_on_boot;
    reader_save_preferences();
    reader_speak_menu_item();
}

static bool reader_status_time(void)
{
    time_t now = time(NULL);
    struct tm local;
    if (now < 1704067200 || !localtime_r(&now, &local))
        return false;
    strftime(reader_announcement, sizeof(reader_announcement),
             "%l:%M %p", &local);
    return true;
}

static bool reader_status_remaining(bool section_scope)
{
    if (!reader_loaded || !reader_text_offset || !reader_total_pcm_generated)
        return false;
    size_t relative = reader_position_text_offset();
    size_t absolute = reader_stream_base_absolute
        + (relative - reader_section_base_offset);
    size_t end = reader_document.text_length;
    size_t position_section = reader_section_index;
    for (size_t i = 0; i < reader_document.section_count; ++i) {
        epub_section_t *candidate = &reader_document.sections[i];
        if (absolute < candidate->text_offset + candidate->text_length
            || i + 1 == reader_document.section_count) {
            position_section = i;
            break;
        }
    }
    if (section_scope
        && position_section < reader_document.section_count) {
        epub_section_t *section = &reader_document.sections[position_section];
        end = section->text_offset + section->text_length;
    }
    size_t remaining_bytes = end > absolute ? end - absolute : 0;
    uint64_t remaining_seconds =
        (uint64_t)remaining_bytes * reader_total_pcm_generated
        / reader_text_offset / SAMPLE_RATE;
    printf("TIME_REMAINING scope=%s section=%u absolute=%u end=%u bytes=%u generated_samples=%llu generated_bytes=%u seconds=%llu\n",
           section_scope ? "section" : "document",
           (unsigned)position_section + 1, (unsigned)absolute,
           (unsigned)end, (unsigned)remaining_bytes,
           (unsigned long long)reader_total_pcm_generated,
           (unsigned)reader_text_offset,
           (unsigned long long)remaining_seconds);
    unsigned minutes = (unsigned)((remaining_seconds + 30) / 60);
    if (minutes < 1) {
        strlcpy(reader_announcement, "less than one minute remaining",
                sizeof(reader_announcement));
    } else if (minutes < 60) {
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "%u minute%s remaining", minutes,
                 minutes == 1 ? "" : "s");
    } else {
        unsigned hours = minutes / 60;
        minutes %= 60;
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "%u hour%s, %u minute%s remaining", hours,
                 hours == 1 ? "" : "s", minutes,
                 minutes == 1 ? "" : "s");
    }
    return true;
}

static bool reader_status_sleep_timer(void)
{
    if (!reader_sleep_timer_active)
        return false;
    TickType_t ticks = reader_sleep_timer_deadline - xTaskGetTickCount();
    unsigned seconds = ticks / configTICK_RATE_HZ;
    unsigned minutes = (seconds + 59) / 60;
    snprintf(reader_announcement, sizeof(reader_announcement),
             "%u minute%s", minutes,
             minutes == 1 ? "" : "s");
    return true;
}

static bool reader_status_date(void)
{
    time_t now = time(NULL);
    struct tm local;
    if (now < 1704067200 || !localtime_r(&now, &local))
        return false;
    strftime(reader_announcement, sizeof(reader_announcement),
             "%A, %e %B %Y", &local);
    return true;
}

static bool reader_status_sd_space(void)
{
    uint64_t total_bytes, bytes;
    if (esp_vfs_fat_info("/sdcard", &total_bytes, &bytes) != ESP_OK)
        return false;
    unsigned megabytes = (unsigned)(bytes / (1024U * 1024U));
    if (megabytes >= 1024)
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "%u point %u gigabytes",
                 megabytes / 1024, (megabytes % 1024) / 102);
    else
        snprintf(reader_announcement, sizeof(reader_announcement),
                 "%u megabytes", megabytes);
    return true;
}

static bool reader_status_device_temperature(void)
{
    float celsius;
    if (!temperature_sensor
        || temperature_sensor_get_celsius(temperature_sensor, &celsius)
               != ESP_OK)
        return false;
    snprintf(reader_announcement, sizeof(reader_announcement),
             "%.1f degrees Celsius", (double)celsius);
    return true;
}

static bool reader_prepare_status(unsigned field)
{
    switch (field) {
    case READER_STATUS_TIME: return reader_status_time();
    case READER_STATUS_DATE: return reader_status_date();
    case READER_STATUS_SECTION_REMAINING: return reader_status_remaining(true);
    case READER_STATUS_FILE_REMAINING: return reader_status_remaining(false);
    case READER_STATUS_BATTERY_LEVEL:
    case READER_STATUS_BATTERY_TIME: return false;
    case READER_STATUS_SLEEP_REMAINING: return reader_status_sleep_timer();
    case READER_STATUS_SD_SPACE: return reader_status_sd_space();
    case READER_STATUS_DEVICE_TEMPERATURE:
        return reader_status_device_temperature();
    default: return false;
    }
}

static void reader_speak_next_status(void)
{
    TickType_t now = xTaskGetTickCount();
    size_t candidate = reader_status_last_press
            && now - reader_status_last_press <= pdMS_TO_TICKS(10000)
        ? (reader_status_index + 1) % READER_STATUS_COUNT : 0;
    reader_status_last_press = now;
    for (size_t tried = 0; tried < READER_STATUS_COUNT; ++tried) {
        unsigned field = reader_status_order[candidate];
        if ((reader_status_enabled & (1U << field))
            && reader_prepare_status(field)) {
            reader_status_index = candidate;
            queue_audio(AUDIO_SPEECH, reader_announcement);
            return;
        }
        candidate = (candidate + 1) % READER_STATUS_COUNT;
    }
    strlcpy(reader_announcement, "status unavailable",
            sizeof(reader_announcement));
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_open_menu(void)
{
    reader_menu = true;
    reader_menu_index = 0;
    reader_menu_announcement_pending = false;
    reader_ui_tone_pending = reader_interface_sounds;
    snprintf(reader_announcement, sizeof(reader_announcement),
             "menu. speaking rate. %d", reader_rate);
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_close_menu(void)
{
    reader_menu_announcement_pending = false;
    reader_status_menu = false;
    reader_menu = false;
    reader_ui_tone_pending = reader_interface_sounds;
    strlcpy(reader_announcement, reader_library ? "library" : "reading",
            sizeof(reader_announcement));
    queue_audio(AUDIO_SPEECH, reader_announcement);
}

static void reader_change_rate(int direction)
{
    int changed = reader_rate + direction * 5;
    if (changed < 60) changed = 60;
    if (changed > 140) changed = 140;
    reader_rate = changed;
    reader_loaded = false;
    reader_pcm_valid = false;
    reader_schedule_preferences_save();
    reader_speak_menu_item();
}

static void reader_change_volume(int direction)
{
    if (direction > 0) {
        if (reader_volume < 10) reader_volume += 2;
        else if (reader_volume < 100) reader_volume += 5;
        if (reader_volume > 100) reader_volume = 100;
    } else {
        if (reader_volume > 10) reader_volume -= 5;
        else if (reader_volume > 2) reader_volume -= 2;
    }
    reader_schedule_preferences_save();
    reader_speak_menu_item();
}

static void reader_put_le16(uint8_t *p, uint16_t value)
{
    p[0] = value & 0xff;
    p[1] = value >> 8;
}

static void reader_put_le32(uint8_t *p, uint32_t value)
{
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = value >> 24;
}

static void reader_make_wav_header(uint8_t header[44], uint32_t data_bytes)
{
    memset(header, 0, 44);
    memcpy(header, "RIFF", 4);
    reader_put_le32(header + 4, data_bytes + 36);
    memcpy(header + 8, "WAVEfmt ", 8);
    reader_put_le32(header + 16, 16);
    reader_put_le16(header + 20, 1);
    reader_put_le16(header + 22, 1);
    reader_put_le32(header + 24, 32000);
    reader_put_le32(header + 28, 32000 * 2);
    reader_put_le16(header + 32, 2);
    reader_put_le16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    reader_put_le32(header + 40, data_bytes);
}

static void reader_recording_task(void *argument)
{
    char *final_path = argument;
    puts("RECORDING task entered");
    char temporary_path[512];
    snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", final_path);
    FILE *output = fopen(temporary_path, "wb+");
    i2s_chan_handle_t rx = NULL;
    bool success = output != NULL;
    uint32_t data_bytes = 0;
    uint8_t header[44];
    reader_make_wav_header(header, 0);
    if (success && fwrite(header, 1, sizeof(header), output) != sizeof(header))
        success = false;

    i2s_chan_config_t channel =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    /* Internal DMA RAM is deliberately scarce while a book is cached.  The
       SD writer drains this small ring comfortably at 32 kHz. */
    channel.dma_desc_num = 4;
    channel.dma_frame_num = 64;
    esp_err_t i2s_result = ESP_OK;
    if (success && (i2s_result = i2s_new_channel(&channel, NULL, &rx)) != ESP_OK) {
        printf("RECORDING i2s_new_channel failed=%s free_internal=%u largest_dma=%u\n",
               esp_err_to_name(i2s_result),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
        success = false;
    }
    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(32000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_3,
            .ws = GPIO_NUM_14,
            .dout = I2S_GPIO_UNUSED,
            .din = GPIO_NUM_46,
            .invert_flags = { false, false, false },
        },
    };
    if (success && (i2s_result = i2s_channel_init_std_mode(rx, &config)) != ESP_OK) {
        printf("RECORDING i2s_init failed=%s\n", esp_err_to_name(i2s_result));
        success = false;
    }
    if (success && (i2s_result = i2s_channel_enable(rx)) != ESP_OK) {
        printf("RECORDING i2s_enable failed=%s\n", esp_err_to_name(i2s_result));
        success = false;
    }
    if (success)
        puts("RECORDING microphone active");

    int32_t input[512];
    int16_t mono[256];
    while (success && !reader_recording_stop) {
        size_t bytes_read = 0;
        esp_err_t result = i2s_channel_read(rx, input, sizeof(input),
                                             &bytes_read,
                                             pdMS_TO_TICKS(200));
        if (result == ESP_ERR_TIMEOUT)
            continue;
        if (result != ESP_OK) {
            success = false;
            break;
        }
        size_t frames = bytes_read / (2 * sizeof(int32_t));
        for (size_t i = 0; i < frames; ++i) {
            int32_t left = input[i * 2], right = input[i * 2 + 1];
            int32_t sample = llabs((long long)left) >= llabs((long long)right)
                ? left : right;
            sample >>= 14;
            if (sample > INT16_MAX) sample = INT16_MAX;
            if (sample < INT16_MIN) sample = INT16_MIN;
            mono[i] = (int16_t)sample;
        }
        size_t written = fwrite(mono, sizeof(int16_t), frames, output);
        data_bytes += written * sizeof(int16_t);
        if (written != frames)
            success = false;
    }
    if (rx) {
        puts("RECORDING stopping microphone");
        (void)i2s_channel_disable(rx);
        (void)i2s_del_channel(rx);
    }
    if (output) {
        reader_make_wav_header(header, data_bytes);
        if (fseek(output, 0, SEEK_SET) ||
            fwrite(header, 1, sizeof(header), output) != sizeof(header))
            success = false;
        if (fflush(output) || fsync(fileno(output)))
            success = false;
        fclose(output);
    }
    if (success && data_bytes && rename(temporary_path, final_path) == 0) {
        printf("RECORDING saved=%s bytes=%u\n", final_path,
               (unsigned)data_bytes);
        if (reader_interface_sounds)
            queue_audio(AUDIO_RECORDING_STOPPED, NULL);
    } else {
        printf("RECORDING failed path=%s bytes=%u errno=%d\n", final_path,
               (unsigned)data_bytes, errno);
        (void)unlink(temporary_path);
        strlcpy(reader_announcement, "recording could not be saved",
                sizeof(reader_announcement));
        queue_audio(AUDIO_SPEECH, reader_announcement);
    }
    free(final_path);
    reader_recording = false;
    reader_recording_task_handle = NULL;
    /* This task's stack is allocated by xTaskCreateWithCaps() in PSRAM.
       It must be released by the matching capability-aware deletion API;
       the ordinary FreeRTOS deleter corrupts/aborts during recording stop. */
    puts("RECORDING task complete");
    vTaskDeleteWithCaps(NULL);
}

static bool reader_begin_recording(void)
{
    if (!reader_selected_path[0] || reader_recording)
        return false;
    if (mkdir("/sdcard/.evv", 0777) != 0 && errno != EEXIST)
        return false;
    if (mkdir("/sdcard/.evv/recordings", 0777) != 0 && errno != EEXIST)
        return false;
    const char *base = strrchr(reader_selected_path, '/');
    base = base ? base + 1 : reader_selected_path;
    char book[180];
    strlcpy(book, base, sizeof(book));
    char *extension = strrchr(book, '.');
    if (extension) *extension = 0;
    for (char *p = book; *p; ++p) {
        if ((unsigned char)*p < 32 || strchr("\\/:*?\"<>|", *p))
            *p = '_';
    }
    size_t length = strlen(book);
    while (length && (book[length - 1] == ' ' || book[length - 1] == '.'))
        book[--length] = 0;
    if (!book[0]) strlcpy(book, "book", sizeof(book));
    char folder[384];
    snprintf(folder, sizeof(folder), "/sdcard/.evv/recordings/%s", book);
    if (mkdir(folder, 0777) != 0 && errno != EEXIST)
        return false;

    char stamp[32];
    time_t now = time(NULL);
    struct tm local;
    bool clock_valid = now >= 1704067200 && localtime_r(&now, &local);
    if (clock_valid)
        strftime(stamp, sizeof(stamp), "%Y-%m-%d-%H%M", &local);
    char *path = malloc(512);
    if (!path) return false;
    if (clock_valid) {
        snprintf(path, 512, "%s/%s.wav", folder, stamp);
        for (unsigned duplicate = 2; access(path, F_OK) == 0; ++duplicate)
            snprintf(path, 512, "%s/%s-%u.wav", folder, stamp, duplicate);
    } else {
        for (unsigned sequence = 1;; ++sequence) {
            snprintf(path, 512, "%s/recording-%04u.wav", folder, sequence);
            if (access(path, F_OK) != 0)
                break;
        }
    }
    reader_recording_stop = false;
    reader_recording = true;
    printf("RECORDING create free_internal=%u free_psram=%u\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    /* FAT writes, I2S and newlib's stdio path have a considerably deeper
       combined call stack than the capture buffers alone suggest.  The old
       6 KB stack overflowed as soon as the first microphone DMA block was
       handled.  Keep the larger stack in PSRAM so scarce internal RAM is not
       consumed. */
    BaseType_t created = xTaskCreateWithCaps(
        reader_recording_task, "recording", 16384, path, 4,
        &reader_recording_task_handle, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (created != pdPASS) {
        printf("RECORDING task create failed free_internal=%u free_psram=%u\n",
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        reader_recording = false;
        free(path);
        return false;
    }
    puts("RECORDING started");
    return true;
}
#endif

static void button_scan_task(void *argument)
{
    static const char *const announcements[] = {
        NULL,
        "centre press detected",
        "up press detected",
        "left press detected",
        "down press detected",
        "right press detected",
    };
    const ButtonContext *buttons = argument;
    int stable_key = 0;
    int candidate_key = 0;
    int candidate_count = 0;
    bool locked = false;
    bool centre_hold_handled = false;
    TickType_t centre_started = 0;
    bool left_hold_handled = false;
    TickType_t left_started = 0;
    bool up_hold_handled = false;
    TickType_t up_started = 0;

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
            int previous_key = stable_key;
            stable_key = candidate_key;
            printf("BUTTON key=%d mv=%d raw=%d state=%s\n", stable_key,
                   millivolts, raw, stable_key ? "pressed" : "released");

            if (stable_key == 1) {
                centre_started = xTaskGetTickCount();
                centre_hold_handled = false;
            } else if (stable_key == 0 && previous_key == 1) {
                if (!centre_hold_handled && !locked)
                    queue_audio(AUDIO_SPEECH, announcements[1]);
            } else if (stable_key != 0 && !locked) {
                queue_audio(AUDIO_SPEECH, announcements[stable_key]);
            } else if (stable_key != 0) {
                printf("BUTTON_IGNORED locked=1 key=%d\n", stable_key);
            }
        }

        if (stable_key == 1 && !centre_hold_handled && reader_paused
            && xTaskGetTickCount() - centre_started >= pdMS_TO_TICKS(500)) {
            locked = !locked;
            centre_hold_handled = true;
            printf("CONTROLS locked=%d\n", locked);
            queue_audio(locked ? AUDIO_LOCKED : AUDIO_UNLOCKED, NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

#ifdef PARAGRAPH_READER_MODE
static void reader_button_scan_task(void *argument)
{
    const ButtonContext *buttons = argument;
    int stable_key = 0;
    int candidate_key = 0;
    int candidate_count = 0;
    bool locked = false;
    bool centre_hold_handled = false;
    TickType_t centre_started = 0;
    bool left_hold_handled = false;
    TickType_t left_started = 0;
    bool up_hold_handled = false;
    bool up_opened_menu = false;
    TickType_t up_started = 0;
    bool right_hold_handled = false;
    TickType_t right_started = 0;
    bool library_announcement_pending = false;
    TickType_t library_changed_at = 0;
    bool suppressed_press = false;

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
            int previous_key = stable_key;
            stable_key = candidate_key;
            printf("BUTTON key=%d mv=%d raw=%d state=%s\n", stable_key,
                   millivolts, raw, stable_key ? "pressed" : "released");
            if (!reader_controls_ready) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            if (stable_key != 0 && reader_recording) {
                reader_recording_stop = true;
                suppressed_press = true;
                puts("RECORDING stop requested by keypress");
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            if (stable_key != 0 && reader_sleep_timer_active
                && reader_sleep_timer_reset_on_key) {
                reader_arm_sleep_timer();
                puts("SLEEP_TIMER reset by keypress");
            }
            if (reader_ui_busy && stable_key != 0
                && !(reader_menu
                     && (stable_key == 2 || stable_key == 3
                         || stable_key == 4 || stable_key == 5))
                && !(reader_library
                     && (stable_key == 2 || stable_key == 3
                         || stable_key == 4 || stable_key == 5))
                && !(!reader_paused && !reader_library && !reader_menu
                     && (stable_key == 2 || stable_key == 4))) {
                suppressed_press = true;
                if (stable_key == 1)
                    centre_hold_handled = true;
                if (stable_key == 3)
                    left_hold_handled = true;
                printf("BUTTON_COALESCED key=%d ui_busy=1\n", stable_key);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            if (stable_key == 0 && suppressed_press) {
                suppressed_press = false;
                continue;
            }
            if (stable_key != 0 && !locked)
                reader_keypress();
            if (stable_key == 1) {
                centre_started = xTaskGetTickCount();
                centre_hold_handled = false;
            } else if (stable_key == 2) {
                up_started = xTaskGetTickCount();
                up_hold_handled = false;
                up_opened_menu = false;
                if (!reader_library && !reader_menu && !reader_paused) {
                    if (reader_volume < 10) reader_volume += 2;
                    else if (reader_volume < 100) reader_volume += 5;
                    if (reader_volume > 100) reader_volume = 100;
                    reader_save_preferences();
                    printf("READER volume_up=%d\n", reader_volume);
                }
            } else if (stable_key == 3) {
                left_started = xTaskGetTickCount();
                left_hold_handled = false;
                if (!reader_paused && !reader_library && !reader_menu
                    && !locked)
                    reader_sentence_seek = -1;
            } else if (stable_key == 5
                       && !reader_menu && !reader_library) {
                right_started = xTaskGetTickCount();
                right_hold_handled = false;
                if (!reader_paused && !reader_library && !reader_menu
                    && !locked)
                    reader_sentence_seek = 1;
            } else if (stable_key == 0 && previous_key == 3
                       && !left_hold_handled && !locked) {
                if (reader_menu) {
                    if (reader_status_menu)
                        reader_move_status_item(-1);
                    else if (reader_menu_index == 0) reader_change_rate(-1);
                    else if (reader_menu_index == 1)
                        reader_change_volume(-1);
                    else if (reader_menu_index == 2)
                        reader_toggle_substitutions();
                    else if (reader_menu_index == 3)
                        reader_toggle_dictionary_commands();
                    else if (reader_menu_index == 4)
                        reader_toggle_interface_sounds();
                    else if (reader_menu_index == 6)
                        reader_toggle_section_pause();
                    else if (reader_menu_index == 7)
                        reader_toggle_startup();
                    else if (reader_menu_index == 8)
                        reader_change_sleep_timer(-1);
                    else if (reader_menu_index == 9)
                        reader_toggle_sleep_key_reset();
                    else if (reader_menu_index == 10)
                        reader_toggle_sleep_start_on_boot();
                    else if (reader_menu_index == 12)
                        reader_change_volume(-1);
                } else if (reader_library) {
                    snprintf(reader_announcement, sizeof(reader_announcement),
                             "SD card root. %u documents",
                             (unsigned)reader_library_count);
                    queue_audio(AUDIO_SPEECH, reader_announcement);
                }
                else if (!reader_library && reader_paused)
                    reader_move_section(-1);
            } else if (stable_key == 0 && previous_key == 2
                       && !up_hold_handled && !locked) {
                if (reader_menu) {
                    if (reader_status_menu) {
                        if (reader_status_menu_index > 0)
                            --reader_status_menu_index;
                        else
                            reader_status_menu_index = READER_STATUS_COUNT - 1;
                        reader_speak_status_menu_item();
                    } else {
                        if (reader_menu_index > 0) --reader_menu_index;
                        else reader_menu_index = READER_MENU_LAST;
                        reader_speak_menu_item();
                    }
                } else if (reader_library) {
                    if (reader_library_count && reader_library_index > 0)
                        --reader_library_index;
                    library_announcement_pending = true;
                    library_changed_at = xTaskGetTickCount();
                }
            } else if (stable_key == 0 && previous_key == 1
                       && !centre_hold_handled && !locked) {
                if (reader_menu) {
                    if (reader_status_menu)
                        reader_toggle_status_item();
                    else if (reader_menu_index == READER_MENU_LAST)
                        reader_close_menu();
                    else if (reader_menu_index == 11
                             || reader_menu_index == 12) {
                        reader_paused = true;
                        if (!reader_library && reader_loaded)
                            reader_save_position();
                        queue_audio(reader_menu_index == 12
                            ? AUDIO_SWITCH_REMOTE : AUDIO_SWITCH_TRANSFER,
                            NULL);
                    }
                    else if (reader_menu_index == 2)
                        reader_toggle_substitutions();
                    else if (reader_menu_index == 3)
                        reader_toggle_dictionary_commands();
                    else if (reader_menu_index == 4)
                        reader_toggle_interface_sounds();
                    else if (reader_menu_index == 5)
                        reader_open_status_menu();
                    else if (reader_menu_index == 6)
                        reader_toggle_section_pause();
                    else if (reader_menu_index == 7)
                        reader_toggle_startup();
                    else if (reader_menu_index == 8)
                        reader_start_sleep_timer();
                    else if (reader_menu_index == 9)
                        reader_toggle_sleep_key_reset();
                    else if (reader_menu_index == 10)
                        reader_toggle_sleep_start_on_boot();
                } else if (reader_library) {
                    library_announcement_pending = false;
                    if (reader_library_count) {
                        (void)nvs_set_str(reader_prefs, "lastbook",
                            reader_library_names[reader_library_index]);
                        (void)nvs_commit(reader_prefs);
                        snprintf(reader_selected_path,
                                 sizeof(reader_selected_path), "/sdcard/%s",
                                 reader_library_names[reader_library_index]);
                        reader_read_saved_section();
                        reader_open_pending = true;
                        reader_started = true;
                        reader_finished = false;
                        reader_paused = true;
                        reader_resume_after_load = true;
                        queue_audio(AUDIO_LOAD, NULL);
                        puts("LIBRARY open requested");
                    } else {
                        reader_speak_library_item();
                    }
                } else if (!reader_loaded) {
                    reader_started = true;
                    reader_finished = false;
                    reader_paused = true;
                    reader_resume_after_load = true;
                    queue_audio(AUDIO_LOAD, NULL);
                    puts("BOOK load requested");
                } else if (reader_paused) {
                    bool boundary_resume = reader_boundary_paused;
                    reader_boundary_paused = false;
                    reader_paused = false;
                    if (boundary_resume)
                        puts("READER continuing after section pause");
                    else {
                        queue_audio(AUDIO_PARAGRAPH, NULL);
                        puts("READER playing");
                    }
                } else {
                    reader_boundary_paused = false;
                    reader_paused = true;
                    reader_save_position();
                    /* Playback no longer drains the rolling PCM buffer.  Ask
                       the engine task to stop after its current short chunk;
                       otherwise it can wait forever for buffer space and the
                       resume event remains stuck behind that wait. */
                    reader_render_stop_requested = true;
                    ++requested_generation;
                    puts("READER paused");
                }
            } else if (stable_key == 0 && previous_key == 5
                       && !right_hold_handled && !locked
                       && !reader_menu && !reader_library && reader_paused) {
                reader_move_section(1);
            } else if (stable_key != 0 && locked) {
                printf("BUTTON_IGNORED locked=1 key=%d\n", stable_key);
            } else if (reader_menu && stable_key == 4) {
                if (reader_status_menu) {
                    if (reader_status_menu_index + 1 < READER_STATUS_COUNT)
                        ++reader_status_menu_index;
                    else
                        reader_status_menu_index = 0;
                    reader_speak_status_menu_item();
                } else {
                    if (reader_menu_index < READER_MENU_LAST) ++reader_menu_index;
                    else reader_menu_index = 0;
                    reader_speak_menu_item();
                }
            } else if (reader_menu && stable_key == 5) {
                if (reader_status_menu)
                    reader_move_status_item(1);
                else if (reader_menu_index == 0)
                    reader_change_rate(1);
                else if (reader_menu_index == 1)
                    reader_change_volume(1);
                else if (reader_menu_index == 2)
                    reader_toggle_substitutions();
                else if (reader_menu_index == 3)
                    reader_toggle_dictionary_commands();
                else if (reader_menu_index == 4)
                    reader_toggle_interface_sounds();
                else if (reader_menu_index == 6)
                    reader_toggle_section_pause();
                else if (reader_menu_index == 7)
                    reader_toggle_startup();
                else if (reader_menu_index == 8)
                    reader_change_sleep_timer(1);
                else if (reader_menu_index == 9)
                    reader_toggle_sleep_key_reset();
                else if (reader_menu_index == 10)
                    reader_toggle_sleep_start_on_boot();
                else if (reader_menu_index == 12)
                    reader_change_volume(1);
            } else if (reader_library && stable_key == 4) {
                if (reader_library_count
                    && reader_library_index + 1 < reader_library_count)
                    ++reader_library_index;
                library_announcement_pending = true;
                library_changed_at = xTaskGetTickCount();
            } else if (reader_library && stable_key == 5) {
                reader_speak_library_item();
            } else if (stable_key == 4) {
                if (!reader_paused) {
                    if (reader_volume > 10) reader_volume -= 5;
                    else if (reader_volume > 2) reader_volume -= 2;
                    reader_save_preferences();
                    printf("READER volume=%d\n", reader_volume);
                    continue;
                }
                reader_speak_next_status();
            } else if (stable_key == 5) {
                /* Paused-reading Right is handled on release so a hold can
                   start recording without first moving section. */
            }
        }
        /* A key used to stop recording is consumed through its release.
           Otherwise these continuously evaluated hold paths can act on it
           even though the edge handler above marked the press suppressed. */
        if (!suppressed_press && stable_key == 1 && !centre_hold_handled
            && xTaskGetTickCount() - centre_started >= pdMS_TO_TICKS(1200)) {
            locked = !locked;
            centre_hold_handled = true;
            /* Stop cleanly before the status sound; this also guarantees the
               release following a hold cannot become play/pause. */
            if (!reader_paused) {
                reader_boundary_paused = false;
                reader_resume_after_status = true;
                reader_paused = true;
                reader_save_position();
                ++requested_generation;
            }
            printf("CONTROLS locked=%d\n", locked);
            queue_audio(locked ? AUDIO_LOCKED : AUDIO_UNLOCKED, NULL);
        }
        if (!suppressed_press && stable_key == 5 && !right_hold_handled && !locked
            && !reader_menu && !reader_library && reader_paused
            && !reader_recording
            && xTaskGetTickCount() - right_started >= pdMS_TO_TICKS(1200)) {
            right_hold_handled = true;
            if (reader_interface_sounds)
                queue_audio(AUDIO_RECORDING_STARTED, NULL);
            else if (!reader_begin_recording()) {
                strlcpy(reader_announcement,
                        "recording could not be started",
                        sizeof(reader_announcement));
                queue_audio(AUDIO_SPEECH, reader_announcement);
            }
            puts("RECORDING start requested by hold right");
        }
        if (!suppressed_press && stable_key == 2 && !up_hold_handled
            && xTaskGetTickCount() - up_started >= pdMS_TO_TICKS(1200)) {
            up_hold_handled = true;
            if (reader_menu && !up_opened_menu) {
                if (reader_status_menu) {
                    reader_close_status_menu();
                    puts("STATUS_MENU closed by hold up");
                } else {
                    reader_close_menu();
                    puts("MENU closed by hold up");
                }
            } else if (!reader_menu && (reader_library || reader_paused)) {
                library_announcement_pending = false;
                reader_open_menu();
                up_opened_menu = true;
                puts("MENU opened by hold up");
            }
        }
        if (!suppressed_press && stable_key == 3 && !left_hold_handled && !locked
            && reader_menu && reader_status_menu
            && xTaskGetTickCount() - left_started >= pdMS_TO_TICKS(1200)) {
            left_hold_handled = true;
            reader_close_status_menu();
            puts("STATUS_MENU closed by hold left");
        }
        if (!suppressed_press && stable_key == 3 && !left_hold_handled && !locked
            && !reader_menu && (reader_library || reader_paused)
            && xTaskGetTickCount() - left_started >= pdMS_TO_TICKS(1200)) {
            left_hold_handled = true;
            if (reader_library) {
                reader_library = false;
                reader_ui_tone_pending = reader_interface_sounds;
                strlcpy(reader_announcement, "reading",
                        sizeof(reader_announcement));
                queue_audio(AUDIO_SPEECH, reader_announcement);
                puts("LIBRARY returned to cached reading");
                continue;
            }
            reader_save_position();
            reader_library = true;
            reader_ui_tone_pending = reader_interface_sounds;
            reader_library_transition_announcement = true;
            for (size_t i = 0; i < reader_library_count; ++i) {
                const char *name = strrchr(reader_selected_path, '/');
                if (name && !strcmp(reader_library_names[i], name + 1)) {
                    reader_library_index = i;
                    break;
                }
            }
            reader_speak_library_item();
            puts("READER library opened");
        }
        /* Rapid movement should speak the item on which the user lands, not
           repeatedly start and cut off every intermediate filename. */
        if (library_announcement_pending && stable_key == 0
            && xTaskGetTickCount() - library_changed_at
                   >= pdMS_TO_TICKS(180)) {
            library_announcement_pending = false;
            reader_speak_library_item();
        }
        /* Menu movement and repeated value changes are deliberately silent
           while the stick is busy.  Compose the text only after landing so
           an in-flight utterance never observes a mutated shared buffer. */
        if (reader_menu_announcement_pending && stable_key == 0
            && !reader_ui_busy
            && xTaskGetTickCount() - reader_menu_changed_at
                   >= pdMS_TO_TICKS(250)) {
            reader_menu_announcement_pending = false;
            if (reader_menu) {
                if (reader_status_menu)
                    reader_format_status_menu_item();
                else
                    reader_format_menu_item();
                queue_audio(AUDIO_SPEECH, reader_announcement);
            }
        }
        if (reader_preferences_dirty && !reader_menu_announcement_pending
            && !reader_ui_busy
            && xTaskGetTickCount() - reader_preferences_changed_at
                   >= pdMS_TO_TICKS(1000)) {
            reader_preferences_dirty = false;
            reader_save_preferences();
            puts("PREFERENCES saved after input burst");
        }
        if (reader_sleep_timer_active
            && (int32_t)(xTaskGetTickCount()
                         - reader_sleep_timer_deadline) >= 0) {
            reader_sleep_timer_active = false;
            puts("SLEEP_TIMER expired");
            if (!reader_paused) {
                reader_boundary_paused = false;
                reader_paused = true;
                reader_save_position();
                reader_render_stop_requested = true;
                ++requested_generation;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
#endif

#ifndef PARAGRAPH_READER_MODE
void app_main(void)
{
    adc_oneshot_unit_handle_t adc;
    adc_cali_handle_t calibration = NULL;
    OldInst *instance;

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
    instance = eo_new();
    if (instance == NULL) {
        ESP_LOGE(TAG, "OpenEVV initialization failed");
        return;
    }
    eo_registerCallback(instance, on_message, NULL);
    ESP_ERROR_CHECK(ev_setOutputBuffer(instance, FRAME_SAMPLES, mono_frame)
                        ? ESP_OK : ESP_FAIL);
    /* Deliberately relaxed for the first public hardware demonstration. */
    ESP_ERROR_CHECK(vc_setVoiceParam(instance, 0, VOICE_SPEED, 90)
                        ? ESP_OK : ESP_FAIL);

    audio_queue = xQueueCreate(1, sizeof(AudioEvent));
    ESP_ERROR_CHECK(audio_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    static ButtonContext buttons;
    buttons.adc = adc;
    buttons.calibration = calibration;
    BaseType_t task_created = xTaskCreate(button_scan_task, "buttons", 4096,
                                           &buttons, 5, NULL);
    ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);

    puts("BUTTON_SPEECH_READY mapping=centre,up,left,down,right gpio=19 hold_ms=500 interruptible=1");
    queue_audio(AUDIO_SPEECH, "ready for button demonstration");

    for (;;) {
        AudioEvent event;
        if (xQueueReceive(audio_queue, &event, portMAX_DELAY) == pdTRUE) {
            active_generation = event.generation;
            if (event.kind == AUDIO_SPEECH) {
                (void)speak(instance, event.text, event.generation);
            } else if (event.kind == AUDIO_LOCKED) {
                (void)play_wav_tone(locked_wav_start, locked_wav_end,
                                    event.generation);
            } else {
                (void)play_wav_tone(unlocked_wav_start, unlocked_wav_end,
                                    event.generation);
            }
        }
    }
}
#else
static OldInst *reader_create_engine(void)
{
    OldInst *instance = eo_new();
    if (!instance)
        return NULL;
    eo_registerCallback(instance, on_message, NULL);
    if (!ev_setOutputBuffer(instance, FRAME_SAMPLES, mono_frame)
        || !vc_setVoiceParam(instance, 0, VOICE_SPEED, reader_rate)
        || vc_setVoiceParam(instance, 0, VOICE_VOLUME,
                            ENGINE_VOLUME_MAX) < 0) {
        (void)es_delete(instance);
        return NULL;
    }
    OI_ENV(instance)[ENV_PHRASE_PREDICTION] = 0;
    OI_ENV_SAVED(instance)[ENV_PHRASE_PREDICTION] = 1;
    if (api_set_param(OI_NEW(instance), 0,
                      PARAM_PHRASE_PREDICTION, 0) != 0) {
        (void)es_delete(instance);
        return NULL;
    }
    return instance;
}

#define BOOT_REQUEST_TRANSFER UINT32_C(0x45565654)

typedef enum {
    NETWORK_CONFIG_OK,
    NETWORK_CONFIG_MISSING,
    NETWORK_CONFIG_INVALID,
} network_config_result_t;

static network_config_result_t reader_check_key_file(
    const char *const *paths, size_t path_count, const char *first_key,
    const char *second_key)
{
    bool found = false;
    bool have_first = false;
    bool have_second = second_key == NULL;
    for (size_t path_index = 0; path_index < path_count; ++path_index) {
        FILE *file = fopen(paths[path_index], "rb");
        if (!file)
            continue;
        found = true;
        char line[384];
        while (fgets(line, sizeof(line), file)) {
            char *start = line;
            while (isspace((unsigned char)*start))
                ++start;
            char *end = start + strlen(start);
            while (end > start && isspace((unsigned char)end[-1]))
                *--end = 0;
            size_t first_length = strlen(first_key);
            size_t second_length = second_key ? strlen(second_key) : 0;
            if (!strncasecmp(start, first_key, first_length)
                && start[first_length] != 0)
                have_first = true;
            else if (second_key
                     && !strncasecmp(start, second_key, second_length)
                     && start[second_length] != 0)
                have_second = true;
        }
        fclose(file);
        if (have_first && have_second)
            return NETWORK_CONFIG_OK;
        have_first = false;
        have_second = second_key == NULL;
    }
    return found ? NETWORK_CONFIG_INVALID : NETWORK_CONFIG_MISSING;
}

static network_config_result_t reader_check_wifi_config(void)
{
    static const char *const paths[] = {
        "/sdcard/.evv/WIFI.INI", "/sdcard/.evv/wifi.ini",
        "/sdcard/EVVZERO/WIFI.INI", "/sdcard/EVVZERO/wifi.ini",
    };
    return reader_check_key_file(paths, sizeof(paths) / sizeof(paths[0]),
                                 "ssid=", "password=");
}

static network_config_result_t reader_check_remote_config(void)
{
    static const char *const paths[] = {
        "/sdcard/.evv/nvdaremote.ini",
        "/sdcard/.evv/NVDAREMOTE.INI",
    };
    network_config_result_t result = reader_check_key_file(
        paths, sizeof(paths) / sizeof(paths[0]), "host=", "key=");
    if (result != NETWORK_CONFIG_OK)
        return result;
    return reader_check_key_file(paths, sizeof(paths) / sizeof(paths[0]),
                                 "port=", NULL);
}

static void reader_engine_task(void *arg)
{
    (void)arg;
    OldInst *instance;

    evv_port_start();
    evvRunStaticInitialisers();
    instance = reader_create_engine();
    if (instance == NULL) {
        ESP_LOGE(TAG, "OpenEVV initialization failed");
        return;
    }
    /* The new API's general parameter 13 is `pp`, but the compatibility
       wrapper stores it in environment word 11 and its public setter refuses
       that word. Keep both representations aligned so a later environment
       flush cannot silently restore phrase prediction. */
    int32_t pp_rc = 0;
    printf("ENGINE_VOLUME value=%d PHRASE_PREDICTION value=%ld env=%ld rc=%ld\n",
           ENGINE_VOLUME_MAX,
           (long)0,
           (long)OI_ENV(instance)[ENV_PHRASE_PREDICTION],
           (long)pp_rc);

    reader_controls_ready = true;
    printf("ROLLING_READER_READY engine_core=%d centre=load_then_play left_right=volume up_down=rate\n",
           xPortGetCoreID());
    for (;;) {
        AudioEvent event;
        if (xQueueReceive(audio_queue, &event, portMAX_DELAY) != pdTRUE)
            continue;
        if (event.generation != requested_generation)
            continue;
        reader_ui_busy = event.kind != AUDIO_PARAGRAPH;
        if (event.kind != AUDIO_PARAGRAPH && reader_ui_tone_pending) {
            reader_ui_tone_pending = false;
            if (reader_interface_sounds) {
                if (!reader_discard_dma_audio()) {
                    ESP_LOGE(TAG, "could not reset audio output for UI tone");
                    continue;
                }
                (void)play_wav_tone(ui_wav_start, ui_wav_end,
                                    event.generation);
            }
        }
        if (event.kind == AUDIO_SWITCH_TRANSFER
            || event.kind == AUDIO_SWITCH_REMOTE) {
            bool remote = event.kind == AUDIO_SWITCH_REMOTE;
            network_config_result_t config = reader_check_wifi_config();
            const char *error = NULL;
            if (config == NETWORK_CONFIG_MISSING)
                error = "Wi-Fi settings file not found";
            else if (config == NETWORK_CONFIG_INVALID)
                error = "Wi-Fi settings file is invalid";
            else if (remote) {
                config = reader_check_remote_config();
                if (config == NETWORK_CONFIG_MISSING)
                    error = "NVDA remote settings file not found";
                else if (config == NETWORK_CONFIG_INVALID)
                    error = "NVDA remote settings file is invalid";
            }
            if (error) {
                printf("NETWORK_MODE_REJECTED mode=%s reason=%s\n",
                       remote ? "nvda_remote" : "webdav", error);
                (void)speak(instance, error, event.generation);
                reader_stop_audio();
                reader_ui_busy = false;
                continue;
            }
            if (reader_rendering)
                reader_finish_book_render(instance);
            (void)reader_discard_dma_audio();
            strlcpy(reader_announcement,
                    remote ? "NVDA remote" : "file transfer",
                    sizeof(reader_announcement));
            (void)speak(instance, reader_announcement, event.generation);
            reader_stop_audio();
            FILE *mode = fopen("/sdcard/.evv/network.mode", "wb");
            if (mode) {
                fputs(remote ? "nvda\n" : "webdav\n", mode);
                fclose(mode);
            }
            REG_WRITE(RTC_CNTL_STORE1_REG, remote ? 1 : 0);
            REG_WRITE(RTC_CNTL_STORE0_REG, BOOT_REQUEST_TRANSFER);
            printf("SWITCHING_TO_NETWORK mode=%s staged_restart=1\n",
                   remote ? "nvda_remote" : "webdav");
            esp_restart();
        }
        if (event.kind == AUDIO_LOAD) {
            int64_t load_started_us = esp_timer_get_time();
            int64_t opening_started_us = 0;
            bool opening_started = false;
            if (!reader_discard_dma_audio()) {
                ESP_LOGE(TAG, "could not reset audio output");
                continue;
            }
            if (reader_open_pending) {
                opening_started_us = esp_timer_get_time();
                opening_started = reader_start_opening_tone(event.generation);
            }
            if (reader_rendering) {
                reader_finish_book_render(instance);
            }
            if (reader_section_change_pending) {
                reader_section_change_pending = false;
                reader_select_section_text();
            }
            if (reader_open_pending) {
                epub_free_document(&reader_document);
                esp_err_t open_result = epub_load_document(
                    reader_selected_path, &reader_document);
                if (open_result != ESP_OK) {
                    ESP_LOGE(TAG, "EPUB open failed: %s",
                             esp_err_to_name(open_result));
                    reader_open_pending = false;
                    reader_library = true;
                    strlcpy(reader_announcement, "this book could not be opened",
                            sizeof(reader_announcement));
                    (void)speak(instance, reader_announcement, event.generation);
                    reader_stop_audio();
                    continue;
                }
                reader_section_index = reader_saved_section
                        < reader_document.section_count
                    ? reader_saved_section : 0;
                while (reader_section_index + 1 < reader_document.section_count
                       && reader_document.sections[reader_section_index]
                                  .text_length < 20)
                    ++reader_section_index;
                reader_select_section_text();
                reader_open_pending = false;
                reader_library = false;
                printf("EPUB_READY title=%s text_bytes=%u sections=%u\n",
                       reader_document.title,
                       (unsigned)reader_document.text_length,
                       (unsigned)reader_document.section_count);
            }
            if (reader_section_announcement) {
                reader_section_announcement = false;
                const char *heading =
                    reader_document.sections[reader_section_index].name;
                if (heading[0])
                    snprintf(reader_announcement,
                             sizeof(reader_announcement), "%s. %u of %u",
                             heading, (unsigned)reader_section_index + 1,
                             (unsigned)reader_document.section_count);
                else
                    snprintf(reader_announcement,
                             sizeof(reader_announcement), "%u of %u",
                             (unsigned)reader_section_index + 1,
                             (unsigned)reader_document.section_count);
                (void)speak(instance, reader_announcement, event.generation);
                reader_stop_audio();
            }
            if (!opening_started) {
                opening_started_us = esp_timer_get_time();
                opening_started = reader_start_opening_tone(event.generation);
            }
            int64_t synthesis_started_us = esp_timer_get_time();
            ESP_ERROR_CHECK(vc_setVoiceParam(instance, 0, VOICE_SPEED,
                                             reader_rate)
                                ? ESP_OK : ESP_FAIL);
            if (!reader_load_book(instance, event.generation)) {
                ESP_LOGE(TAG, "book load failed");
                reader_loaded = false;
                continue;
            }
            while (reader_opening_tone_running)
                vTaskDelay(pdMS_TO_TICKS(5));
            int64_t ready_us = esp_timer_get_time();
            printf("BOOK_OPEN_TIMING prework_ms=%u tone_overlap_ms=%u synth_ms=%u total_ms=%u initial_target_ms=%u\n",
                   (unsigned)((opening_started_us - load_started_us) / 1000),
                   (unsigned)((ready_us - opening_started_us) / 1000),
                   (unsigned)((ready_us - synthesis_started_us) / 1000),
                   (unsigned)((ready_us - load_started_us) / 1000),
                   ROLLING_START_MS);
            puts("BOOK ready");
            if (reader_resume_after_load) {
                reader_resume_after_load = false;
                reader_paused = false;
                queue_audio(AUDIO_PARAGRAPH, NULL);
                puts("READER continuing after section navigation");
            }
            reader_ui_busy = false;
            continue;
        }
        if (event.kind == AUDIO_NAVIGATION_SPEECH) {
            if (!reader_discard_dma_audio()) {
                ESP_LOGE(TAG, "could not reset audio output");
                continue;
            }
            if (reader_rendering)
                reader_finish_book_render(instance);
            ESP_ERROR_CHECK(vc_setVoiceParam(instance, 0, VOICE_SPEED,
                                             reader_rate)
                                ? ESP_OK : ESP_FAIL);
            (void)reader_speak_buffered(instance, event.text,
                                        event.generation);
            reader_ui_busy = false;
            continue;
        }
        if (event.kind != AUDIO_SPEECH && event.kind != AUDIO_PARAGRAPH) {
            if (!reader_discard_dma_audio()) {
                ESP_LOGE(TAG, "could not reset audio output");
                continue;
            }
            if (event.kind == AUDIO_LOCKED)
                (void)play_wav_tone(locked_wav_start, locked_wav_end,
                                    event.generation);
            else if (event.kind == AUDIO_UNLOCKED)
                (void)play_wav_tone(unlocked_wav_start, unlocked_wav_end,
                                    event.generation);
            else if (event.kind == AUDIO_VOLUME_MINIMUM)
                (void)play_wav_tone(volume_minimum_wav_start,
                                    volume_minimum_wav_end, event.generation);
            else if (event.kind == AUDIO_VOLUME_MAXIMUM)
                (void)play_wav_tone(volume_maximum_wav_start,
                                    volume_maximum_wav_end, event.generation);
            else if (event.kind == AUDIO_RECORDING_STARTED) {
                (void)play_wav_tone(recording_started_wav_start,
                                    recording_started_wav_end,
                                    event.generation);
                if (!reader_begin_recording()) {
                    strlcpy(reader_announcement,
                            "recording could not be started",
                            sizeof(reader_announcement));
                    queue_audio(AUDIO_SPEECH, reader_announcement);
                }
            } else if (event.kind == AUDIO_RECORDING_STOPPED)
                (void)play_wav_tone(recording_stopped_wav_start,
                                    recording_stopped_wav_end,
                                    event.generation);
            if (reader_resume_after_status
                && (event.kind == AUDIO_LOCKED
                    || event.kind == AUDIO_UNLOCKED)) {
                reader_resume_after_status = false;
                reader_paused = false;
                queue_audio(AUDIO_PARAGRAPH, NULL);
                puts("READER resumed after lock status");
            }
            reader_ui_busy = false;
            continue;
        }
        if (event.kind == AUDIO_SPEECH) {
            /* The engine instance is single-owner. Let background book
               rendering finish (or honour a rate-change abort) before using
               it for a spoken control announcement. */
            if (reader_rendering) {
                reader_finish_book_render(instance);
            }
            if (!reader_discard_dma_audio()) {
                ESP_LOGE(TAG, "could not reset audio output");
                continue;
            }
            ESP_ERROR_CHECK(vc_setVoiceParam(instance, 0, VOICE_SPEED, reader_rate)
                                ? ESP_OK : ESP_FAIL);
            (void)speak(instance, event.text, event.generation);
            reader_stop_audio();
            reader_render_abort = false;
            reader_ui_busy = false;
            continue;
        }
        if (reader_paused)
        {
            reader_ui_busy = false;
            continue;
        }
        if (!reader_loaded || !reader_pcm_valid
            || reader_pcm_rate != reader_rate) {
            ESP_LOGE(TAG, "book cache is not valid");
            reader_paused = true;
            continue;
        }
        if (reader_play_task_running)
            continue;
        if (reader_text_offset < reader_book_length)
            reader_production_complete = false;
        printf("PLAY_TASK free_internal=%u free_psram=%u\n",
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        BaseType_t play_created = xTaskCreatePinnedToCoreWithCaps(
            reader_play_task, "reader_play", 4096,
            (void *)(uintptr_t)event.generation, 4, NULL, 0,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (play_created != pdPASS) {
            ESP_LOGE(TAG, "could not start playback task");
            reader_paused = true;
            continue;
        }
        /* The engine must remain on this task, while the dedicated task above
           keeps I2S fed on the other core. */
        reader_finish_book_render(instance);
        reader_ui_busy = false;
    }
}

static bool reader_resume_last_book(void)
{
    if (!reader_startup_resume || !reader_prefs || !reader_library_count)
        return false;
    char last_book[256];
    size_t length = sizeof(last_book);
    if (nvs_get_str(reader_prefs, "lastbook", last_book, &length) != ESP_OK)
        return false;
    for (size_t i = 0; i < reader_library_count; ++i) {
        if (strcmp(reader_library_names[i], last_book))
            continue;
        reader_library_index = i;
        snprintf(reader_selected_path, sizeof(reader_selected_path),
                 "/sdcard/%s", last_book);
        reader_read_saved_section();
        reader_open_pending = true;
        reader_started = true;
        reader_finished = false;
        reader_paused = true;
        reader_resume_after_load = true;
        queue_audio(AUDIO_LOAD, NULL);
        puts("STARTUP resuming last book");
        return true;
    }
    return false;
}

void app_main(void)
{
    int64_t boot_started_us = esp_timer_get_time();
    setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0/2", 1);
    tzset();
    if (REG_READ(RTC_CNTL_STORE0_REG) == BOOT_REQUEST_TRANSFER) {
        REG_WRITE(RTC_CNTL_STORE0_REG, 0);
        const esp_partition_t *transfer = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0,
            "transfer");
        esp_err_t result = transfer
            ? esp_ota_set_boot_partition(transfer) : ESP_ERR_NOT_FOUND;
        printf("TRANSFER_BOOT_SELECT result=%s\n", esp_err_to_name(result));
        if (result == ESP_OK)
            esp_restart();
    }
    static ButtonContext buttons;
    adc_cali_handle_t calibration = NULL;

    /* These are staging buffers rather than DMA descriptors. Keeping them in
       PSRAM preserves scarce internal RAM for OpenEVV's deferred locks while
       allowing the faster 32 KB instruction-cache configuration. */
    mono_frame = heap_caps_malloc(FRAME_SAMPLES * sizeof(*mono_frame),
                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    stereo_frame = heap_caps_malloc(FRAME_SAMPLES * 2 * sizeof(*stereo_frame),
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    reader_playback_frame = heap_caps_malloc(
        FRAME_SAMPLES * 2 * sizeof(*reader_playback_frame),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(mono_frame && stereo_frame && reader_playback_frame
                    ? ESP_OK : ESP_ERR_NO_MEM);

    /* I2S is deliberately created on core 0. Its interrupt remains separate
       from the speech engine, which is created and owned on core 1 below. */
    ESP_ERROR_CHECK(init_audio());
    printf("BOOT_STAGE audio_ms=%u core=0\n",
           (unsigned)((esp_timer_get_time() - boot_started_us) / 1000));
    (void)play_wav_tone(unlocked_wav_start, unlocked_wav_end,
                        requested_generation);
    temperature_sensor_config_t temperature_config =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
    esp_err_t temperature_result = temperature_sensor_install(
        &temperature_config, &temperature_sensor);
    if (temperature_result == ESP_OK)
        temperature_result = temperature_sensor_enable(temperature_sensor);
    if (temperature_result != ESP_OK) {
        ESP_LOGW(TAG, "temperature sensor unavailable: %s",
                 esp_err_to_name(temperature_result));
        temperature_sensor = NULL;
    }
    reader_load_preferences();
    if (reader_sleep_timer_start_on_boot && reader_sleep_timer_choice)
        reader_arm_sleep_timer();
    printf("BOOT_STAGE preferences_ms=%u\n",
           (unsigned)((esp_timer_get_time() - boot_started_us) / 1000));
    esp_err_t library_result = reader_mount_library();
    if (library_result != ESP_OK)
        ESP_LOGE(TAG, "library mount failed: %s",
                 esp_err_to_name(library_result));
    else
        printf("LIBRARY_READY documents=%u\n", (unsigned)reader_library_count);
    if (library_result == ESP_OK) {
        reader_load_substitutions();
    }
    printf("BOOT_STAGE library_ms=%u\n",
           (unsigned)((esp_timer_get_time() - boot_started_us) / 1000));

    adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
        .unit_id = BUTTON_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &buttons.adc));
    adc_oneshot_chan_cfg_t adc_channel_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        buttons.adc, BUTTON_ADC_CHANNEL, &adc_channel_cfg));
    adc_cali_curve_fitting_config_t cal_cfg = {
        .unit_id = BUTTON_ADC_UNIT,
        .chan = BUTTON_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cal_cfg,
                                                          &calibration));
    buttons.calibration = calibration;

    reader_pcm_capacity = READER_PCM_BYTES / sizeof(int16_t);
    reader_pcm = heap_caps_malloc(reader_pcm_capacity * sizeof(int16_t),
                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(reader_pcm != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    printf("PCM_RING bytes=%u free_psram=%u\n",
           (unsigned)(reader_pcm_capacity * sizeof(int16_t)),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    reader_pcm_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(reader_pcm_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    reader_marker_capacity = 4096;
    reader_markers = heap_caps_calloc(reader_marker_capacity,
                                      sizeof(*reader_markers),
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(reader_markers != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    audio_queue = xQueueCreate(1, sizeof(AudioEvent));
    ESP_ERROR_CHECK(audio_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    BaseType_t engine_created = xTaskCreatePinnedToCoreWithCaps(
        reader_engine_task, "reader_engine", 65536, NULL, 4, NULL, 1,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    ESP_ERROR_CHECK(engine_created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    while (!reader_controls_ready)
        vTaskDelay(pdMS_TO_TICKS(10));
    printf("BOOT_STAGE engine_ms=%u\n",
           (unsigned)((esp_timer_get_time() - boot_started_us) / 1000));

    BaseType_t buttons_created = xTaskCreate(reader_button_scan_task,
                                              "reader_buttons", 4096,
                                              &buttons, 5, NULL);
    ESP_ERROR_CHECK(buttons_created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    if (library_result == ESP_OK) {
        if (!reader_resume_last_book())
            reader_speak_library_item();
    }
    else {
        strlcpy(reader_announcement, "SD card unavailable",
                sizeof(reader_announcement));
        queue_audio(AUDIO_SPEECH, reader_announcement);
    }
    printf("BOOT_STAGE request_ms=%u\n",
           (unsigned)((esp_timer_get_time() - boot_started_us) / 1000));
    for (;;)
        vTaskDelay(portMAX_DELAY);
}
#endif
