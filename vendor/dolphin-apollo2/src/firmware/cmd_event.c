/* The deferred handlers that queue a command event instead of changing a setting (M4.2 spec 2.1): @T, @A, @B
   and the deferred part of @I+. An event is keyed by the phonetic position tn_phon_len and runs at playback
   (cmd_run_event $BB74, C since M4.4b: cmd_play_event.c). Entered as the ROM handlers are: x = the argument's
   boot_clause index, a = boot_clause[x]. */
#include "cmd_handlers.h"
#include "cmd_addr.h"
#include "cmd_match.h"

/* The queue as the producers address it, n = cmd_ev_count:
   - cmd_ev_args[n] is STA $1406,X with X = n, unwrapped: n >= 128 writes on into cmd_count ($1486) and the
     deferred queue;
   - positions and handlers are STA $1206,X / $1306,X with X = ASL n, which wraps at n = 128; an offset in the
     instruction ($1208,X, $1308,X) adds to the 16-bit address and does not wrap, so ofs is X plus that offset;
   - only the low byte of a 16-bit position is written (pros_phrase_gen rewrites the whole word later).
   Within one producer the addresses never overlap, so the order of the writes does not matter. */
static void put_event(fw *f, uint16_t ofs, uint16_t handler)
{
    fw_wr8(f, (uint16_t)(CMD_EV_POSITIONS + ofs), fw_rd8(f, TN_PHON_LEN));
    fw_wr16(f, (uint16_t)(CMD_EV_HANDLERS + ofs), handler);
}

/* $A8C0 @T: effect 0-$10 (so 'G' is effect 16), or 'X' (hex value $21) for effect 16. Two events at the same
   position with the same argument: cmd_ev_sound_start, then cmd_ev_sound_end. */
void cmd_sound(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    uint8_t v = cmd_hex_value(f, a);
    if (v == 0x21)
        v = 0x10;
    if (v >= 0x11)
        return;
    uint8_t n = fw_rd8(f, CMD_EV_COUNT);
    fw_wr8(f, (uint16_t)(CMD_EV_ARGS + n), v);
    fw_wr8(f, (uint16_t)(CMD_EV_ARGS + 1 + n), v);              /* STA $1407,X */
    uint8_t i = (uint8_t)(n << 1);
    put_event(f, i, ROM_CMD_EV_SOUND_START);
    put_event(f, (uint16_t)(i + 2), ROM_CMD_EV_SOUND_END);      /* $1208,X / $1308,X: no wrap */
    fw_wr8(f, CMD_EV_COUNT, (uint8_t)(n + 2));                  /* INC twice */
}

/* $AB05 @A: volume 0-F, set at playback by cmd_ev_set_volume */
void cmd_set_volume(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    uint8_t v = cmd_hex_value(f, a);
    if (v >= 0x10)
        return;
    uint8_t n = fw_rd8(f, CMD_EV_COUNT);
    fw_wr8(f, (uint16_t)(CMD_EV_ARGS + n), v);
    put_event(f, (uint8_t)(n << 1), ROM_CMD_EV_SET_VOLUME);
    fw_wr8(f, CMD_EV_COUNT, (uint8_t)(n + 1));
}

/* $AB70 @B: voicing 0-9, set at playback by cmd_ev_set_voicing */
void cmd_set_voicing(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    uint8_t v = cmd_hex_value(f, a);
    if (v >= 0x0A)
        return;
    uint8_t n = fw_rd8(f, CMD_EV_COUNT);
    fw_wr8(f, (uint16_t)(CMD_EV_ARGS + n), v);
    put_event(f, (uint8_t)(n << 1), ROM_CMD_EV_SET_VOICING);
    fw_wr8(f, CMD_EV_COUNT, (uint8_t)(n + 1));
}

/* $AB2E, the deferred part of @I+: with the raw argument '+' (no cmd_hex_value), an index-mark event; no
   argument is written, so cmd_ev_args[n] stays stale */
void cmd_mark_index(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    if (a != '+')
        return;
    uint8_t n = fw_rd8(f, CMD_EV_COUNT);
    put_event(f, (uint8_t)(n << 1), ROM_CMD_EV_INDEX);
    fw_wr8(f, CMD_EV_COUNT, (uint8_t)(n + 1));
}
