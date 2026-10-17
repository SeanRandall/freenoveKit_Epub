#include "cmd_play_event.h"
#include "cmd_addr.h"
#include "fw_ops.h"
#include "isr_dsp.h"
#include "par_addr.h"
#include "data/fw_data.h"

/* ---- cmd_run_event $BB74 ---- */

/* The C handlers by ROM address: the handler words the producers queue in cmd_ev_handlers (cmd_event.c). Any other
   address runs in ROM (spec 2.3); the producers queue none. The NULL row ends the table. */
static const struct { uint16_t addr; void (*fn)(fw *f, fw_regs *r); } k_events[] = {
    { ROM_CMD_EV_INDEX,       cmd_ev_index },
    { ROM_CMD_EV_SOUND_START, cmd_ev_sound_start },
    { ROM_CMD_EV_SOUND_END,   cmd_ev_sound_end },
    { ROM_CMD_EV_SET_VOICING, cmd_ev_set_voicing },
    { ROM_CMD_EV_SET_VOLUME,  cmd_ev_set_volume },
    { 0, NULL },
};

/* $BB74-$BB87: A = cmd_ev_next; INC cmd_ev_next; ASL A / TAX (8 bits); cmd_ev_vector = the word at $1306 + X, low
   byte first. */
static void event_vector(fw *f, fw_regs *r)
{
    r->a = fw_rd8(f, CMD_EV_NEXT);
    fw_inc8(f, CMD_EV_NEXT);
    r->a = (uint8_t)(r->a << 1);
    r->x = r->a;
    r->a = fw_rd8(f, (uint16_t)(CMD_EV_HANDLERS + r->x));
    fw_wr8(f, CMD_EV_VECTOR, r->a);
    r->a = fw_rd8(f, (uint16_t)(CMD_EV_HANDLERS + 1 + r->x));
    fw_wr8(f, CMD_EV_VECTOR + 1, r->a);
}

/* $BB88-$BB95: INX / INX (8 bits: X = $FE gives 0); cmd_ev_pos = the word at $1206 + X, low byte first. */
static void next_position(fw *f, fw_regs *r)
{
    r->x = (uint8_t)(r->x + 2);
    r->a = fw_rd8(f, (uint16_t)(CMD_EV_POSITIONS + r->x));
    fw_wr8(f, CMD_EV_POS, r->a);
    r->a = fw_rd8(f, (uint16_t)(CMD_EV_POSITIONS + 1 + r->x));
    fw_wr8(f, CMD_EV_POS + 1, r->a);
}

/* $BB96-$BB9F: LDX cmd_ev_next / DEX (8 bits); LDA $1406,X; JMP (cmd_ev_vector). */
static void dispatch(fw *f, fw_regs *r)
{
    r->x = (uint8_t)(fw_rd8(f, CMD_EV_NEXT) - 1);
    r->a = fw_rd8(f, (uint16_t)(CMD_EV_ARGS + r->x));
    uint16_t vector = fw_rd16(f, CMD_EV_VECTOR);
    for (unsigned i = 0; k_events[i].fn; i++) {
        if (k_events[i].addr == vector) {
            k_events[i].fn(f, r);
            return;
        }
    }
    fw_rom_call(f, vector, r, FW_ROM_CALL_BUDGET);   /* the handler's RTS returns to cmd_run_event's caller */
}

void cmd_run_event(fw *f, fw_regs *r)
{
    event_vector(f, r);
    next_position(f, r);
    dispatch(f, r);
}

/* ---- cmd_ev_index $BBA0 ---- */

void cmd_ev_index(fw *f, fw_regs *r)
{
    (void)r;
    if (fw_rd8(f, CMD_INDEX_COUNT) != 0)                         /* TST / BEQ $BBA6 */
        fw_wr8(f, CMD_INDEX_COUNT, (uint8_t)(fw_rd8(f, CMD_INDEX_COUNT) - 1));
}

/* ---- cmd_ev_sound_start $BBA7 ---- */

/* $BBAB-$BBD2: the four playback pointers (pros_phoneme_ptr, pros_duration_ptr, pros_pitch_ptr, pros_flags_ptr,
   $22-$29) to cmd_sound_save[0..3], each low byte then high byte. */
static void save_pointers(fw *f, fw_regs *r)
{
    for (uint8_t i = 0; i < 8; i++) {
        r->a = fw_rd8(f, (uint16_t)(PROS_PHONEME_PTR + i));
        fw_wr8(f, (uint16_t)(CMD_SOUND_SAVE + i), r->a);
    }
}

/* $BBD3-$BBFA: the effect's four pointers, LDA $9F74,X ... $9F7B,X, into $22-$29. */
static void effect_pointers(fw *f, fw_regs *r)
{
    for (uint8_t i = 0; i < 8; i++) {
        r->a = fw_rom8(f, (uint16_t)(CMD_SOUND_TABLE + i + r->x));
        fw_wr8(f, (uint16_t)(PROS_PHONEME_PTR + i), r->a);
    }
}

/* $BBFB-$BC35: par_seg_index to cmd_sound_seg_index; pros_seg_count, cmd_voicing, pros_pitch_centre,
   cmd_source_prosody and pros_play_target to cmd_sound_state[0..5]; par_next_phoneme to [7], then par_next_rank to
   [6]. */
static void save_state(fw *f, fw_regs *r)
{
    static const struct { uint16_t from, to; } k_saves[] = {
        { PAR_SEG_INDEX,      CMD_SOUND_SEG_INDEX },
        { PAR_SEG_INDEX + 1,  CMD_SOUND_SEG_INDEX + 1 },
        { PROS_SEG_COUNT,     CMD_SOUND_STATE },
        { PROS_SEG_COUNT + 1, CMD_SOUND_STATE + 1 },
        { CMD_VOICING,        CMD_SOUND_STATE + 2 },
        { PROS_PITCH_CENTRE,  CMD_SOUND_STATE + 3 },
        { CMD_SOURCE_PROSODY, CMD_SOUND_STATE + 4 },
        { PROS_PLAY_TARGET,   CMD_SOUND_STATE + 5 },
        { PAR_NEXT_PHONEME,   CMD_SOUND_STATE + 7 },
        { PAR_NEXT_RANK,      CMD_SOUND_STATE + 6 },
    };
    for (unsigned i = 0; i < sizeof k_saves / sizeof k_saves[0]; i++) {
        r->a = fw_rd8(f, k_saves[i].from);
        fw_wr8(f, k_saves[i].to, r->a);
    }
}

/* $BC36-$BC42: LDY #0; par_next_phoneme = (pros_phoneme_ptr),Y; TAY; par_next_rank = (par_rank_ptr),Y. The pointer is
   now into ROM. */
static void first_phoneme(fw *f, fw_regs *r)
{
    r->y = 0;
    r->a = fw_ind_y(f, PROS_PHONEME_PTR, r->y);
    fw_wr8(f, PAR_NEXT_PHONEME, r->a);
    r->y = r->a;
    r->a = fw_ind_y(f, PAR_RANK_PTR, r->y);
    fw_wr8(f, PAR_NEXT_RANK, r->a);
}

/* $BC43-$BC60: the fixed pitch centre, prosody and voicing; LDA #0 / LDY #$FF: par_seg_index = 0, pros_seg_count+1
   = 0, pros_seg_count = $FF. */
static void fixed_state(fw *f, fw_regs *r)
{
    r->a = CMD_SOUND_PITCH_CENTRE;
    fw_wr8(f, PROS_PITCH_CENTRE, r->a);
    r->a = CMD_SOUND_SOURCE_PROSODY;
    fw_wr8(f, CMD_SOURCE_PROSODY, r->a);
    r->a = CMD_SOUND_VOICING;
    fw_wr8(f, CMD_VOICING, r->a);
    r->a = 0;
    r->y = 0xFF;
    fw_wr8(f, PAR_SEG_INDEX, r->a);
    fw_wr8(f, PAR_SEG_INDEX + 1, r->a);
    fw_wr8(f, PROS_SEG_COUNT + 1, r->a);
    fw_wr8(f, PROS_SEG_COUNT, r->y);
}

/* $BC61-$BC67: INY; LDA (pros_phoneme_ptr),Y; CMP #$FF; BNE: Y = the end's index (8 bits; from Y = $FF, the first
   read is at 0). The ROM loops forever without an end; the C stops after 256 reads and returns false. */
static bool find_end(fw *f, fw_regs *r)
{
    for (unsigned n = 0; n < 0x100; n++) {
        r->y = (uint8_t)(r->y + 1);
        r->a = fw_ind_y(f, PROS_PHONEME_PTR, r->y);
        if (r->a == CMD_SOUND_END)
            return true;
    }
    return false;
}

/* $BC68-$BC7F: LDX cmd_ev_next / TXA / ASL A / TAX (8 bits); TYA: the word at $1206 + X and cmd_ev_pos = Y, high
   bytes 0 (LDA #0); SEB0 cmd_sound_active. */
static void end_event(fw *f, fw_regs *r)
{
    r->x = fw_rd8(f, CMD_EV_NEXT);
    r->a = (uint8_t)(r->x << 1);
    r->x = r->a;
    r->a = r->y;
    fw_wr8(f, (uint16_t)(CMD_EV_POSITIONS + r->x), r->a);
    fw_wr8(f, CMD_EV_POS, r->a);
    r->a = 0;
    fw_wr8(f, (uint16_t)(CMD_EV_POSITIONS + 1 + r->x), r->a);
    fw_wr8(f, CMD_EV_POS + 1, r->a);
    fw_wr8(f, CMD_SOUND_ACTIVE, (uint8_t)(fw_rd8(f, CMD_SOUND_ACTIVE) | CMD_SOUND_ACTIVE_ON));
}

void cmd_ev_sound_start(fw *f, fw_regs *r)
{
    r->a = util_shl3(r->a);                                     /* $BBA7 JSR util_shl3 */
    r->x = r->a;                                                /* TAX */
    save_pointers(f, r);
    effect_pointers(f, r);
    save_state(f, r);
    first_phoneme(f, r);
    fixed_state(f, r);
    if (!find_end(f, r)) {
        fw_fault(f, ROM_CMD_EV_SOUND_START);                    /* the ROM never leaves the scan */
        return;
    }
    end_event(f, r);
}

/* ---- cmd_ev_sound_end $BC80 ---- */

/* $BC86-$BCAD: cmd_sound_save[0..3] back into the four playback pointers, each low byte then high byte. */
static void restore_pointers(fw *f, fw_regs *r)
{
    for (uint8_t i = 0; i < 8; i++) {
        r->a = fw_rd8(f, (uint16_t)(CMD_SOUND_SAVE + i));
        fw_wr8(f, (uint16_t)(PROS_PHONEME_PTR + i), r->a);
    }
}

/* $BCAE-$BCE8: the reverse of save_state, in the same order ([7] to par_next_phoneme before [6] to par_next_rank). */
static void restore_state(fw *f, fw_regs *r)
{
    static const struct { uint16_t from, to; } k_restores[] = {
        { CMD_SOUND_SEG_INDEX,     PAR_SEG_INDEX },
        { CMD_SOUND_SEG_INDEX + 1, PAR_SEG_INDEX + 1 },
        { CMD_SOUND_STATE,         PROS_SEG_COUNT },
        { CMD_SOUND_STATE + 1,     PROS_SEG_COUNT + 1 },
        { CMD_SOUND_STATE + 2,     CMD_VOICING },
        { CMD_SOUND_STATE + 3,     PROS_PITCH_CENTRE },
        { CMD_SOUND_STATE + 4,     CMD_SOURCE_PROSODY },
        { CMD_SOUND_STATE + 5,     PROS_PLAY_TARGET },
        { CMD_SOUND_STATE + 7,     PAR_NEXT_PHONEME },
        { CMD_SOUND_STATE + 6,     PAR_NEXT_RANK },
    };
    for (unsigned i = 0; i < sizeof k_restores / sizeof k_restores[0]; i++) {
        r->a = fw_rd8(f, k_restores[i].from);
        fw_wr8(f, k_restores[i].to, r->a);
    }
}

void cmd_ev_sound_end(fw *f, fw_regs *r)
{
    if (!(fw_rd8(f, CMD_SOUND_ACTIVE) & CMD_SOUND_ACTIVE_ON))   /* BBS0 cmd_sound_active, $BC84 / RTS */
        return;
    fw_wr8(f, CMD_SOUND_ACTIVE, (uint8_t)(fw_rd8(f, CMD_SOUND_ACTIVE) & ~CMD_SOUND_ACTIVE_ON));   /* CLB0 */
    restore_pointers(f, r);
    restore_state(f, r);
}

/* ---- cmd_ev_set_voicing $BCEA, cmd_ev_set_volume $BCED ---- */

void cmd_ev_set_voicing(fw *f, fw_regs *r) { fw_wr8(f, CMD_VOICING, r->a); }

void cmd_ev_set_volume(fw *f, fw_regs *r)
{
    fw_wr8(f, CMD_VOLUME, r->a);
    isr_set_volume(f, r);                                        /* $BCEF; its RTS is ours ($BCF2) */
}
