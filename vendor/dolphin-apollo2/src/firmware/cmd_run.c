#include <stddef.h>
#include "cmd_run.h"
#include "cmd_addr.h"
#include "cmd_handlers.h"

/* The C handlers by ROM address: the bytes cmd_scan_clause copies into cmd_handlers. Every deferred handler is
   here (bank_cmd_select since M4.6c1); any other address (the latent NUL bug of the scan, M4.2 spec 2.6) runs through
   the ROM cmd_run_next (M4.6c1 spec 2.5). The NULL row ends the table. */
static const struct { uint16_t addr; cmd_handler *fn; } k_handlers[] = {
    { ROM_CMD_SAY_VERSION,      cmd_say_version },
    { ROM_CMD_SET_PHRASE_PAUSE, cmd_set_phrase_pause },
    { ROM_CMD_SET_FORMANT,      cmd_set_formant },
    { ROM_CMD_SET_WORD_PAUSE,   cmd_set_word_pause },
    { ROM_CMD_SET_MARK_SPACE,   cmd_set_mark_space },
    { ROM_CMD_SET_SPELL,        cmd_set_spell },
    { ROM_CMD_SET_PUNCT,        cmd_set_punct },
    { ROM_CMD_SET_HYPER,        cmd_set_hyper },
    { ROM_CMD_SET_PHONETIC,     cmd_set_phonetic },
    { ROM_CMD_IGNORE,           cmd_ignore },
    { ROM_CMD_SET_SPEED,        cmd_set_speed },
    { ROM_CMD_SET_PROSODY,      cmd_set_prosody },
    { ROM_CMD_SET_PITCH,        cmd_set_pitch },
    { ROM_CMD_SET_SPEAKER,      cmd_set_speaker },
    { ROM_CMD_SET_VOICE,        cmd_set_voice },
    { ROM_CMD_SET_SOURCE,       cmd_set_source },
    { ROM_CMD_SOUND,            cmd_sound },
    { ROM_CMD_SET_VOLUME,       cmd_set_volume },
    { ROM_CMD_SET_VOICING,      cmd_set_voicing },
    { ROM_CMD_MARK_INDEX,       cmd_mark_index },
    { ROM_CMD_ECHO,             cmd_echo },
    { ROM_CMD_QUERY,            cmd_query },
    { ROM_CMD_LIST_LANGUAGES,   cmd_list_languages },
    { ROM_BANK_CMD_SELECT,      bank_cmd_select },
    { 0, NULL },
};

static cmd_handler *find_handler(uint16_t addr)
{
    for (unsigned i = 0; k_handlers[i].fn; i++)
        if (k_handlers[i].addr == addr)
            return k_handlers[i].fn;
    return NULL;
}

bool cmd_run_next(fw *f, uint8_t x)
{
    uint8_t n = fw_rd8(f, CMD_NEXT);
    uint8_t i = (uint8_t)(n << 1);                                      /* ASL A / TAX: an 8-bit index */
    uint16_t addr = (uint16_t)(fw_rd8(f, (uint16_t)(CMD_HANDLERS + i)) |
                               fw_rd8(f, (uint16_t)(CMD_HANDLERS + 1 + i)) << 8);
    cmd_handler *h = find_handler(addr);
    if (!h) {
        /* No C handler: an address of the NUL bug. The ROM cmd_run_next does everything, starting from RAM as the
           C found it, through a ROM call that skips its own hook (M4.6c1 spec 2.5). Registers as at $A88D: X = the
           position, Y = cmd_next (the LDY at $A880), and A = X, which cmd_run_next's TXA makes it anyway. */
        fw_regs r = { x, x, n, 0 };
        return fw_rom_call_unhooked(f, ROM_CMD_RUN_NEXT, &r, FW_ROM_CALL_BUDGET);
    }
    fw_wr16(f, CMD_VECTOR, addr);                                       /* cmd_vector is RAM: kept exact */
    n++;                                                                /* INC cmd_next */
    fw_wr8(f, CMD_NEXT, n);
    fw_wr8(f, CMD_NEXT_POS, fw_rd8(f, (uint16_t)(CMD_POSITIONS + n)));  /* LDX cmd_next / LDA $148B,X */
    uint8_t ax = fw_rd8(f, (uint16_t)(CMD_ARG_INDEX + (uint8_t)(n - 1)));   /* DEX / LDA $160B,X / TAX */
    h(f, ax, fw_rd8(f, (uint16_t)(BOOT_CLAUSE + ax)));                  /* JMP ($1489) */
    return !f->unwound;                                                 /* cmd_run_done: X back to x */
}

void cmd_run_due(fw *f, uint8_t x)
{
    while (fw_rd8(f, CMD_NEXT) < fw_rd8(f, CMD_COUNT) && x == fw_rd8(f, CMD_NEXT_POS)) {
        if (!cmd_run_next(f, x))
            return;
    }
}
