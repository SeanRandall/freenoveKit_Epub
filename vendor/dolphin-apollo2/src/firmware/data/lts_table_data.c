/* Converted from the Apollo 2 ROM by tools/extract_tables.py (M4.0 spec section 4), then reviewed.
   tests/test_firmware_data.c re-encodes it and compares it byte for byte with the ROM. */
#include <stddef.h>
#include "fw_data.h"


const fw_rule_list *const *const lts_pair_lists[26] = {
    lts_pairs_a, lts_pairs_b, lts_pairs_c, lts_pairs_d, lts_pairs_e, lts_pairs_f, lts_pairs_g, lts_pairs_h, lts_pairs_i, lts_pairs_j, lts_pairs_k, lts_pairs_l, lts_pairs_m, lts_pairs_n, lts_pairs_o, lts_pairs_p, lts_pairs_q, lts_pairs_r, lts_pairs_s, lts_pairs_t, lts_pairs_u, lts_pairs_v, lts_pairs_w, lts_pairs_x, lts_pairs_y, lts_pairs_z,
};

const fw_rule_table *const fw_rule_tables[] = {
    &tn_rules_ctrl,
    &tn_rules_punct,
    &tn_rules_digits,
    &tn_rules_punct2,
    &lts_rules_letters,
    &tn_rules_symbols3,
    &tn_rules_lower,
    &tn_rules_symbols4,
    &tn_rules_high,
    &lts_rules_a,
    &lts_rules_b,
    &lts_rules_c,
    &lts_rules_d,
    &lts_rules_e,
    &lts_rules_f,
    &lts_rules_g,
    &lts_rules_h,
    &lts_rules_i,
    &lts_rules_j,
    &lts_rules_k,
    &lts_rules_l,
    &lts_rules_m,
    &lts_rules_n,
    &lts_rules_o,
    &lts_rules_p,
    &lts_rules_q,
    &lts_rules_r,
    &lts_rules_s,
    &lts_rules_t,
    &lts_rules_u,
    &lts_rules_v,
    &lts_rules_w,
    &lts_rules_x,
    &lts_rules_y,
    &lts_rules_z,
};
const size_t fw_rule_table_count = sizeof fw_rule_tables / sizeof fw_rule_tables[0];

static const char *const k_lts_suffixes[] = { "ABLE", "AL", "ATION", "EABLE", "ED", "EMENT", "EN", "ER", "ES", "E ", "ING" };
static const char *const k_lts_sibilants[] = { "S", "CE", "CI", "G", "Z", "X", "J", "CH", "SH" };
static const char *const k_lts_at_consonants[] = { "S", "R", "J", "L", "CH", "SH" };
const fw_affix_list fw_affix_lists[] = {
    { "lts_suffixes", 0x9825, k_lts_suffixes, 11 },
    { "lts_sibilants", 0x9861, k_lts_sibilants, 9 },
    { "lts_at_consonants", 0x9884, k_lts_at_consonants, 6 },
};
const size_t fw_affix_list_count = sizeof fw_affix_lists / sizeof fw_affix_lists[0];

const uint8_t lts_char_class[256] = {
    [0x21] = CC_PUNCT,   /* '!' ($21) */
    [0x22] = CC_PUNCT,   /* '"' ($22) */
    [0x23] = CC_PUNCT,   /* '#' ($23) */
    [0x24] = CC_PUNCT,   /* '$' ($24) */
    [0x25] = CC_PUNCT,   /* '%' ($25) */
    [0x26] = CC_PUNCT,   /* '&' ($26) */
    [0x27] = CC_LETTER,   /* $27 */
    [0x28] = CC_PUNCT,   /* '(' ($28) */
    [0x29] = CC_PUNCT,   /* ')' ($29) */
    [0x2A] = CC_PUNCT,   /* '*' ($2A) */
    [0x2B] = CC_PUNCT,   /* '+' ($2B) */
    [0x2C] = CC_PUNCT,   /* ',' ($2C) */
    [0x2D] = CC_PUNCT,   /* '-' ($2D) */
    [0x2E] = CC_PUNCT,   /* '.' ($2E) */
    [0x2F] = CC_PUNCT,   /* '/' ($2F) */
    [0x3A] = CC_PUNCT,   /* ':' ($3A) */
    [0x3B] = CC_PUNCT,   /* ';' ($3B) */
    [0x3C] = CC_PUNCT,   /* '<' ($3C) */
    [0x3D] = CC_PUNCT,   /* '=' ($3D) */
    [0x3E] = CC_PUNCT,   /* '>' ($3E) */
    [0x3F] = CC_PUNCT,   /* '?' ($3F) */
    [0x40] = CC_PUNCT,   /* '@' ($40) */
    [0x41] = CC_LETTER | CC_VOWEL,   /* 'A' ($41) */
    [0x42] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'B' ($42) */
    [0x43] = CC_LETTER | CC_CONSONANT,   /* 'C' ($43) */
    [0x44] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'D' ($44) */
    [0x45] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL,   /* 'E' ($45) */
    [0x46] = CC_LETTER | CC_CONSONANT,   /* 'F' ($46) */
    [0x47] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'G' ($47) */
    [0x48] = CC_LETTER | CC_CONSONANT,   /* 'H' ($48) */
    [0x49] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL,   /* 'I' ($49) */
    [0x4A] = CC_LETTER | CC_CONSONANT,   /* 'J' ($4A) */
    [0x4B] = CC_LETTER | CC_CONSONANT,   /* 'K' ($4B) */
    [0x4C] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'L' ($4C) */
    [0x4D] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* 'M' ($4D) */
    [0x4E] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* 'N' ($4E) */
    [0x4F] = CC_LETTER | CC_VOWEL,   /* 'O' ($4F) */
    [0x50] = CC_LETTER | CC_CONSONANT,   /* 'P' ($50) */
    [0x51] = CC_LETTER | CC_CONSONANT,   /* 'Q' ($51) */
    [0x52] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'R' ($52) */
    [0x53] = CC_LETTER | CC_CONSONANT,   /* 'S' ($53) */
    [0x54] = CC_LETTER | CC_CONSONANT,   /* 'T' ($54) */
    [0x55] = CC_LETTER | CC_VOWEL,   /* 'U' ($55) */
    [0x56] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'V' ($56) */
    [0x57] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'W' ($57) */
    [0x58] = CC_LETTER | CC_CONSONANT,   /* 'X' ($58) */
    [0x59] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL,   /* 'Y' ($59) */
    [0x5A] = CC_LETTER | CC_CONSONANT,   /* 'Z' ($5A) */
    [0x5B] = CC_PUNCT,   /* '[' ($5B) */
    [0x5C] = CC_PUNCT,   /* $5C */
    [0x5D] = CC_PUNCT,   /* ']' ($5D) */
    [0x5E] = CC_PUNCT,   /* '^' ($5E) */
    [0x5F] = CC_LETTER | CC_VOWEL,   /* '_' ($5F) */
    [0x60] = CC_PUNCT,   /* '`' ($60) */
    [0x61] = CC_LETTER | CC_VOWEL,   /* 'a' ($61) */
    [0x62] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'b' ($62) */
    [0x63] = CC_LETTER | CC_CONSONANT,   /* 'c' ($63) */
    [0x64] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'd' ($64) */
    [0x65] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL,   /* 'e' ($65) */
    [0x66] = CC_LETTER | CC_CONSONANT,   /* 'f' ($66) */
    [0x67] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'g' ($67) */
    [0x68] = CC_LETTER | CC_CONSONANT,   /* 'h' ($68) */
    [0x69] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL,   /* 'i' ($69) */
    [0x6A] = CC_LETTER | CC_CONSONANT,   /* 'j' ($6A) */
    [0x6B] = CC_LETTER | CC_CONSONANT,   /* 'k' ($6B) */
    [0x6C] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'l' ($6C) */
    [0x6D] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* 'm' ($6D) */
    [0x6E] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* 'n' ($6E) */
    [0x6F] = CC_LETTER | CC_VOWEL,   /* 'o' ($6F) */
    [0x70] = CC_LETTER | CC_CONSONANT,   /* 'p' ($70) */
    [0x71] = CC_LETTER | CC_CONSONANT,   /* 'q' ($71) */
    [0x72] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'r' ($72) */
    [0x73] = CC_LETTER | CC_CONSONANT,   /* 's' ($73) */
    [0x74] = CC_LETTER | CC_CONSONANT,   /* 't' ($74) */
    [0x75] = CC_LETTER | CC_VOWEL,   /* 'u' ($75) */
    [0x76] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* 'v' ($76) */
    [0x77] = CC_LETTER | CC_CONSONANT,   /* 'w' ($77) */
    [0x78] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL,   /* 'x' ($78) */
    [0x79] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_CONSONANT,   /* 'y' ($79) */
    [0x7A] = CC_LETTER | CC_CONSONANT,   /* 'z' ($7A) */
    [0x7B] = CC_PUNCT,   /* '{' ($7B) */
    [0x7C] = CC_PUNCT,   /* '|' ($7C) */
    [0x7D] = CC_PUNCT,   /* '}' ($7D) */
    [0x7E] = CC_PUNCT,   /* '~' ($7E) */
    [0x80] = CC_LETTER | CC_CONSONANT,   /* $80 */
    [0x81] = CC_LETTER | CC_VOWEL,   /* $81 */
    [0x82] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $82 */
    [0x83] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $83 */
    [0x84] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $84 */
    [0x85] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $85 */
    [0x86] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $86 */
    [0x87] = CC_LETTER | CC_CONSONANT,   /* $87 */
    [0x88] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $88 */
    [0x89] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $89 */
    [0x8A] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $8A */
    [0x8B] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $8B */
    [0x8C] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $8C */
    [0x8D] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $8D */
    [0x8E] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $8E */
    [0x8F] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $8F */
    [0x90] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $90 */
    [0x91] = CC_LETTER | CC_VOWEL,   /* $91 */
    [0x92] = CC_LETTER | CC_VOWEL,   /* $92 */
    [0x93] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $93 */
    [0x94] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $94 */
    [0x95] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $95 */
    [0x96] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $96 */
    [0x97] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $97 */
    [0x98] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $98 */
    [0x99] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $99 */
    [0x9A] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $9A */
    [0x9B] = CC_LETTER | CC_CONSONANT,   /* $9B */
    [0x9C] = CC_PUNCT,   /* $9C */
    [0xA0] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $A0 */
    [0xA1] = CC_LETTER | CC_VOWEL | CC_FRONT_VOWEL | CC_HIGH,   /* $A1 */
    [0xA2] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $A2 */
    [0xA3] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $A3 */
    [0xA4] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* $A4 */
    [0xA5] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* $A5 */
    [0xA6] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $A6 */
    [0xA7] = CC_LETTER | CC_VOWEL | CC_HIGH,   /* $A7 */
    [0xC0] = CC_LETTER | CC_VOICED,   /* $C0 */
    [0xE0] = CC_LETTER | CC_VOWEL,   /* $E0 */
    [0xE1] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* $E1 */
    [0xE2] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* $E2 */
    [0xE3] = CC_LETTER | CC_CONSONANT,   /* $E3 */
    [0xE4] = CC_LETTER | CC_CONSONANT,   /* $E4 */
    [0xE5] = CC_LETTER | CC_CONSONANT,   /* $E5 */
    [0xE6] = CC_LETTER | CC_NASAL | CC_VOICED | CC_CONSONANT,   /* $E6 */
    [0xE7] = CC_LETTER | CC_CONSONANT,   /* $E7 */
    [0xE8] = CC_LETTER | CC_CONSONANT,   /* $E8 */
    [0xE9] = CC_LETTER | CC_CONSONANT,   /* $E9 */
    [0xEA] = CC_LETTER | CC_VOWEL,   /* $EA */
    [0xEB] = CC_LETTER | CC_VOICED | CC_CONSONANT,   /* $EB */
    [0xEC] = CC_PUNCT,   /* $EC */
    [0xED] = CC_LETTER | CC_CONSONANT,   /* $ED */
    [0xEE] = CC_PUNCT,   /* $EE */
    [0xEF] = CC_PUNCT,   /* $EF */
    [0xF0] = CC_PUNCT,   /* $F0 */
    [0xF1] = CC_PUNCT,   /* $F1 */
    [0xF2] = CC_PUNCT,   /* $F2 */
    [0xF3] = CC_PUNCT,   /* $F3 */
    [0xF4] = CC_PUNCT,   /* $F4 */
    [0xF5] = CC_PUNCT,   /* $F5 */
    [0xF6] = CC_PUNCT,   /* $F6 */
    [0xF7] = CC_PUNCT,   /* $F7 */
    [0xF8] = CC_PUNCT,   /* $F8 */
    [0xFB] = CC_PUNCT,   /* $FB */
    [0xFC] = CC_PUNCT,   /* $FC */
    [0xFD] = CC_PUNCT,   /* $FD */
};
