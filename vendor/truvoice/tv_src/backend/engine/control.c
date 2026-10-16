/*
 * Control commands.
 *
 * Type-0 nodes carry the commands the preformatter parsed out of the text
 * (ESC[..X).  Every stage executes them as its cursor passes, so a change
 * takes effect at the right point of that stage's work; most commands only
 * act in the one stage that owns the setting, and the values are kept
 * per stage in StageCtx.
 *
 * TV_ENG32's version (REFERENCE 8.6): the same cases as CGRM_EN's, without
 * the SAPI object.  What differs:
 *  - the lock bytes lock_rate / lock_volume / lock_pitch make V, r/v, a and
 *    p do nothing (they are only ever cleared, so this is dead);
 *  - V and a no longer set cur_voice's speed and pitch through SAPI, and a
 *    does not touch cur_volume; r/v and p do not set cur_speed / cur_pitch
 *    except p at stage 2;
 *  - c sets cur_bac at stage 2 always (CGRM: only with a SAPI object);
 *  - i calls the handle's index-mark callback at stage 4 (CGRM: posts SAPI
 *    messages at stage 3 and may request a reset); TruVoice never sets the
 *    callback, so index marks do nothing;
 *  - z (TruVoice only) stores its argument in z_value, which nothing reads.
 */
#include "engine.h"
#include "crt.h"

/* Per-voice defaults, indexed by voice number. */
/* @0x100bbe50 */
extern const uint32_t g_voice_pitch[10];
/* @0x100bbe78 */
extern const uint32_t g_voice_rate_index[10];
/* @0x100bbea0 */
extern const uint32_t g_voice_speed[10];

/* Execute the control node the running stage has reached.  Returns 0 only
 * when stage 2 meets a node it must stop at. */
/* @0x10011310 */
uint8_t TV_THISCALL Engine_RunControl(Engine *self)
{
    StageCtx *st = self->stage;
    Node *n = st->ctl;
    int32_t stage = (int32_t)(st - self->stage_ctx);
    TvMarkSink *sink;
    uint32_t flags;
    int32_t arg;
    uint8_t b15;

    if (stage == 2 && NODE_TYPE(n) == 5)
        return 0;
    flags = n->flags;
    if (flags & NODE_TYPE_MASK)
        return 1;

    arg = (int32_t)n->arg;
    if (n->value != 'i')
        arg &= 0xff;
    b15 = n->b15;

    switch (n->value) {
    case 'A':
        st->p_38 = ((int32_t)b15 << 8) | arg;
        break;
    case 'C':
        if (stage == 0)
            Engine_NodeAlloc(self, n, 0, 3, ']');
        break;
    case 'I':
        st->p_1c = arg;
        break;
    case 'N':
        st->p_34 = ((int32_t)b15 << 8) | arg;
        break;
    case 'P':
        st->p_20 = ((int32_t)b15 << 8) + arg;
        break;
    case 'V':
        st->voice = arg;
        if (!self->lock_pitch)
            self->stage->pitch = (int32_t)g_voice_pitch[self->stage->voice];
        if (!self->lock_rate)
            self->stage->rate_index = (int32_t)g_voice_rate_index[self->stage->voice];
        if (self->stage - self->stage_ctx != 3)
            break;
        self->cur_voice = (int16_t)arg;
        if (!self->lock_rate)
            self->cur_speed = g_voice_speed[(int16_t)arg];
        if (!self->lock_pitch)
            self->cur_pitch = (uint32_t)self->stage->pitch;
        break;
    case 'a':
        if (self->lock_volume)
            break;
        st->volume_atten = arg;
        if (self->stage - self->stage_ctx != 3)
            break;
        if (arg == 0x10)
            self->stage->volume_atten = 100;
        else
            self->mute = 0;
        break;
    case 'c':
        if (stage == 2)
            self->cur_bac = arg;
        break;
    case 'f':
        if (st->p_30 == 0)
            self->e_1ae4 = st->rate_index;
        st->p_30 = arg;
        self->stage->rate_index = b15;
        if (self->stage->p_30 != 0)
            break;
        self->stage->rate_index = self->e_1ae4;
        break;
    case 'g':
    case 's':
    case 't':
        if (stage != 2)
            break;
        n->flags = (flags & ~2u) | 5;
        break;
    case 'i':
        sink = (TvMarkSink *)self->handle;
        if (stage == 4 && sink != NULL && sink->fn != NULL && n->arg != 0)
            sink->fn(sink->ctx, n->arg);
        break;
    case 'l':
        /* the preformatter has already clamped the index below 22 */
        self->s3_param_raw[arg] = b15;
        break;
    case 'p':
        if (self->lock_pitch)
            break;
        st->pitch = arg + arg;
        if (self->stage - self->stage_ctx == 2)
            self->cur_pitch = (uint32_t)self->stage->pitch;
        break;
    case 'r':
    case 'v':
        if (!self->lock_rate)
            st->rate_index = arg;
        break;
    case 'x':
        if (stage == 4) {
            self->reset_value = 0;
            self->reset_pending = 1;
        } else if (stage == 0) {
            Engine_NodeAlloc(self, n, 1, 0, 'C');
        }
        break;
    case 'z':
        if (stage == 4)
            self->z_value = (int32_t)n->arg;
        break;
    default:
        break;
    }
    return 1;
}
