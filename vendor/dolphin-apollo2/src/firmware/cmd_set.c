/* The setting handlers of the deferred commands (ROM $A902-$ABDE), decompiled (M4.2 spec 2.2). Each one is
   entered with X = its argument index and A = boot_clause[X] (cmd_handlers.h). */
#include "cmd_handlers.h"
#include "cmd_apply.h"
#include "cmd_match.h"
#include "fw_ops.h"
#include "tn_rules.h"
#include "data/fw_data.h"

static uint8_t clause_at(const fw *f, uint8_t x) { return fw_rd8(f, (uint16_t)(BOOT_CLAUSE + x)); }

/* The one-digit setters: setting = cmd_hex_value(a) when it is below limit. The ROM's CMP #0 / BCC is dead,
   because CMP #0 always sets C. */
static void set_below(fw *f, uint16_t setting, uint8_t a, uint8_t limit)
{
    uint8_t v = cmd_hex_value(f, a);
    if (v < limit)
        fw_wr8(f, setting, v);
}

/* $A868 (cmd_handlers.h): the ROM preserves A and Y and returns the X tn_append_phonetic leaves (cmd_say_version's
   cmd_run_done restores the handler's X at the end). */
uint8_t cmd_put_phonetic(fw *f, uint8_t x)
{
    uint8_t y = 0;
    tn_append_phonetic(f, tn_rule_byte(f, 0), &x, &y);
    return x;
}

void cmd_say_version(fw *f, uint8_t x, uint8_t a)
{
    if (a != '?')
        return;
    fw_wr16(f, TN_RULE_PTR, CMD_VERSION_ENGINE_TEXT);
    x = cmd_put_phonetic(f, x);
    fw_wr16(f, TN_RULE_PTR, CMD_VERSION_LANG_TEXT);
    cmd_put_phonetic(f, x);                        /* X as the first call left it, as in the ROM */
}

void cmd_set_phrase_pause(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    set_below(f, CMD_PHRASE_PAUSE, a, 0x10);
}

void cmd_set_word_pause(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    set_below(f, CMD_WORD_PAUSE, a, 10);
}

void cmd_set_mark_space(fw *f, uint8_t x, uint8_t a)
{
    (void)a;                                            /* reloaded after the re-anchoring */
    x = cmd_find_back(f, 'M', x);                       /* $75 = 'M' */
    uint8_t v = cmd_hex_value(f, clause_at(f, x));
    if (v >= 4)
        return;
    fw_wr8(f, FW_TMP_75, util_shl4(v));
    v = cmd_hex_value(f, clause_at(f, ++x));
    if (v >= 0x10)
        return;
    fw_wr8(f, CMD_MARK_SPACE, (uint8_t)(v + fw_rd8(f, FW_TMP_75)));
}

void cmd_set_spell(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    set_below(f, CMD_SPELL, a, 2);                      /* the ROM: BEQ stores 0, then CMP #2 / BCS */
}

void cmd_set_punct(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    set_below(f, CMD_PUNCT, a, 2);
}

void cmd_set_hyper(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    set_below(f, CMD_HYPER, a, 2);
}

void cmd_set_phonetic(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    set_below(f, CMD_PHONETIC, a, 2);
}

void cmd_set_formant(fw *f, uint8_t x, uint8_t a)
{
    (void)a;
    x = cmd_find_back(f, 'U', x);                       /* $75 = 'U' */
    uint8_t d = cmd_hex_value(f, clause_at(f, x));      /* the parameter 0-9 */
    if (d >= 10)
        return;
    uint8_t v = cmd_hex_value(f, clause_at(f, ++x));
    if (v >= 0x10)
        return;
    fw_wr8(f, FW_TMP_75, util_shl4(v));
    v = cmd_hex_value(f, clause_at(f, ++x));
    if (v >= 0x10)
        return;
    fw_wr8(f, FW_TMP_75, (uint8_t)(v + fw_rd8(f, FW_TMP_75)));
    uint8_t sign = clause_at(f, ++x);                   /* raw, not case-mapped */
    if (sign == '-')
        fw_wr8(f, (uint16_t)(CMD_FORMANT_OFS + d), (uint8_t)(0 - fw_rd8(f, FW_TMP_75)));
    else if (sign == '+')
        fw_wr8(f, (uint16_t)(CMD_FORMANT_OFS + d), fw_rd8(f, FW_TMP_75));
}

void cmd_ignore(fw *f, uint8_t x, uint8_t a)
{
    (void)f;
    (void)x;
    (void)a;
}

void cmd_set_pitch(fw *f, uint8_t x, uint8_t a)
{
    if (a == '+') {
        fw_wr8(f, CMD_PITCH_STEP, 0x10);
        return;
    }
    if (a == '-') {
        fw_wr8(f, CMD_PITCH_STEP, 0);
        return;
    }
    uint8_t v = cmd_hex_value(f, a);
    if (v >= 0x10)
        return;
    fw_wr8(f, CMD_PITCH, v);
    fw_wr8(f, CMD_PITCH_STEP, 8);
    uint8_t y = cmd_handler_y(f);                       /* handed on to par_setup_voice, which does not read it */
    cmd_apply_pitch(f, &x, &y);
}

void cmd_set_speed(fw *f, uint8_t x, uint8_t a)
{
    (void)x;
    uint8_t v = cmd_hex_value(f, a);
    if (v < 1 || v >= 10)
        return;
    fw_wr8(f, CMD_SPEED, v);
    cmd_apply_speed(f);
}

void cmd_set_prosody(fw *f, uint8_t x, uint8_t a)
{
    x = cmd_find_back(f, 'R', x);                       /* $75 = 'R' */
    a = clause_at(f, x);                                /* raw: the digit test is not case-mapped */
    if (a < '0')
        return;
    if (a < '8') {                                      /* a digit 0-7: the prosody */
        fw_wr8(f, CMD_PROSODY, (uint8_t)(a - '0'));
        uint8_t y = cmd_handler_y(f);
        cmd_apply_pitch(f, &x, &y);
        return;
    }
    a = cmd_upcase(f, a);                               /* $A9F7: a letter, then two hex digits */
    if (a < 'A' || a > 'Z')
        return;
    fw_wr8(f, FW_TMP_76, (uint8_t)(a - 'A'));
    fw_wr8(f, FW_TMP_34, cmd_upcase(f, clause_at(f, ++x)));
    fw_wr8(f, FW_TMP_35, cmd_upcase(f, clause_at(f, ++x)));
    a = cmd_hex_byte(f);
    fw_wr8(f, (uint16_t)(PROS_PARAMS + fw_rd8(f, FW_TMP_76)), a);
}

void cmd_set_source(fw *f, uint8_t x, uint8_t a)
{
    uint8_t v = cmd_hex_value(f, a);
    if (v >= 8)
        return;
    fw_wr8(f, CMD_SOURCE, v);
    uint8_t y = cmd_handler_y(f);
    cmd_apply_pitch(f, &x, &y);
}

void cmd_set_speaker(fw *f, uint8_t x, uint8_t a)
{
    uint8_t v = cmd_hex_value(f, a);
    if (v > 1)
        return;
    fw_wr8(f, CMD_SPEAKER, v);
    uint8_t y = cmd_handler_y(f);
    cmd_apply_speaker(f, &x, &y);
}

void cmd_set_voice(fw *f, uint8_t x, uint8_t a)
{
    /* the presets 1-6: 1-3 with speaker 0 and source 0, 4-6 with speaker 1 and source 7 */
    static const struct { uint8_t voicing, mark_space, pitch; } k_presets[6] = {
        { 8, 0x14, 8 },    { 8, 0x28, 6 },    { 7, 0x0A, 9 },
        { 7, 0x14, 0x0B }, { 6, 0x0A, 0x0A }, { 8, 0x14, 0x0C },
    };
    cmd_clear_formant(f);
    uint8_t v = cmd_hex_value(f, a);
    if (v == 0) {                                       /* $AA7A: BNE on cmd_hex_value's Z; @V0 turns the voice off */
        fw_wr8(f, CMD_VOICE, 0);
        return;
    }
    if (v > 6)
        return;
    fw_wr8(f, CMD_VOICING, k_presets[v - 1].voicing);
    fw_wr8(f, CMD_MARK_SPACE, k_presets[v - 1].mark_space);
    fw_wr8(f, CMD_PITCH, k_presets[v - 1].pitch);
    fw_wr8(f, CMD_SPEAKER, (uint8_t)(v >= 4 ? 1 : 0));
    fw_wr8(f, CMD_SOURCE, (uint8_t)(v >= 4 ? 7 : 0));
    fw_wr8(f, CMD_VOICE, v);
    uint8_t y = cmd_handler_y(f);
    cmd_apply_speaker(f, &x, &y);
}
