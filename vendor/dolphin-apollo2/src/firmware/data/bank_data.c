/* Converted from the Apollo 2 ROM (M4.6c1 spec 3.5), then reviewed: bank_ramcode_image $ADFE-$AFFC, which
   bank_load_ramcode copies to RAM $0200-$03FE, and the main bank's header. Code is an instruction listing that the
   encoder (fw_data.c, step 12) assembles. tests/test_firmware_data.c re-encodes it and compares it byte for byte with
   the ROM. */
#include "fw_data.h"
#include "../bank_addr.h"

/* The labels of the letter-to-sound code $AF11-$AFFD (ghidra740/annotations/lts.txt) */
#define LTS_AFFIX_NEXT          0xAF11
#define LTS_AFFIX_PREV          0xAF1C
#define LTS_AFFIX_MATCH_BACK    0xAF69
#define LTS_STEP_BACK           0xB004
/* Loop heads inside it, unnamed in the annotations: the JMPs back to them */
#define LTS_SUFFIX_NEXT_ENTRY   0xAF33   /* lts_sym_suffix: the next entry, from lts_match_pos again (LDX) */
#define LTS_SUFFIX_NEXT_CHAR    0xAF35   /* lts_sym_suffix: the entry's next character against the text */
#define LTS_MATCH_BACK_NEXT     0xAF6B   /* lts_affix_match_back: the entry's next character, backwards */
#define LTS_AFFIX_FIRST         3      /* lts_sym_suffix: the index of lts_suffixes' first entry (after $00 CR LF) */
#define LTS_SIBILANTS_LAST      0x1F   /* lts_sym_sibilant: the index of lts_sibilants' last character */
#define LTS_AT_CONSONANTS_LAST  0x14   /* lts_sym_at: the index of lts_at_consonants' last character */

/* bank_p2_table ($0200 in RAM) and bank_p2_table_alt ($0208): the P2 bank selection per bank number 0-7. bank_select
   reads them as $0200,X and $0208,X, so a bank number 8-15 reads the alt table through the first. */
const uint8_t fw_bank_p2_table[8] = {
    P2_MAIN_BANK,   /* 0: unused by bank_scan (1-7) */
    P2_MAIN_BANK, P2_BANK_2, P2_BANK_3, P2_BANK_4, P2_BANK_5, P2_BANK_6, P2_BANK_7,
};
const uint8_t fw_bank_p2_table_alt[8] = {
    P2_MAIN_BANK, P2_MAIN_BANK, P2_BANK_5, P2_BANK_6, P2_BANK_7, P2_MAIN_BANK, P2_MAIN_BANK, P2_MAIN_BANK,
};

/* The bank routines $0210-$0312, at their RAM addresses (ROM: + BANK_RAMCODE_OFFSET) */
const fw_insn fw_bank_ramcode[] = {
    /* bank_enter: A = bank 1-7; absent: bank_restore; else the stack reset and the warm or cold entry */
    { 0x0210, FWI_PHA, FWM_IMP },
    { 0x0211, FWI_PHP, FWM_IMP },
    { 0x0212, FWI_SEI, FWM_IMP },
    { 0x0213, FWI_JSR, FWM_ABS, BANK_SELECT },
    { 0x0216, FWI_JSR, FWM_ABS, BANK_PROBE },
    { 0x0219, FWI_BBC, FWM_BIT_ZP_REL, BANK_ABSENT, 0x021F, 0 },
    { 0x021C, FWI_JMP, FWM_ABS, BANK_RESTORE },
    { 0x021F, FWI_LDX, FWM_IMM, BANK_STACK_TOP },
    { 0x0221, FWI_TXS, FWM_IMP },
    { 0x0222, FWI_LDA, FWM_ABS, BANK_HDR_VERSION + 1 },
    { 0x0225, FWI_CMP, FWM_ABS, BANK_VER_MINOR },
    { 0x0228, FWI_BNE, FWM_REL, 0, 0x0235 },
    { 0x022A, FWI_LDA, FWM_ABS, BANK_HDR_VERSION },
    { 0x022D, FWI_CMP, FWM_ABS, BANK_VER_MAJOR },
    { 0x0230, FWI_BNE, FWM_REL, 0, 0x0235 },
    { 0x0232, FWI_JMP, FWM_ABS, BANK_ENTRY_WARM },
    { 0x0235, FWI_LDA, FWM_IMM, 0 },
    { 0x0237, FWI_LDX, FWM_IMM, BANK_ZP_CLEAR_LAST },
    { 0x0239, FWI_STA, FWM_ZPX, 0x00 },
    { 0x023B, FWI_DEX, FWM_IMP },
    { 0x023C, FWI_BNE, FWM_REL, 0, 0x0239 },
    { 0x023E, FWI_JMP, FWM_ABS, BANK_ENTRY_COLD },
    /* bank_scan: banks 7 down to 1, then its tail bank_restore $0252 */
    { 0x0241, FWI_PHA, FWM_IMP },
    { 0x0242, FWI_PHP, FWM_IMP },
    { 0x0243, FWI_SEI, FWM_IMP },
    { 0x0244, FWI_LDA, FWM_IMM, BANK_COUNT },
    { 0x0246, FWI_JSR, FWM_ABS, BANK_SELECT },
    { 0x0249, FWI_JSR, FWM_ABS, BANK_PROBE },
    { 0x024C, FWI_JSR, FWM_ABS, BANK_READ_ID },
    { 0x024F, FWI_DEC, FWM_ACC },
    { 0x0250, FWI_BNE, FWM_REL, 0, 0x0246 },
    { 0x0252, FWI_LDA, FWM_ZP, FW_TMP_75 },
    { 0x0254, FWI_STA, FWM_ZP, SFR_P2 },
    { 0x0256, FWI_LDX, FWM_ZP, FW_TMP_76 },
    { 0x0258, FWI_LDY, FWM_ZP, FW_TMP_78 },
    { 0x025A, FWI_PLP, FWM_IMP },
    { 0x025B, FWI_NOP, FWM_IMP },
    { 0x025C, FWI_PLA, FWM_IMP },
    { 0x025D, FWI_RTS, FWM_IMP },
    /* bank_select: A = bank; P2 bits 2-6 from bank_p2_table[A] (bank_p2_table_alt with DIP bit 1) */
    { 0x025E, FWI_PHA, FWM_IMP },
    { 0x025F, FWI_TAX, FWM_IMP },
    { 0x0260, FWI_LDA, FWM_ABX, BANK_P2_TABLE },
    { 0x0263, FWI_BBC, FWM_BIT_ZP_REL, BOOT_DIP, 0x0269, 1 },
    { 0x0266, FWI_LDA, FWM_ABX, BANK_P2_TABLE_ALT },
    { 0x0269, FWI_TAX, FWM_IMP },
    { 0x026A, FWI_LDA, FWM_IMM, P2_KEEP },
    { 0x026C, FWI_AND, FWM_ZP, SFR_P2 },
    { 0x026E, FWI_STA, FWM_ZP, TN_RESUME_POS },
    { 0x0270, FWI_TXA, FWM_IMP },
    { 0x0271, FWI_ORA, FWM_ZP, TN_RESUME_POS },
    { 0x0273, FWI_STA, FWM_ZP, SFR_P2 },
    { 0x0275, FWI_PLA, FWM_IMP },
    { 0x0276, FWI_RTS, FWM_IMP },
    /* bank_probe: bank_absent bit 0 unless the selected bank has the signature at $FFD0 */
    { 0x0277, FWI_PHA, FWM_IMP },
    { 0x0278, FWI_SEB, FWM_BIT_ZP, BANK_ABSENT, 0, 0 },
    { 0x027A, FWI_LDA, FWM_ABS, BANK_ENTRY_COLD },
    { 0x027D, FWI_EOR, FWM_IMM, BANK_SIGNATURE },
    { 0x027F, FWI_BNE, FWM_REL, 0, 0x0283 },
    { 0x0281, FWI_CLB, FWM_BIT_ZP, BANK_ABSENT, 0, 0 },
    { 0x0283, FWI_PLA, FWM_IMP },
    { 0x0284, FWI_RTS, FWM_IMP },
    /* bank_read_id: A = bank; 14 ID characters to bank_id_table + 14 x (A - 1) */
    { 0x0285, FWI_PHA, FWM_IMP },
    { 0x0286, FWI_TAX, FWM_IMP },
    { 0x0287, FWI_LDA, FWM_IMM, 0 },
    { 0x0289, FWI_DEX, FWM_IMP },
    { 0x028A, FWI_BEQ, FWM_REL, 0, 0x0291 },
    { 0x028C, FWI_CLC, FWM_IMP },
    { 0x028D, FWI_ADC, FWM_IMM, BANK_ID_CHARS },
    { 0x028F, FWI_BRA, FWM_REL, 0, 0x0289 },
    { 0x0291, FWI_TAX, FWM_IMP },
    { 0x0292, FWI_LDY, FWM_IMM, 0 },
    { 0x0294, FWI_BBS, FWM_BIT_ZP_REL, BANK_ABSENT, 0x02D4, 0 },
    { 0x0297, FWI_LDA, FWM_ABY, BANK_HDR_LANG_CODE },
    { 0x029A, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x029D, FWI_INY, FWM_IMP },
    { 0x029E, FWI_CPY, FWM_IMM, BANK_LANG_CODE_CHARS },
    { 0x02A0, FWI_BCC, FWM_REL, 0, 0x0297 },
    { 0x02A2, FWI_LDA, FWM_ABS, BANK_HDR_ENGINE_VER_TEXT },
    { 0x02A5, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02A8, FWI_LDA, FWM_ABS, BANK_HDR_ENGINE_VER_TEXT + 1 },
    { 0x02AB, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02AE, FWI_LDA, FWM_ABS, BANK_HDR_ENGINE_VER_TEXT + 2 },
    { 0x02B1, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02B4, FWI_LDA, FWM_ABS, BANK_HDR_ENGINE_VER_TEXT + 3 },
    { 0x02B7, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02BA, FWI_LDA, FWM_ABS, BANK_HDR_LANG_VER_TEXT },
    { 0x02BD, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02C0, FWI_LDA, FWM_ABS, BANK_HDR_LANG_VER_TEXT + 1 },
    { 0x02C3, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02C6, FWI_LDA, FWM_ABS, BANK_HDR_LANG_VER_TEXT + 2 },
    { 0x02C9, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02CC, FWI_LDA, FWM_ABS, BANK_HDR_LANG_VER_TEXT + 3 },
    { 0x02CF, FWI_JSR, FWM_ABS, BANK_PUT_ID_CHAR },
    { 0x02D2, FWI_BRA, FWM_REL, 0, 0x02DF },
    { 0x02D4, FWI_LDA, FWM_IMM, ' ' },
    { 0x02D6, FWI_STA, FWM_ABX, BANK_ID_TABLE },
    { 0x02D9, FWI_INX, FWM_IMP },
    { 0x02DA, FWI_INY, FWM_IMP },
    { 0x02DB, FWI_CPY, FWM_IMM, BANK_ID_CHARS },
    { 0x02DD, FWI_BCC, FWM_REL, 0, 0x02D6 },
    { 0x02DF, FWI_PLA, FWM_IMP },
    { 0x02E0, FWI_RTS, FWM_IMP },
    /* bank_put_id_char */
    { 0x02E1, FWI_JSR, FWM_ABS, BANK_SAFE_CHAR },
    { 0x02E4, FWI_STA, FWM_ABX, BANK_ID_TABLE },
    { 0x02E7, FWI_INX, FWM_IMP },
    { 0x02E8, FWI_RTS, FWM_IMP },
    /* bank_safe_char */
    { 0x02E9, FWI_CMP, FWM_IMM, ' ' },
    { 0x02EB, FWI_BCS, FWM_REL, 0, 0x02EF },
    { 0x02ED, FWI_LDA, FWM_IMM, '*' },
    { 0x02EF, FWI_RTS, FWM_IMP },
    /* bank_find_current */
    { 0x02F0, FWI_PHA, FWM_IMP },
    { 0x02F1, FWI_TXA, FWM_IMP },
    { 0x02F2, FWI_PHA, FWM_IMP },
    { 0x02F3, FWI_LDX, FWM_IMM, 1 },
    { 0x02F5, FWI_LDA, FWM_IMM, P2_BANK_BITS },
    { 0x02F7, FWI_AND, FWM_ZP, SFR_P2 },
    { 0x02F9, FWI_BBC, FWM_BIT_ZP_REL, BOOT_DIP, 0x0301, 1 },
    { 0x02FC, FWI_CMP, FWM_ABX, BANK_P2_TABLE_ALT },
    { 0x02FF, FWI_BRA, FWM_REL, 0, 0x0304 },
    { 0x0301, FWI_CMP, FWM_ABX, BANK_P2_TABLE },
    { 0x0304, FWI_BEQ, FWM_REL, 0, 0x030D },
    { 0x0306, FWI_INX, FWM_IMP },
    { 0x0307, FWI_CPX, FWM_IMM, BANK_COUNT + 1 },
    { 0x0309, FWI_BCC, FWM_REL, 0, 0x02F9 },
    { 0x030B, FWI_BRA, FWM_REL, 0, 0x030F },
    { 0x030D, FWI_STX, FWM_ZP, BANK_CURRENT },
    { 0x030F, FWI_PLA, FWM_IMP },
    { 0x0310, FWI_TAX, FWM_IMP },
    { 0x0311, FWI_PLA, FWM_IMP },
    { 0x0312, FWI_RTS, FWM_IMP },
};
const size_t fw_bank_ramcode_count = sizeof fw_bank_ramcode / sizeof fw_bank_ramcode[0];

/* The letter-to-sound code the copy takes along ($AF11-$AFFC, RAM $0313-$03FE; never executed in RAM), at its ROM
   addresses. Its last instruction's operand is at $AFFD, one byte past the copy. */
const fw_insn fw_bank_lts_code[] = {
    /* lts_affix_next */
    { 0xAF11, FWI_INY, FWM_IMP },
    { 0xAF12, FWI_LDA, FWM_IZY, LTS_AFFIX_PTR },
    { 0xAF14, FWI_BEQ, FWM_REL, 0, 0xAF1B },
    { 0xAF16, FWI_CMP, FWM_IMM, '\n' },
    { 0xAF18, FWI_BNE, FWM_REL, 0, 0xAF11 },
    { 0xAF1A, FWI_INY, FWM_IMP },
    { 0xAF1B, FWI_RTS, FWM_IMP },
    /* lts_affix_prev */
    { 0xAF1C, FWI_DEY, FWM_IMP },
    { 0xAF1D, FWI_LDA, FWM_IZY, LTS_AFFIX_PTR },
    { 0xAF1F, FWI_BEQ, FWM_REL, 0, 0xAF26 },
    { 0xAF21, FWI_CMP, FWM_IMM, '\r' },
    { 0xAF23, FWI_BNE, FWM_REL, 0, 0xAF1C },
    { 0xAF25, FWI_DEY, FWM_IMP },
    { 0xAF26, FWI_RTS, FWM_IMP },
    /* lts_sym_suffix */
    { 0xAF27, FWI_LDA, FWM_IMM, LTS_SUFFIXES >> 8 },
    { 0xAF29, FWI_STA, FWM_ZP, LTS_AFFIX_PTR + 1 },
    { 0xAF2B, FWI_LDA, FWM_IMM, LTS_SUFFIXES & 0xFF },
    { 0xAF2D, FWI_STA, FWM_ZP, LTS_AFFIX_PTR },
    { 0xAF2F, FWI_STY, FWM_ZP, FW_TMP_78 },
    { 0xAF31, FWI_LDY, FWM_IMM, LTS_AFFIX_FIRST },
    { 0xAF33, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAF35, FWI_LDA, FWM_IZY, LTS_AFFIX_PTR },
    { 0xAF37, FWI_BEQ, FWM_REL, 0, 0xAF66 },
    { 0xAF39, FWI_CMP, FWM_IMM, '\r' },
    { 0xAF3B, FWI_BEQ, FWM_REL, 0, 0xAF62 },
    { 0xAF3D, FWI_CMP, FWM_IMM, ' ' },
    { 0xAF3F, FWI_BNE, FWM_REL, 0, 0xAF4E },
    { 0xAF41, FWI_LDA, FWM_ABX, CMD_TEXT },
    { 0xAF44, FWI_TAY, FWM_IMP },
    { 0xAF45, FWI_LDA, FWM_ABY, LTS_CHAR_CLASS },
    { 0xAF48, FWI_BBC, FWM_BIT_A_REL, 0, 0xAF62, 7 },
    { 0xAF4A, FWI_LDA, FWM_IMM, 0 },
    { 0xAF4C, FWI_BRA, FWM_REL, 0, 0xAF66 },
    { 0xAF4E, FWI_CMP, FWM_ABX, CMD_TEXT },
    { 0xAF51, FWI_BNE, FWM_REL, 0, 0xAF5C },
    { 0xAF53, FWI_INX, FWM_IMP },
    { 0xAF54, FWI_CPX, FWM_ZP, BOOT_CLAUSE_LEN },
    { 0xAF56, FWI_BCS, FWM_REL, 0, 0xAF5C },
    { 0xAF58, FWI_INY, FWM_IMP },
    { 0xAF59, FWI_JMP, FWM_ABS, LTS_SUFFIX_NEXT_CHAR },
    { 0xAF5C, FWI_JSR, FWM_ABS, LTS_AFFIX_NEXT },
    { 0xAF5F, FWI_JMP, FWM_ABS, LTS_SUFFIX_NEXT_ENTRY },
    { 0xAF62, FWI_STX, FWM_ZP, LTS_MATCH_POS },
    { 0xAF64, FWI_LDA, FWM_IMM, 1 },
    { 0xAF66, FWI_LDY, FWM_ZP, FW_TMP_78 },
    { 0xAF68, FWI_RTS, FWM_IMP },
    /* lts_affix_match_back */
    { 0xAF69, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAF6B, FWI_LDA, FWM_IZY, LTS_AFFIX_PTR },
    { 0xAF6D, FWI_BEQ, FWM_REL, 0, 0xAF85 },
    { 0xAF6F, FWI_CMP, FWM_IMM, '\n' },
    { 0xAF71, FWI_BEQ, FWM_REL, 0, 0xAF83 },
    { 0xAF73, FWI_CMP, FWM_ABX, CMD_TEXT },
    { 0xAF76, FWI_BNE, FWM_REL, 0, 0xAF7D },
    { 0xAF78, FWI_DEX, FWM_IMP },
    { 0xAF79, FWI_DEY, FWM_IMP },
    { 0xAF7A, FWI_JMP, FWM_ABS, LTS_MATCH_BACK_NEXT },
    { 0xAF7D, FWI_JSR, FWM_ABS, LTS_AFFIX_PREV },
    { 0xAF80, FWI_JMP, FWM_ABS, LTS_AFFIX_MATCH_BACK },
    { 0xAF83, FWI_STX, FWM_ZP, LTS_MATCH_POS },
    { 0xAF85, FWI_LDY, FWM_ZP, FW_TMP_78 },
    { 0xAF87, FWI_RTS, FWM_IMP },
    /* lts_sym_literal */
    { 0xAF88, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAF8A, FWI_LDA, FWM_IZY, TN_RULE_PTR },
    { 0xAF8C, FWI_CMP, FWM_ABX, CMD_TEXT },
    { 0xAF8F, FWI_BEQ, FWM_REL, 0, 0xAF93 },
    { 0xAF91, FWI_AND, FWM_IMM, 0 },
    { 0xAF93, FWI_BBC, FWM_BIT_ZP_REL, LTS_MATCH_DIR, 0xAF99, 0 },
    { 0xAF96, FWI_INC, FWM_ZP, LTS_MATCH_POS },
    { 0xAF98, FWI_RTS, FWM_IMP },
    { 0xAF99, FWI_DEC, FWM_ZP, LTS_MATCH_POS },
    { 0xAF9B, FWI_RTS, FWM_IMP },
    /* lts_sym_sibilant */
    { 0xAF9C, FWI_LDA, FWM_IMM, LTS_SIBILANTS >> 8 },
    { 0xAF9E, FWI_STA, FWM_ZP, LTS_AFFIX_PTR + 1 },
    { 0xAFA0, FWI_LDA, FWM_IMM, LTS_SIBILANTS & 0xFF },
    { 0xAFA2, FWI_STA, FWM_ZP, LTS_AFFIX_PTR },
    { 0xAFA4, FWI_STY, FWM_ZP, FW_TMP_78 },
    { 0xAFA6, FWI_LDY, FWM_IMM, LTS_SIBILANTS_LAST },
    { 0xAFA8, FWI_JMP, FWM_ABS, LTS_AFFIX_MATCH_BACK },
    /* lts_sym_at */
    { 0xAFAB, FWI_LDA, FWM_IMM, LTS_AT_CONSONANTS >> 8 },
    { 0xAFAD, FWI_STA, FWM_ZP, LTS_AFFIX_PTR + 1 },
    { 0xAFAF, FWI_LDA, FWM_IMM, LTS_AT_CONSONANTS & 0xFF },
    { 0xAFB1, FWI_STA, FWM_ZP, LTS_AFFIX_PTR },
    { 0xAFB3, FWI_STY, FWM_ZP, FW_TMP_78 },
    { 0xAFB5, FWI_LDY, FWM_IMM, LTS_AT_CONSONANTS_LAST },
    { 0xAFB7, FWI_JMP, FWM_ABS, LTS_AFFIX_MATCH_BACK },
    /* lts_sym_voiced */
    { 0xAFBA, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAFBC, FWI_LDA, FWM_ABX, CMD_TEXT },
    { 0xAFBF, FWI_TAX, FWM_IMP },
    { 0xAFC0, FWI_LDA, FWM_ABX, LTS_CHAR_CLASS },
    { 0xAFC3, FWI_AND, FWM_IMM, CC_VOICED },
    { 0xAFC5, FWI_BBC, FWM_BIT_ZP_REL, LTS_MATCH_DIR, LTS_STEP_BACK, 0 },
    { 0xAFC8, FWI_INC, FWM_ZP, LTS_MATCH_POS },
    { 0xAFCA, FWI_RTS, FWM_IMP },
    /* lts_sym_consonant */
    { 0xAFCB, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAFCD, FWI_LDA, FWM_ABX, CMD_TEXT },
    { 0xAFD0, FWI_TAX, FWM_IMP },
    { 0xAFD1, FWI_LDA, FWM_ABX, LTS_CHAR_CLASS },
    { 0xAFD4, FWI_AND, FWM_IMM, CC_CONSONANT },
    { 0xAFD6, FWI_BBC, FWM_BIT_ZP_REL, LTS_MATCH_DIR, LTS_STEP_BACK, 0 },
    { 0xAFD9, FWI_INC, FWM_ZP, LTS_MATCH_POS },
    { 0xAFDB, FWI_RTS, FWM_IMP },
    /* lts_sym_front_vowel */
    { 0xAFDC, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAFDE, FWI_LDA, FWM_ABX, CMD_TEXT },
    { 0xAFE1, FWI_TAX, FWM_IMP },
    { 0xAFE2, FWI_LDA, FWM_ABX, LTS_CHAR_CLASS },
    { 0xAFE5, FWI_AND, FWM_IMM, CC_FRONT_VOWEL },
    { 0xAFE7, FWI_BBC, FWM_BIT_ZP_REL, LTS_MATCH_DIR, LTS_STEP_BACK, 0 },
    { 0xAFEA, FWI_INC, FWM_ZP, LTS_MATCH_POS },
    { 0xAFEC, FWI_RTS, FWM_IMP },
    /* lts_sym_nonletter (its last two instructions follow at $AFFE) */
    { 0xAFED, FWI_LDX, FWM_ZP, LTS_MATCH_POS },
    { 0xAFEF, FWI_LDA, FWM_ABX, CMD_TEXT },
    { 0xAFF2, FWI_TAX, FWM_IMP },
    { 0xAFF3, FWI_LDA, FWM_ABX, LTS_CHAR_CLASS },
    { 0xAFF6, FWI_BBC, FWM_BIT_A_REL, 0, 0xAFFC, 7 },
    { 0xAFF8, FWI_LDA, FWM_IMM, 0 },
    { 0xAFFA, FWI_BRA, FWM_REL, 0, 0xAFFE },
    { 0xAFFC, FWI_LDA, FWM_IMM, 1 },
};
const size_t fw_bank_lts_code_count = sizeof fw_bank_lts_code / sizeof fw_bank_lts_code[0];

/* The main bank's header: bank_entry_cold $FFD0 (its SEI is the signature bank_probe tests); bank_entry_warm $FFD4
   is not read */
const fw_insn fw_bank_entry_code[] = {
    { 0xFFD0, FWI_SEI, FWM_IMP },
    { 0xFFD1, FWI_JMP, FWM_ABS, ROM_BOOT_REENTRY_COLD },
};
const size_t fw_bank_entry_code_count = sizeof fw_bank_entry_code / sizeof fw_bank_entry_code[0];
const uint8_t fw_bank_hdr_version[2] = { 2, 1 };   /* $FFE0: the engine version 2.01, major then minor */
const char fw_bank_hdr_lang_code[7] = "000440";    /* $FFE2: ID part 1 */
const char fw_bank_hdr_lang_ver_text[5] = "0404";  /* $FFEA: ID part 3 */
const char fw_bank_hdr_engine_ver_text[5] = "0201";   /* $9800: ID part 2 */
