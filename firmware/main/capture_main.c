#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "evv_abi.h"

typedef struct OldInst OldInst;

enum ECIMessage { eciWaveformBuffer, eciPhonemeBuffer, eciIndexReply };
enum ECICallbackReturn { eciDataNotProcessed, eciDataProcessed, eciDataAbort };

OldInst *STDCALL eo_new(void);
int STDCALL es_delete(OldInst *h);
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

#define FRAME_SAMPLES 2048
#define SAMPLE_RATE 11025
#define PCM_CAPACITY_SAMPLES (SAMPLE_RATE * 45)
#define VOICE_SPEED 6

static int16_t frame[FRAME_SAMPLES];
static int16_t *pcm;
static size_t pcm_samples;
static int pcm_overflow;

static void write_base64(const uint8_t *data, size_t size)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char out[1025];
    size_t at = 0;

    while (at < size) {
        size_t made = 0;
        while (at < size && made <= sizeof(out) - 5) {
            size_t left = size - at;
            uint32_t v = (uint32_t)data[at] << 16;
            if (left > 1)
                v |= (uint32_t)data[at + 1] << 8;
            if (left > 2)
                v |= data[at + 2];
            out[made++] = alphabet[(v >> 18) & 63];
            out[made++] = alphabet[(v >> 12) & 63];
            out[made++] = left > 1 ? alphabet[(v >> 6) & 63] : '=';
            out[made++] = left > 2 ? alphabet[v & 63] : '=';
            at += left >= 3 ? 3 : left;
        }
        out[made] = 0;
        puts(out);
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static enum ECICallbackReturn STDCALL on_message(OldInst *instance,
                                                  enum ECIMessage message,
                                                  long parameter,
                                                  void *context)
{
    size_t count = (size_t)parameter;
    (void)instance;
    (void)context;

    if (message != eciWaveformBuffer)
        return eciDataProcessed;
    if (pcm_samples + count > PCM_CAPACITY_SAMPLES) {
        pcm_overflow = 1;
        return eciDataAbort;
    }
    memcpy(pcm + pcm_samples, frame, count * sizeof(*frame));
    pcm_samples += count;
    return eciDataProcessed;
}

static int synthesize_pass(OldInst *instance, const char *label,
                           int speed, const char *paragraph)
{
    int64_t started;
    int64_t finished;
    size_t previous = 0;
    int speaking;

    if (!vc_setVoiceParam(instance, 0, VOICE_SPEED, speed))
        return 0;

    pcm_samples = 0;
    pcm_overflow = 0;
    started = esp_timer_get_time();
    if (!et_addText(instance, paragraph) || !et_synthesize(instance)) {
        return 0;
    }

    for (;;) {
        int64_t now = esp_timer_get_time();
        speaking = eo_speaking(instance);
        if (pcm_samples != previous) {
            previous = pcm_samples;
        }
        if (pcm_samples > 0 && !speaking)
            break;
        if (now - started >= 60000000) {
            printf("CAPTURE_TIMEOUT label=%s samples=%lu speaking=%d psram_free=%lu internal_free=%lu\n",
                   label, (unsigned long)pcm_samples, speaking,
                   (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                   (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    eo_synchronizeSynth(instance);
    finished = esp_timer_get_time();
    if (pcm_overflow)
        return 0;

    printf("B64_BEGIN label=%s samples=%lu rate=%d generation_us=%" PRId64 "\n",
           label, (unsigned long)pcm_samples, SAMPLE_RATE, finished - started);
    fflush(stdout);
    write_base64((const uint8_t *)pcm, pcm_samples * sizeof(*pcm));
    printf("B64_END label=%s\n", label);
    fflush(stdout);
    return 1;
}

void app_main(void)
{
    OldInst *instance;
    static const char paragraph[] =
        "This passage provides a repeatable test of English speech synthesis on "
        "the E S P thirty two S three. It includes short and long words, several "
        "phrases, and ordinary punctuation so that timing and sound can be compared "
        "at different speaking rates. Each pass uses exactly the same text. The "
        "captured samples therefore show how changes in speed affect generation "
        "time, playback duration, pronunciation, rhythm, and clarity.";
    static const struct {
        const char *label;
        int speed;
    } passes[] = {
        { "speed080", 80 },
        { "speed090", 90 },
        { "speed100", 100 },
        { "speed110", 110 },
        { "speed120", 120 },
        { "speed130", 130 },
        { "speed140", 140 },
        { "speed150", 150 },
        { "speed160", 160 },
        { "speed170", 170 },
        { "speed180", 180 },
    };
    puts("CAPTURE_READY version=3 rules=compiled icache=32k passes=11");
    pcm = heap_caps_malloc(PCM_CAPACITY_SAMPLES * sizeof(*pcm),
                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (pcm == NULL) {
        puts("CAPTURE_ERROR pcm_allocation");
        return;
    }

    evv_port_start();
    evvRunStaticInitialisers();
    printf("CAPTURE_MEMORY after_init psram_free=%lu internal_free=%lu\n",
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    instance = eo_new();
    printf("CAPTURE_MEMORY instance psram_free=%lu internal_free=%lu\n",
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    if (instance == NULL) {
        puts("CAPTURE_ERROR engine_init");
        return;
    }
    eo_registerCallback(instance, on_message, NULL);
    if (!ev_setOutputBuffer(instance, FRAME_SAMPLES, frame)) {
        puts("CAPTURE_ERROR output_buffer");
        es_delete(instance);
        return;
    }

    for (size_t i = 0; i < sizeof(passes) / sizeof(passes[0]); ++i) {
        if (!synthesize_pass(instance, passes[i].label, passes[i].speed,
                             paragraph)) {
            printf("CAPTURE_ERROR pass=%s\n", passes[i].label);
            break;
        }
    }
    es_delete(instance);
    puts("CAPTURE_DONE");
}
