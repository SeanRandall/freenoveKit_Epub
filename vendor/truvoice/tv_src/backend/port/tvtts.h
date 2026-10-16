#ifndef TRUVOICE_TVTTS_H
#define TRUVOICE_TVTTS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TVTTS_CALL
#define TVTTS_RATE_MIN 46
#define TVTTS_RATE_MAX 253
#define TVTTS_RATE_MAX_EXT 400
#define TVTTS_EXT_RATE 1u
#define TVTTS_EXT_ALL TVTTS_EXT_RATE

enum {
    TVTTS_AUDIO = 1,
    TVTTS_MARK = 2,
    TVTTS_END = 3
};

enum {
    TVTTS_SR_8K = 0,
    TVTTS_SR_11K = 1,
    TVTTS_SR_16K = 2
};

typedef struct tvtts_synth tvtts_synth;

typedef struct {
    int32_t type;
    uint32_t count;
    const int16_t *samples;
    uint32_t mark;
    uint32_t sample_pos;
} tvtts_event;

typedef int (TVTTS_CALL *tvtts_callback)(const tvtts_event *event, void *user);

tvtts_synth *TVTTS_CALL tvtts_create(uint32_t sample_rate);
void TVTTS_CALL tvtts_destroy(tvtts_synth *synth);
void TVTTS_CALL tvtts_set_voice(tvtts_synth *synth, int voice);
void TVTTS_CALL tvtts_set_rate(tvtts_synth *synth, int wpm);
void TVTTS_CALL tvtts_set_pitch(tvtts_synth *synth, int pitch);
void TVTTS_CALL tvtts_set_volume(tvtts_synth *synth, uint32_t volume);
int TVTTS_CALL tvtts_get_voice(const tvtts_synth *synth);
int TVTTS_CALL tvtts_get_rate(const tvtts_synth *synth);
int TVTTS_CALL tvtts_get_pitch(const tvtts_synth *synth);
int TVTTS_CALL tvtts_voice_count(void);
const char *TVTTS_CALL tvtts_voice_name(int voice);
int TVTTS_CALL tvtts_voice_rate(int voice);
int TVTTS_CALL tvtts_voice_pitch(int voice);
int TVTTS_CALL tvtts_speak_utf8(tvtts_synth *synth, const char *text,
                                tvtts_callback callback, void *user);

#ifdef __cplusplus
}
#endif
#endif
