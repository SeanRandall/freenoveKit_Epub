/*
 * Engine object lifecycle and parameter setters.
 */
#include "engine.h"

/* Default raw values of the 22 synthesis parameters for a new engine. */
/* @0x100a1c30 */
extern const uint8_t g_default_params[22];

/* TV_ENG32: the track block it clears is at the end of the object (CGRM:
 * at the start), there is no SAPI object, fmt_2104 and w_2130/w_2132 are
 * gone, and the TruVoice-only fields get their defaults.  Everything else
 * is left as it was: SaveFile_Synthesize 10001990 constructs the object in
 * its own uninitialised frame, and Engine_Init / Engine_Reset set the rest
 * (the one exception, out_spill_len, only indexes a buffer nothing reads). */
/* @0x10001c10 */
Engine *TV_THISCALL Engine_Construct(Engine *self)
{
    memset(&self->synth_busy, 0, sizeof *self - offsetof(Engine, synth_busy));
    self->sample_rate = 8000;
    self->z_value = 0;
    self->handle = NULL;
    self->item_notify = 0;
    self->out_limit = 16000;
    self->cur_voice = 0;
    self->item_done = 1;
    self->cur_bac = 0;
    self->e_0234 = 0;
    self->feed_stop = 1;
    self->feed_mode_i = 0;
    self->lock_rate = 0;
    self->lock_volume = 0;
    self->lock_pitch = 0;
    self->cur_pitch = 85;
    self->cur_speed = 150;
    self->cur_volume = 16;
    return self;
}

/* Default voice parameters, then a full reset. */
/* @0x100240c0 */
int32_t TV_THISCALL Engine_Init(Engine *self)
{
    int i;
    self->mode_P = 1;
    self->rate_index = 13;
    self->speed_wpm = 150;
    self->pitch = 85;
    self->flags_N = 0x17c0;
    self->flags_A = 0x41;
    self->mode_I = 0;
    self->volume_atten = 0;
    self->rate_class = 0;
    self->voice = 0;
    for (i = 0; i < 22; i++)
        self->cfg_bytes[i] = g_default_params[i];
    self->stop_mark = 0;
    return Engine_Reset(self, 1);
}

/* Clear all pipeline state.  Returns reset_value if a reset was requested
 * with one pending (stop_mark), else -1.
 * TV_ENG32: `full` (every caller in the DLL passes 1) also marks the input
 * empty and idle and clears the rings; without it only the input stage's
 * lookahead character is dropped. */
/* @0x10024120 */
int32_t TV_THISCALL Engine_Reset(Engine *self, int32_t full)
{
    int32_t ret = -1;
    if (self->stop_mark)
        ret = self->reset_value;
    self->st_2110 = 0;
    if (full) {
        self->st_input_empty = 1;
        self->st_idle = 1;
    }
    self->st_2113 = 0;
    self->st_2114 = 0;
    self->st_2115 = 0;
    self->st_2116 = 0;
    self->st_2117 = 0;
    self->skip_text = 0;
    self->reset_pending = 0;
    self->stop_mark = 0;
    self->out_state = 2;
    Preformat_Reset(self);
    Engine_ResetNodes(self);
    Synth_ResetTracks(self);
    Synth_InitFilters(self);
    if (full)
        Engine_ResetRings(self);
    else
        self->e_9180 = -1;
    Output_Reset(self, (int16_t)self->sample_rate);
    Stage4_Reset(self);
    Prosody_Reset(self);
    Stage3_Reset(self);
    Stage2_Reset(self);
    Stage1_Reset(self);
    Stage0_Reset(self);
    return ret;
}

/* @0x100244a0 */
void TV_THISCALL Engine_SetPitch(Engine *self, int32_t pitch)
{
    int i;
    self->pitch = pitch;
    for (i = 0; i < 5; i++)
        self->stage_ctx[i].pitch = pitch;
}

/* @0x100244d0 */
void TV_THISCALL Engine_SetSpeed(Engine *self, int32_t wpm)
{
    int32_t idx = (int32_t)((uint32_t)(wpm - 46) >> 3);
    int i;

    /* As in the original, above 253 wpm the index runs off the end of the
     * 26-row rate tables, and below 46 it wraps unsigned into a wild one. */
    self->speed_wpm = wpm;
    self->rate_index = idx;
    for (i = 0; i < 5; i++)
        self->stage_ctx[i].rate_index = idx;
}

/* TV_ENG32's volume is 0..16 (tts handle +0x7c), the attenuation 16 minus
 * it; 0 mutes.  (CGRM_EN took the SAPI volume, 0..0xffff linear, through
 * Volume_ToAtten, and muted below 0x50.)  Nothing in TV_ENG32 calls this:
 * the driver sets the volume with ESC[..a. */
/* @0x10024500 */
void TV_THISCALL Engine_SetVolume(Engine *self, uint32_t vol)
{
    int32_t att = 0x10 - (int32_t)vol;
    int i;
    if (att == 0x10) {
        self->mute = 1;
        return;
    }
    self->volume_atten = att;
    self->mute = 0;
    for (i = 0; i < 5; i++)
        self->stage_ctx[i].volume_atten = att;
}

/* @none cg:0x1002c8f0 */
void TV_THISCALL Engine_SetVoice(Engine *self, uint32_t voice)
{
    int i;
    if (voice >= 10)
        return;
    self->voice = (int32_t)voice;
    for (i = 0; i < 5; i++)
        self->stage_ctx[i].voice = (int32_t)voice;
}

/* Clear the rings and the input-side state. */
/* @0x10003640 */
void TV_THISCALL Engine_ResetRings(Engine *self)
{
    self->in_wr = 0;
    self->in_rd = 0;
    self->pre_wr = 0;
    self->pre_rd = 0;
    self->mid_wr = 0;
    self->mid_rd = 0;
    self->e_9180 = -1;
    self->in_flags_N = 0x17c0;
    self->in_flags_A = 0x41;
    self->mid_wrapped = 0;          /* TV_ENG32 only */
}

/* @0x10044710 */
void TV_THISCALL Prosody_Reset(Engine *self)
{
    self->e_206c = 85;
    self->e_2074 = 0;
    self->e_2070 = 0;
}

/* One scheduling step of the pipeline.  Downstream work always goes first:
 * audio is generated (Synth_Step, in step with Stage 4) until about 12000
 * bytes are waiting; otherwise the furthest stage with pending work runs,
 * provided enough nodes are free.  New input is read only when every later
 * stage is idle.
 *
 * Returns 2 when PCM is ready in out_buf, 0 otherwise; when the input is
 * exhausted and everything is idle it returns 1, or 3 if PCM remains. */
/* @0x10024210 */
int32_t TV_THISCALL Engine_Step(Engine *self)
{
    uint8_t produced = 0;

    if (self->reset_pending)
        Engine_Reset(self, 1);
    if (self->out_state != 2)
        self->out_state = Stage4_Run(self);

    if (self->st_2117) {
        if (Tracks_Op(self, 1, self->trk_38))
            self->st_2117 = 0;
    }
    /* TV_ENG32: stage 3 also runs while synth_19ad is set, and the audio
     * loop stops when out_buf is full (out_limit) instead of at 12000. */
    if (self->synth_19ad || (!self->st_2117 && self->st_2116)) {
        switch (Stage3_Run(self)) {
        case 0:
            self->st_2116 = 0;
            break;
        case 1:
            self->st_2117 = 1;
            break;
        }
        for (;;) {
            if (self->synth_busy || self->synth_hold)
                break;
            if (!Synth_Step(self))
                break;
            if (!self->synth_19ad)
                break;
            if (self->trk_34 != 0) {
                produced = 1;
                self->out_state = Stage4_Run(self);
                if (self->out_count >= self->out_limit)
                    break;
            } else if (self->out_count >= self->out_limit) {
                produced = 1;
                break;
            }
        }
        self->out_state = Stage4_Run(self);
    } else if (self->st_2115) {
        if (self->free_nodes > 0x69) {
            if (Stage2_Run(self))
                self->st_2116 = 1;
            else
                self->st_2115 = 0;
        }
    } else if (self->st_2114) {
        if (self->free_nodes > 0x25) {
            if (Stage1_Run(self))
                self->st_2115 = 1;
            else
                self->st_2114 = 0;
        }
    } else if (self->st_2113) {
        if (self->free_nodes > 0x25) {
            if (Stage0_Run(self))
                self->st_2114 = 1;
            else
                self->st_2113 = 0;
        }
    } else if (self->st_idle) {
        return (self->out_count != 0 ? 2 : 0) + 1;
    } else if (self->free_nodes > 0xf) {
        if (Engine_InputStage(self))
            self->st_2113 = 1;
        else
            self->st_idle = 1;
    }
    return produced ? 2 : 0;
}
