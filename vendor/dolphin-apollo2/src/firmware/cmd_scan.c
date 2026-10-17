#include "cmd_scan.h"
#include "cmd_match.h"
#include "fw_ops.h"
#include "data/fw_data.h"

void cmd_scan_clause(fw *f)
{
    uint8_t a = 0xFF, x = 0, y = 0;                 /* X: boot_clause read index, Y: cmd_text write index */
    fw_wr8(f, CMD_COUNT, y);
    fw_wr8(f, CMD_NEXT, y);
    fw_wr8(f, (uint16_t)(CMD_POSITIONS + x), a);    /* cmd_positions[0] = $FF: no command yet */
    do {                                            /* $AD09: one boot_clause character */
        fw_par_ready(f, (fw_regs){ a, x, y, 0 });                /* parallel mode: ready for the next byte */
        a = fw_rd8(f, (uint16_t)(BOOT_CLAUSE + x++));
        if (a != fw_rd8(f, CMD_CHAR)) {
            if (!(fw_rd8(f, CMD_PHONETIC) & 1) && a >= 0x21) {   /* BBS0: bit 0 only; control codes stay raw */
                fw_wr8(f, FW_TMP_78, y);                         /* Y saved around the table read */
                a = cmd_upcase(f, a);
            }
            fw_wr8(f, (uint16_t)(CMD_TEXT + y++), a);
            continue;
        }
        /* $AD36: a command character: match the text after it against cmd_table */
        fw_wr16(f, CMD_TABLE_PTR, CMD_TABLE);
        fw_wr8(f, CMD_TABLE_OFS, 0);
        fw_wr16(f, CMD_TEXT_PTR, (uint16_t)(BOOT_CLAUSE + x));  /* 0 + X never carries: the INC at $AD4D is dead */
        fw_wr8(f, FW_TMP_78, y);
        uint8_t len = cmd_match_table(f);
        if (len == 0) {                                          /* $AD58: no command: the character stays, raw */
            y = fw_rd8(f, FW_TMP_78);
            a = fw_rd8(f, CMD_CHAR);
            fw_wr8(f, (uint16_t)(CMD_TEXT + y++), a);
            continue;
        }
        /* $AD63: queue command n, cut out of cmd_text. Any len != 0 counts, a partial match on a zero byte too:
           its handler bytes then come from inside the pattern (the ROM's latent bug, kept). */
        uint8_t n = fw_rd8(f, CMD_COUNT);
        fw_wr8(f, CMD_COUNT, (uint8_t)(n + 1));
        uint8_t resume = (uint8_t)(x + len);
        fw_wr8(f, FW_TMP_76, resume);
        fw_wr8(f, (uint16_t)(CMD_ARG_INDEX + n), (uint8_t)(resume - 1));   /* its last matched character */
        fw_wr8(f, (uint16_t)(CMD_POSITIONS + n), fw_rd8(f, FW_TMP_78));
        uint8_t h = (uint8_t)(n << 1);                                     /* ASL: an 8-bit index */
        fw_wr8(f, (uint16_t)(CMD_HANDLERS + h), fw_ind_y(f, CMD_PAT_PTR, (uint8_t)(len + 1)));
        a = fw_ind_y(f, CMD_PAT_PTR, (uint8_t)(len + 2));
        fw_wr8(f, (uint16_t)(CMD_HANDLERS + 1 + h), a);
        x = fw_rd8(f, FW_TMP_76);
        y = fw_rd8(f, FW_TMP_78);
    } while (x < fw_rd8(f, BOOT_CLAUSE_LEN));       /* $AD27 */
    fw_wr8(f, BOOT_CLAUSE_LEN, y);
    fw_wr8(f, CMD_NEXT_POS, fw_rd8(f, CMD_POSITIONS));
}
