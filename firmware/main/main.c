#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "evv_abi.h"

typedef struct OldInst OldInst;

enum ECIMessage {
    eciWaveformBuffer,
    eciPhonemeBuffer,
    eciIndexReply
};

enum ECICallbackReturn {
    eciDataNotProcessed,
    eciDataProcessed,
    eciDataAbort
};

OldInst *STDCALL eo_new(void);
int STDCALL es_delete(OldInst *h);
int STDCALL et_addText(OldInst *h, const char *text);
int STDCALL et_synthesize(OldInst *h);
int STDCALL ev_setOutputBuffer(OldInst *h, int32_t n, void *buf);
void STDCALL eo_registerCallback(OldInst *h, void *cb, void *data);
void STDCALL eo_synchronizeSynth(OldInst *h);
int STDCALL eo_speaking(OldInst *h);
void evvRunStaticInitialisers(void);
void evv_port_start(void);

#define FRAME_SAMPLES 2048
#define SAMPLE_RATE 11025
#define EXPECTED_SAMPLES UINT64_C(49324)
#define EXPECTED_HASH UINT64_C(0x841804d4a4218b32)

static const char *const TAG = "openevv";
static int16_t frame[FRAME_SAMPLES];
static uint64_t sample_count;
static uint64_t pcm_hash;

/* FNV-1a over the exact little-endian PCM byte stream. */
static void hash_samples(const int16_t *samples, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        uint16_t value = (uint16_t)samples[i];
        pcm_hash ^= value & 0xffu;
        pcm_hash *= UINT64_C(1099511628211);
        pcm_hash ^= value >> 8;
        pcm_hash *= UINT64_C(1099511628211);
    }
    sample_count += count;
}

static enum ECICallbackReturn STDCALL on_message(OldInst *instance,
                                                  enum ECIMessage message,
                                                  long parameter,
                                                  void *context)
{
    (void)instance;
    (void)context;
    if (message == eciWaveformBuffer)
        hash_samples(frame, (size_t)parameter);
    return eciDataProcessed;
}

static void print_memory(const char *label)
{
    printf("MEMORY %s internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u\n",
           label,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

void app_main(void)
{
    static const char text[] =
        "Hello from Open E V V. This is a timing and memory test.";
    OldInst *instance;
    int64_t started;
    int64_t elapsed_us;
    double audio_seconds;

    puts("OPENEVV_VIABILITY version=1 target=esp32s3 rules=bytecode");
    print_memory("before_init");

    evv_port_start();
    evvRunStaticInitialisers();
    instance = eo_new();
    if (instance == NULL) {
        ESP_LOGE(TAG, "eo_new failed");
        return;
    }

    eo_registerCallback(instance, on_message, NULL);
    if (!ev_setOutputBuffer(instance, FRAME_SAMPLES, frame)) {
        ESP_LOGE(TAG, "ev_setOutputBuffer failed");
        es_delete(instance);
        return;
    }
    sample_count = 0;
    pcm_hash = UINT64_C(14695981039346656037);
    print_memory("before_synthesis");
    started = esp_timer_get_time();

    if (!et_addText(instance, text) || !et_synthesize(instance)) {
        ESP_LOGE(TAG, "synthesis request failed");
        es_delete(instance);
        return;
    }
    /* Public callbacks are delivered while the application queue is pumped,
       rather than directly from the synthesis worker.  eo_speaking() can be
       momentarily false before the worker accepts its queued request, so the
       sample stream itself is the reliable completion condition here. */
    while (sample_count < EXPECTED_SAMPLES
           && esp_timer_get_time() - started < 10000000) {
        (void)eo_speaking(instance);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    eo_synchronizeSynth(instance);
    elapsed_us = esp_timer_get_time() - started;
    audio_seconds = (double)sample_count / SAMPLE_RATE;

    print_memory("after_synthesis");
    printf("RESULT samples=%" PRIu64 " hash=%016" PRIx64
           " elapsed_ms=%.3f audio_s=%.3f realtime_x=%.3f\n",
           sample_count, pcm_hash, elapsed_us / 1000.0, audio_seconds,
           elapsed_us > 0 ? audio_seconds / (elapsed_us / 1000000.0) : 0.0);
    printf("VERDICT pcm_exact=%s realtime=%s\n",
           sample_count == EXPECTED_SAMPLES && pcm_hash == EXPECTED_HASH
               ? "PASS" : "FAIL",
           elapsed_us > 0 && audio_seconds > elapsed_us / 1000000.0
               ? "PASS" : "FAIL");

    es_delete(instance);
    print_memory("after_delete");
    puts("OPENEVV_VIABILITY done");
}
