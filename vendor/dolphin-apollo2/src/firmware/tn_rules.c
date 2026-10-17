#include "tn_rules.h"
#include "data/fw_data.h"

uint8_t tn_append_phonetic(fw *f, uint8_t len, uint8_t *x, uint8_t *y)
{
    if (len == 0)
        return 0xFF;
    fw_wr8(f, FW_TMP_78, len);
    *x = fw_rd8(f, TN_PHON_LEN);
    if (fw_rd8(f, CMD_VOICE)) {
        do {
            (*y)++;
            fw_wr8(f, (uint16_t)(TN_PHONETIC + *x), tn_rule_byte(f, *y));
            (*x)++;
            fw_wr8(f, FW_TMP_78, (uint8_t)(fw_rd8(f, FW_TMP_78) - 1));
        } while (fw_rd8(f, FW_TMP_78) != 0);
    }
    fw_wr8(f, TN_PHON_LEN, *x);
    return 0xFF;
}

uint8_t tn_rule_skip_text(fw *f, uint8_t *y)
{
    uint8_t a;
    while ((a = tn_rule_byte(f, *y)) >= 0x1E)
        (*y)++;
    return a;
}

void tn_rule_skip_field(fw *f, uint8_t a, uint8_t *y)
{
    uint8_t y1 = (uint8_t)(*y + 1);
    fw_wr8(f, FW_TMP_78, y1);
    /* CLC/ADC $78 then CLC/ADC tn_rule_ptr: the 8-bit sum a + y + 1 is added to the 16-bit pointer */
    fw_wr16(f, TN_RULE_PTR, (uint16_t)(fw_rd16(f, TN_RULE_PTR) + (uint8_t)(a + y1)));
    *y = 0;
}

uint8_t tn_say_char(fw *f, uint8_t c)
{
    uint8_t y = 0;
    fw_wr8(f, TN_RULE_PTR, fw_rom8(f, (uint16_t)(TN_CHAR_RULES_LO + c)));
    fw_wr8(f, TN_RULE_PTR + 1, fw_rom8(f, (uint16_t)(TN_CHAR_RULES_HI + c)));
    uint8_t n = tn_rule_skip_text(f, &y);   /* past the left context to the count n */
    tn_rule_skip_field(f, n, &y);           /* past n and the n consumed characters */
    tn_rule_skip_text(f, &y);               /* past the rest of the right context to the length */
    uint8_t x0 = 0;                         /* X: reloaded from tn_pos below, as in the ROM */
    tn_append_phonetic(f, tn_rule_byte(f, y), &x0, &y);
    uint8_t x = fw_rd8(f, TN_POS);
    fw_wr8(f, TN_POS, (uint8_t)(x + 1));
    return x;
}
