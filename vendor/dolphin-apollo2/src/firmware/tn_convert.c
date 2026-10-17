#include "tn_convert.h"
#include "cmd_run.h"
#include "fw_ops.h"
#include "lts.h"
#include "par_play.h"
#include "pros_make.h"
#include "tn_rules.h"
#include "data/fw_data.h"

static uint8_t char_class(fw *f, uint8_t c) { return fw_rom8(f, (uint16_t)(LTS_CHAR_CLASS + c)); }
static uint8_t text_at(const fw *f, uint8_t x) { return fw_rd8(f, (uint16_t)(CMD_TEXT + x)); }

/* tn_phonetic[tn_phon_len++] = c */
static void put_phonetic(fw *f, uint8_t c)
{
    uint8_t n = fw_rd8(f, TN_PHON_LEN);
    fw_wr8(f, (uint16_t)(TN_PHONETIC + n), c);
    fw_wr8(f, TN_PHON_LEN, (uint8_t)(n + 1));
}

void tn_convert_clause(fw *f)
{
    fw_wr8(f, TN_RESUME_POS, 0);
    for (;;) {                                                  /* a pass over tn_phonetic: $B1E9 */
        uint8_t x = 0;
        fw_wr8(f, CMD_EV_COUNT, x);
        fw_wr8(f, TN_PHON_LEN, x);
        x = fw_rd8(f, TN_RESUME_POS);
        fw_wr8(f, TN_POS, x);
        fw_wr8(f, TN_KEY_POS, x);
        for (;;) {                                              /* per character: $B1F6 */
            fw_par_ready(f, (fw_regs){ 0, x, 0, 0 });           /* X kept */
            if (fw_rd8(f, TN_PHON_LEN) >= TN_PHONETIC_FLUSH)
                break;                                          /* nearly full: speak it now */
            for (;;) {                                          /* $B20A: the deferred commands up to tn_pos */
                x = fw_rd8(f, TN_KEY_POS);
                cmd_run_due(f, x);                              /* X kept: every handler ends at cmd_run_done */
                if (x >= fw_rd8(f, TN_POS))
                    break;
                fw_inc8(f, TN_KEY_POS);
            }
            uint8_t c = text_at(f, x);
            if (fw_rd8(f, CMD_PHONETIC)) {                      /* @X: copied verbatim; tn_key_pos lags */
                put_phonetic(f, c);
                fw_wr8(f, TN_POS, ++x);
                if (x >= fw_rd8(f, BOOT_CLAUSE_LEN))
                    return;                                     /* no final cmd_run_due here */
                continue;
            }
            bool to_rules = false;
            bool copy = false;
            if (char_class(f, c) & CC_LETTER) {
                fw_wr8(f, TN_KEY_POS, x);
                if (!(fw_rd8(f, TN_IN_WORD) & 1) && !(fw_rd8(f, CMD_SPELL) & 1)) {
                    /* $B23A: a word starts; it is spelled (cmd_spell = 3) when a non-letter comes before any vowel */
                    for (uint8_t i = x;; i++) {
                        uint8_t cl = char_class(f, text_at(f, i));
                        if (!(cl & CC_LETTER)) {
                            fw_wr8(f, CMD_SPELL, 3);
                            break;
                        }
                        if (cl & CC_VOWEL)
                            break;
                    }
                }
                to_rules = true;
            } else {                                            /* $B264: not a letter */
                fw_wr8(f, TN_IN_WORD, fw_rd8(f, TN_IN_WORD) & 0xFE);
                if (fw_rd8(f, CMD_SPELL) & 2)
                    fw_wr8(f, CMD_SPELL, 0);                    /* automatic spelling ends */
                if (c == ' ') {
                    uint8_t pos = fw_rd8(f, TN_POS);
                    if ((fw_rd8(f, CMD_SPELL) & 1) && pos != 0 && (uint8_t)(pos + 1) != fw_rd8(f, BOOT_CLAUSE_LEN))
                        x = tn_say_char(f, c);                  /* spell mode: the space is spoken */
                    else
                        copy = true;
                } else if (c >= '0' && c <= '9') {
                    fw_wr8(f, TN_KEY_POS, x);
                    to_rules = true;                            /* digits go to the rules like letters */
                } else if (fw_rd8(f, CMD_PUNCT)) {
                    x = tn_say_char(f, c);                      /* @P1: spoken by name */
                } else {
                    copy = true;
                }
            }
            if (to_rules) {                                     /* $B256 */
                lts_convert_char(f, c);
                fw_set_low_window(f, false);                    /* CLB7 P2 */
                x = (uint8_t)(fw_rd8(f, TN_POS) - 1);           /* the rules moved tn_pos */
                fw_wr8(f, TN_IN_WORD, fw_rd8(f, TN_IN_WORD) | 1);
            } else if (copy) {                                  /* $B2A5: X saved in $75 around the copy */
                fw_wr8(f, FW_TMP_75, x);
                put_phonetic(f, c);
                x = fw_rd8(f, FW_TMP_75);
            }
            fw_wr8(f, TN_POS, ++x);                             /* $B297 */
            if (x >= fw_rd8(f, BOOT_CLAUSE_LEN)) {
                cmd_run_due(f, x);
                return;
            }
        }
        /* $B1FF: speak tn_phonetic, then convert on from here with an empty buffer. The C pros_make_segments
           (M4.3b) leaves in r what the ROM's leaves (pros_phrase_gen's A, X and Y) for the C boot_loop_play
           (M4.4b). The entry Y here is 0, the ROM has the last character's: it reaches only isr_par_ready, which
           never reads it, and $CBB3 reloads Y, so no RAM differs (M4.3b spec 6). */
        fw_wr8(f, TN_RESUME_POS, x);
        fw_regs r = { fw_rd8(f, TN_PHON_LEN), x, 0, 0 };
        pros_make_segments(f, &r);
        boot_loop_play(f, &r);
        f->tn_flushes++;
    }
}
