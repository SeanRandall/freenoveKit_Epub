#include "cmd_match.h"
#include "fw_ops.h"
#include "data/fw_data.h"

uint8_t cmd_upcase(fw *f, uint8_t c) { return fw_rom8(f, (uint16_t)(CMD_CASE_MAP + c)); }

uint8_t cmd_hex_value(fw *f, uint8_t c)
{
    uint8_t a = cmd_upcase(f, c);
    if (a >= 0x3A && a < 0x41)
        a = 0x5A;                                       /* $C52B: ':'..'@' count as 'Z' */
    uint8_t d = (uint8_t)(a - 0x41);                    /* SEC; SBC #'A' */
    return (d & 0x80) ? (uint8_t)(d + 0x11)             /* BMI: bit 7 of the difference decides, not a compare */
                      : (uint8_t)(d + 0x0A);
}

uint8_t cmd_hex_byte(fw *f)
{
    fw_wr8(f, FW_TMP_1809, util_shl4(cmd_hex_value(f, fw_rd8(f, FW_TMP_34))));
    return (uint8_t)(cmd_hex_value(f, fw_rd8(f, FW_TMP_35)) + fw_rd8(f, FW_TMP_1809));
}

uint8_t cmd_find_back(fw *f, uint8_t letter, uint8_t x)
{
    fw_wr8(f, FW_TMP_75, letter);
    for (;;) {
        x--;
        if (x == 0 || cmd_upcase(f, fw_rd8(f, (uint16_t)(BOOT_CLAUSE + x))) == fw_rd8(f, FW_TMP_75))
            break;
    }
    return (uint8_t)(x + 1);
}

uint8_t cmd_match_pattern(fw *f)
{
    uint8_t y = 0xFF;
    for (;;) {
        y++;
        uint8_t t = fw_ind_y(f, CMD_TEXT_PTR, y);
        if (t == 0)
            return y;                                   /* the text ends: a partial match (0 at index 0) */
        uint8_t u = cmd_upcase(f, t);                   /* the ROM inlines the table read (LDA $99B4,X) */
        uint8_t p = fw_ind_y(f, CMD_PAT_PTR, y);
        if (p == 0)
            return y;                                   /* the pattern ends: a full match */
        if (p == '*')
            continue;
        if (p == '#') {
            if (u < '0' || u >= ':')
                return 0;
            continue;
        }
        if (u != p)
            return 0;
    }
}

uint8_t cmd_match_table(fw *f)
{
    for (;;) {
        uint8_t ofs = fw_rd8(f, CMD_TABLE_OFS);
        fw_wr8(f, CMD_PAT_PTR, fw_ind_y(f, CMD_TABLE_PTR, ofs));
        fw_wr8(f, CMD_PAT_PTR + 1, fw_ind_y(f, CMD_TABLE_PTR, (uint8_t)(ofs + 1)));
        if (fw_ind_y(f, CMD_PAT_PTR, 0) == 0xFF)
            return 0;                                   /* the end marker */
        uint8_t y = cmd_match_pattern(f);
        if (y != 0)
            return y;
        fw_wr8(f, CMD_TABLE_OFS, (uint8_t)(ofs + 2));   /* INC; INC */
    }
}
