/* Converted from the Apollo 2 ROM by tools/extract_tables.py (M4.3a spec section 3), then reviewed.
   tests/test_firmware_data.c re-encodes it and compares it byte for byte with the ROM. */
#include <stddef.h>
#include "fw_data.h"

/* ph_symbol_class ($9BB4): the class bits of each symbol code of tn_phonetic. Codes not listed have
   none, among them the control codes, the space and the prosody markers ` { | } ~. */
const uint8_t fw_ph_symbol_class[256] = {
    [0x21] = PHC_PUNCT,   /* '!' ($21) */
    [0x22] = PHC_PUNCT,   /* '"' ($22) */
    [0x23] = PHC_PUNCT,   /* '#' ($23) */
    [0x24] = PHC_PUNCT,   /* '$' ($24) */
    [0x25] = PHC_PUNCT,   /* '%' ($25) */
    [0x26] = PHC_PUNCT,   /* '&' ($26) */
    [0x27] = PHC_PUNCT,   /* $27 */
    [0x28] = PHC_PUNCT,   /* '(' ($28) */
    [0x29] = PHC_PUNCT,   /* ')' ($29) */
    [0x2A] = PHC_PUNCT,   /* '*' ($2A) */
    [0x2B] = PHC_PUNCT,   /* '+' ($2B) */
    [0x2C] = PHC_KIND_2 | PHC_PUNCT,   /* ',' ($2C) */
    [0x2D] = PHC_PUNCT,   /* '-' ($2D) */
    [0x2E] = PHC_KIND_3 | PHC_KIND_1 | PHC_PUNCT,   /* '.' ($2E) */
    [0x2F] = PHC_PUNCT,   /* '/' ($2F) */
    [0x30] = PHC_PHONETIC,   /* '0' ($30) */
    [0x31] = PHC_PHONETIC,   /* '1' ($31) */
    [0x32] = PHC_PHONETIC,   /* '2' ($32) */
    [0x33] = PHC_PHONETIC,   /* '3' ($33) */
    [0x34] = PHC_PHONETIC,   /* '4' ($34) */
    [0x35] = PHC_PHONETIC,   /* '5' ($35) */
    [0x36] = PHC_PHONETIC,   /* '6' ($36) */
    [0x37] = PHC_PHONETIC,   /* '7' ($37) */
    [0x38] = PHC_PHONETIC,   /* '8' ($38) */
    [0x39] = PHC_PHONETIC,   /* '9' ($39) */
    [0x3A] = PHC_KIND_3 | PHC_KIND_1 | PHC_PUNCT,   /* ':' ($3A) */
    [0x3B] = PHC_KIND_3 | PHC_KIND_1 | PHC_PUNCT,   /* ';' ($3B) */
    [0x3C] = PHC_PUNCT,   /* '<' ($3C) */
    [0x3D] = PHC_PUNCT,   /* '=' ($3D) */
    [0x3E] = PHC_PUNCT,   /* '>' ($3E) */
    [0x3F] = PHC_KIND_3 | PHC_KIND_2 | PHC_PUNCT,   /* '?' ($3F) */
    [0x40] = PHC_PUNCT,   /* '@' ($40) */
    [0x41] = PHC_PHONETIC | PHC_VOWEL,   /* 'A' ($41) */
    [0x42] = PHC_PHONETIC,   /* 'B' ($42) */
    [0x43] = PHC_PHONETIC,   /* 'C' ($43) */
    [0x44] = PHC_PHONETIC,   /* 'D' ($44) */
    [0x45] = PHC_PHONETIC | PHC_VOWEL,   /* 'E' ($45) */
    [0x46] = PHC_PHONETIC,   /* 'F' ($46) */
    [0x47] = PHC_PHONETIC,   /* 'G' ($47) */
    [0x48] = PHC_PHONETIC,   /* 'H' ($48) */
    [0x49] = PHC_PHONETIC | PHC_VOWEL,   /* 'I' ($49) */
    [0x4A] = PHC_PHONETIC | PHC_VOWEL,   /* 'J' ($4A) */
    [0x4B] = PHC_PHONETIC,   /* 'K' ($4B) */
    [0x4C] = PHC_PHONETIC,   /* 'L' ($4C) */
    [0x4D] = PHC_PHONETIC,   /* 'M' ($4D) */
    [0x4E] = PHC_PHONETIC,   /* 'N' ($4E) */
    [0x4F] = PHC_PHONETIC | PHC_VOWEL,   /* 'O' ($4F) */
    [0x50] = PHC_PHONETIC,   /* 'P' ($50) */
    [0x51] = PHC_PHONETIC | PHC_VOWEL,   /* 'Q' ($51) */
    [0x52] = PHC_PHONETIC,   /* 'R' ($52) */
    [0x53] = PHC_PHONETIC,   /* 'S' ($53) */
    [0x54] = PHC_PHONETIC,   /* 'T' ($54) */
    [0x55] = PHC_PHONETIC | PHC_VOWEL,   /* 'U' ($55) */
    [0x56] = PHC_PHONETIC,   /* 'V' ($56) */
    [0x57] = PHC_PHONETIC,   /* 'W' ($57) */
    [0x58] = PHC_PHONETIC,   /* 'X' ($58) */
    [0x59] = PHC_PHONETIC,   /* 'Y' ($59) */
    [0x5A] = PHC_PHONETIC,   /* 'Z' ($5A) */
    [0x5B] = PHC_PUNCT,   /* '[' ($5B) */
    [0x5C] = PHC_PUNCT,   /* $5C */
    [0x5D] = PHC_PUNCT,   /* ']' ($5D) */
    [0x5E] = PHC_PUNCT,   /* '^' ($5E) */
    [0x5F] = PHC_PUNCT,   /* '_' ($5F) */
    [0x61] = PHC_PHONETIC | PHC_VOWEL,   /* 'a' ($61) */
    [0x62] = PHC_PHONETIC,   /* 'b' ($62) */
    [0x63] = PHC_PHONETIC,   /* 'c' ($63) */
    [0x64] = PHC_PHONETIC,   /* 'd' ($64) */
    [0x65] = PHC_PHONETIC | PHC_VOWEL,   /* 'e' ($65) */
    [0x66] = PHC_PHONETIC,   /* 'f' ($66) */
    [0x67] = PHC_PHONETIC,   /* 'g' ($67) */
    [0x68] = PHC_PHONETIC | PHC_VOWEL,   /* 'h' ($68) */
    [0x69] = PHC_PHONETIC | PHC_VOWEL,   /* 'i' ($69) */
    [0x6A] = PHC_PHONETIC,   /* 'j' ($6A) */
    [0x6B] = PHC_PHONETIC,   /* 'k' ($6B) */
    [0x6C] = PHC_PHONETIC | PHC_VOWEL,   /* 'l' ($6C) */
    [0x6D] = PHC_PHONETIC,   /* 'm' ($6D) */
    [0x6E] = PHC_PHONETIC,   /* 'n' ($6E) */
    [0x6F] = PHC_PHONETIC | PHC_VOWEL,   /* 'o' ($6F) */
    [0x70] = PHC_PHONETIC,   /* 'p' ($70) */
    [0x71] = PHC_PHONETIC | PHC_VOWEL,   /* 'q' ($71) */
    [0x72] = PHC_PHONETIC | PHC_VOWEL,   /* 'r' ($72) */
    [0x73] = PHC_PHONETIC,   /* 's' ($73) */
    [0x74] = PHC_PHONETIC,   /* 't' ($74) */
    [0x75] = PHC_PHONETIC | PHC_VOWEL,   /* 'u' ($75) */
    [0x76] = PHC_PHONETIC | PHC_VOWEL,   /* 'v' ($76) */
    [0x77] = PHC_PHONETIC | PHC_VOWEL,   /* 'w' ($77) */
    [0x78] = PHC_PHONETIC | PHC_VOWEL,   /* 'x' ($78) */
    [0x79] = PHC_PHONETIC | PHC_VOWEL,   /* 'y' ($79) */
    [0x7A] = PHC_PHONETIC,   /* 'z' ($7A) */
    [0x80] = PHC_PHONETIC,   /* $80 */
    [0x81] = PHC_PHONETIC,   /* $81 */
    [0x82] = PHC_PHONETIC,   /* $82 */
    [0x83] = PHC_PHONETIC,   /* $83 */
    [0x84] = PHC_PHONETIC,   /* $84 */
    [0x85] = PHC_PHONETIC,   /* $85 */
    [0x86] = PHC_PHONETIC,   /* $86 */
    [0x87] = PHC_PHONETIC,   /* $87 */
    [0x88] = PHC_PHONETIC,   /* $88 */
    [0x89] = PHC_PHONETIC,   /* $89 */
    [0x8A] = PHC_PHONETIC,   /* $8A */
    [0x8B] = PHC_PHONETIC,   /* $8B */
    [0x8C] = PHC_PHONETIC,   /* $8C */
    [0x8D] = PHC_PHONETIC,   /* $8D */
    [0x8E] = PHC_PHONETIC,   /* $8E */
    [0x8F] = PHC_PHONETIC,   /* $8F */
    [0x90] = PHC_PHONETIC,   /* $90 */
    [0x91] = PHC_PHONETIC,   /* $91 */
    [0x92] = PHC_PHONETIC,   /* $92 */
    [0x93] = PHC_PHONETIC,   /* $93 */
    [0x94] = PHC_PHONETIC,   /* $94 */
    [0x95] = PHC_PHONETIC,   /* $95 */
    [0x96] = PHC_PHONETIC,   /* $96 */
    [0x97] = PHC_PHONETIC,   /* $97 */
    [0x98] = PHC_PHONETIC,   /* $98 */
    [0x99] = PHC_PHONETIC,   /* $99 */
    [0x9A] = PHC_PHONETIC,   /* $9A */
    [0x9B] = PHC_PHONETIC,   /* $9B */
    [0x9C] = PHC_PUNCT,   /* $9C */
    [0xA0] = PHC_PHONETIC,   /* $A0 */
    [0xA1] = PHC_PHONETIC,   /* $A1 */
    [0xA2] = PHC_PHONETIC,   /* $A2 */
    [0xA3] = PHC_PHONETIC,   /* $A3 */
    [0xA4] = PHC_PHONETIC,   /* $A4 */
    [0xA5] = PHC_PHONETIC,   /* $A5 */
    [0xA6] = PHC_PHONETIC,   /* $A6 */
    [0xA7] = PHC_PHONETIC,   /* $A7 */
    [0xC0] = PHC_PHONETIC,   /* $C0 */
    [0xE0] = PHC_PHONETIC | PHC_VOWEL | PHC_KIND_2,   /* $E0 */
    [0xE1] = PHC_PHONETIC | PHC_VOWEL | PHC_KIND_2,   /* $E1 */
    [0xE2] = PHC_PHONETIC | PHC_VOWEL | PHC_KIND_2,   /* $E2 */
};

/* ph_symbol_features ($9CB4): the feature bits of each symbol code (codes not listed: none). Its last
   64 bytes, the features of $C0-$FF ($9D74-$9DB3), are also the list-pointer slots of the symbols
   $00-$1F, which have no class and are never decoded: the encoder writes them once, as features. */
const uint8_t fw_ph_symbol_features[256] = {
    [0x30] = PHF_VOWEL | PHF_VOICED,   /* '0' ($30) */
    [0x31] = PHF_CONSONANT | PHF_LIQUID | PHF_VOICED,   /* '1' ($31) */
    [0x32] = PHF_VOWEL | PHF_VOICED,   /* '2' ($32) */
    [0x33] = PHF_CONSONANT | PHF_LIQUID,   /* '3' ($33) */
    [0x34] = PHF_VOWEL | PHF_VOICED,   /* '4' ($34) */
    [0x35] = PHF_VOWEL | PHF_VOICED,   /* '5' ($35) */
    [0x36] = PHF_CONSONANT | PHF_STOP,   /* '6' ($36) */
    [0x37] = PHF_CONSONANT | PHF_NASAL | PHF_LONG | PHF_VOICED,   /* '7' ($37) */
    [0x39] = PHF_VOWEL | PHF_VOICED,   /* '9' ($39) */
    [0x41] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'A' ($41) */
    [0x42] = PHF_CONSONANT | PHF_STOP | PHF_VOICED,   /* 'B' ($42) */
    [0x43] = PHF_CONSONANT | PHF_STOP,   /* 'C' ($43) */
    [0x44] = PHF_CONSONANT | PHF_STOP | PHF_VOICED,   /* 'D' ($44) */
    [0x45] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'E' ($45) */
    [0x46] = PHF_CONSONANT | PHF_FRICATIVE,   /* 'F' ($46) */
    [0x47] = PHF_CONSONANT | PHF_STOP | PHF_VOICED,   /* 'G' ($47) */
    [0x48] = PHF_CONSONANT | PHF_FRICATIVE,   /* 'H' ($48) */
    [0x49] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'I' ($49) */
    [0x4A] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'J' ($4A) */
    [0x4B] = PHF_CONSONANT | PHF_STOP,   /* 'K' ($4B) */
    [0x4C] = PHF_CONSONANT | PHF_LIQUID | PHF_VOICED,   /* 'L' ($4C) */
    [0x4D] = PHF_CONSONANT | PHF_NASAL | PHF_VOICED,   /* 'M' ($4D) */
    [0x4E] = PHF_CONSONANT | PHF_NASAL | PHF_VOICED,   /* 'N' ($4E) */
    [0x4F] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'O' ($4F) */
    [0x50] = PHF_CONSONANT | PHF_STOP,   /* 'P' ($50) */
    [0x51] = PHF_VOWEL | PHF_VOICED,   /* 'Q' ($51) */
    [0x52] = PHF_CONSONANT | PHF_LIQUID | PHF_VOICED,   /* 'R' ($52) */
    [0x53] = PHF_CONSONANT | PHF_FRICATIVE,   /* 'S' ($53) */
    [0x54] = PHF_CONSONANT | PHF_STOP,   /* 'T' ($54) */
    [0x55] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'U' ($55) */
    [0x56] = PHF_CONSONANT | PHF_FRICATIVE | PHF_VOICED,   /* 'V' ($56) */
    [0x57] = PHF_CONSONANT | PHF_LIQUID | PHF_VOICED,   /* 'W' ($57) */
    [0x59] = PHF_CONSONANT | PHF_LIQUID | PHF_VOICED,   /* 'Y' ($59) */
    [0x5A] = PHF_CONSONANT | PHF_FRICATIVE | PHF_VOICED,   /* 'Z' ($5A) */
    [0x61] = PHF_VOWEL | PHF_VOICED,   /* 'a' ($61) */
    [0x62] = PHF_VOWEL | PHF_VOICED,   /* 'b' ($62) */
    [0x63] = PHF_CONSONANT | PHF_FRICATIVE,   /* 'c' ($63) */
    [0x64] = PHF_CONSONANT | PHF_FRICATIVE | PHF_VOICED,   /* 'd' ($64) */
    [0x65] = PHF_VOWEL | PHF_VOICED,   /* 'e' ($65) */
    [0x66] = PHF_CONSONANT | PHF_STOP,   /* 'f' ($66) */
    [0x67] = PHF_CONSONANT | PHF_NASAL | PHF_VOICED,   /* 'g' ($67) */
    [0x68] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'h' ($68) */
    [0x69] = PHF_VOWEL | PHF_VOICED,   /* 'i' ($69) */
    [0x6A] = PHF_CONSONANT | PHF_FRICATIVE | PHF_VOICED,   /* 'j' ($6A) */
    [0x6B] = PHF_CONSONANT | PHF_STOP,   /* 'k' ($6B) */
    [0x6C] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'l' ($6C) */
    [0x6D] = PHF_CONSONANT | PHF_NASAL | PHF_LONG | PHF_VOICED,   /* 'm' ($6D) */
    [0x6E] = PHF_CONSONANT | PHF_NASAL | PHF_VOICED,   /* 'n' ($6E) */
    [0x6F] = PHF_VOWEL | PHF_VOICED,   /* 'o' ($6F) */
    [0x70] = PHF_CONSONANT | PHF_STOP,   /* 'p' ($70) */
    [0x71] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'q' ($71) */
    [0x72] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'r' ($72) */
    [0x73] = PHF_CONSONANT | PHF_FRICATIVE,   /* 's' ($73) */
    [0x74] = PHF_CONSONANT | PHF_FRICATIVE,   /* 't' ($74) */
    [0x75] = PHF_VOWEL | PHF_VOICED,   /* 'u' ($75) */
    [0x76] = PHF_VOWEL | PHF_VOICED,   /* 'v' ($76) */
    [0x77] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'w' ($77) */
    [0x78] = PHF_VOWEL | PHF_VOICED,   /* 'x' ($78) */
    [0x79] = PHF_VOWEL | PHF_LONG | PHF_VOICED,   /* 'y' ($79) */
    [0x7A] = PHF_CONSONANT | PHF_FRICATIVE | PHF_VOICED,   /* 'z' ($7A) */
    [0x83] = PHF_VOWEL | PHF_VOICED,   /* $83 */
    [0x84] = PHF_VOWEL | PHF_VOICED,   /* $84 */
    [0x87] = PHF_CONSONANT | PHF_STOP | PHF_VOICED,   /* $87 */
    [0x8C] = PHF_CONSONANT | PHF_LIQUID,   /* $8C */
    [0x98] = PHF_CONSONANT | PHF_STOP | PHF_VOICED,   /* $98 */
    [0xC0] = PHF_CONSONANT | PHF_LIQUID | PHF_LONG | PHF_VOICED,   /* $C0; also symbol $00's pointer slot, low byte */
    [0xE0] = PHF_VOWEL | PHF_VOICED,   /* $E0; also symbol $10's pointer slot, low byte */
    [0xE1] = PHF_VOWEL | PHF_VOICED,   /* $E1; also symbol $10's pointer slot, high byte */
    [0xE2] = PHF_VOWEL | PHF_VOICED,   /* $E2; also symbol $11's pointer slot, low byte */
};

/* The phoneme lists ($A187-$A396), by symbol: the (phoneme, max, min) triples ph_decode_symbol turns
   into segments (the phoneme numbers a row of the voice tables; the duration is min plus the scaled
   max - min). The encoder lays out the lists of $20-$FF end to end in code order, each with its $FF
   end, and writes the list pointers (PH_LIST_PTRS + 2 * symbol, $9DB4-$9F73) from where they land.
   Symbols not listed have an empty list (only the end) and make no segment. */
const fw_ph_list fw_ph_lists[256] = {
    /* the space: only the word-pause item, a pause of @Q length when @Q is not 0 */
    [0x20] = { .word_pause = true },   /* $20 */
    [0x30] = { 2, { { 50, 140, 70 }, { 54, 70, 30 } } },   /* '0' ($30) */
    [0x31] = { 1, { { 39, 90, 70 } } },   /* '1' ($31) */
    [0x32] = { 1, { { 70, 160, 60 } } },   /* '2' ($32) */
    [0x33] = { 1, { { 61, 80, 40 } } },   /* '3' ($33) */
    [0x34] = { 2, { { 53, 130, 70 }, { 49, 70, 30 } } },   /* '4' ($34) */
    [0x35] = { 2, { { 29, 130, 70 }, { 30, 70, 30 } } },   /* '5' ($35) */
    [0x36] = { 3, { { 31, 80, 60 }, { 34, 20, 20 }, { 35, 40, 30 } } },   /* '6' ($36) */
    [0x37] = { 1, { { 44, 170, 100 } } },   /* '7' ($37) */
    [0x39] = { 2, { { 51, 140, 80 }, { 52, 60, 30 } } },   /* '9' ($39) */
    [0x41] = { 2, { { 4, 130, 40 }, { 5, 110, 35 } } },   /* 'A' ($41) */
    [0x42] = { 2, { { 8, 90, 55 }, { 9, 25, 10 } } },   /* 'B' ($42) */
    [0x43] = { 3, { { 31, 80, 60 }, { 36, 20, 20 }, { 37, 40, 30 } } },   /* 'C' ($43) */
    [0x44] = { 2, { { 13, 50, 35 }, { 15, 25, 10 } } },   /* 'D' ($44) */
    [0x45] = { 1, { { 16, 150, 50 } } },   /* 'E' ($45) */
    [0x46] = { 1, { { 20, 130, 50 } } },   /* 'F' ($46) */
    [0x47] = { 2, { { 21, 65, 50 }, { 24, 30, 20 } } },   /* 'G' ($47) */
    [0x48] = { 1, { { 76, 160, 40 } } },   /* 'H' ($48) */
    [0x49] = { 2, { { 29, 140, 80 }, { 5, 110, 40 } } },   /* 'I' ($49) */
    [0x4A] = { 2, { { 27, 100, 55 }, { 28, 110, 45 } } },   /* 'J' ($4A) */
    [0x4B] = { 3, { { 31, 80, 60 }, { 32, 20, 20 }, { 33, 40, 30 } } },   /* 'K' ($4B) */
    [0x4C] = { 1, { { 38, 80, 40 } } },   /* 'L' ($4C) */
    [0x4D] = { 1, { { 42, 80, 60 } } },   /* 'M' ($4D) */
    [0x4E] = { 1, { { 44, 70, 60 } } },   /* 'N' ($4E) */
    [0x4F] = { 2, { { 48, 130, 40 }, { 46, 100, 35 } } },   /* 'O' ($4F) */
    [0x50] = { 3, { { 55, 80, 60 }, { 56, 20, 20 }, { 57, 30, 20 } } },   /* 'P' ($50) */
    [0x51] = { 1, { { 47, 140, 40 } } },   /* 'Q' ($51) */
    [0x52] = { 1, { { 60, 90, 50 } } },   /* 'R' ($52) */
    [0x53] = { 1, { { 62, 125, 80 } } },   /* 'S' ($53) */
    [0x54] = { 3, { { 64, 80, 40 }, { 65, 20, 20 }, { 66, 30, 20 } } },   /* 'T' ($54) */
    [0x55] = { 1, { { 69, 210, 70 } } },   /* 'U' ($55) */
    [0x56] = { 1, { { 71, 90, 50 } } },   /* 'V' ($56) */
    [0x57] = { 1, { { 72, 80, 60 } } },   /* 'W' ($57) */
    [0x58] = { 1, { { 59, 10, 10 } } },   /* 'X' ($58) */
    [0x59] = { 1, { { 73, 110, 60 } } },   /* 'Y' ($59) */
    [0x5A] = { 1, { { 74, 140, 40 } } },   /* 'Z' ($5A) */
    [0x61] = { 1, { { 0, 140, 40 } } },   /* 'a' ($61) */
    [0x62] = { 1, { { 16, 110, 60 } } },   /* 'b' ($62) */
    [0x63] = { 3, { { 10, 70, 50 }, { 11, 20, 20 }, { 12, 120, 40 } } },   /* 'c' ($63) */
    [0x64] = { 1, { { 14, 100, 35 } } },   /* 'd' ($64) */
    [0x65] = { 1, { { 17, 150, 70 } } },   /* 'e' ($65) */
    [0x66] = { 2, { { 64, 80, 40 }, { 65, 20, 20 } } },   /* 'f' ($66) */
    [0x67] = { 2, { { 44, 70, 30 }, { 21, 60, 20 } } },   /* 'g' ($67) */
    [0x68] = { 1, { { 2, 220, 70 } } },   /* 'h' ($68) */
    [0x69] = { 1, { { 26, 110, 40 } } },   /* 'i' ($69) */
    [0x6A] = { 2, { { 13, 70, 50 }, { 75, 100, 30 } } },   /* 'j' ($6A) */
    [0x6B] = { 2, { { 31, 80, 60 }, { 32, 20, 20 } } },   /* 'k' ($6B) */
    [0x6C] = { 2, { { 17, 145, 55 }, { 19, 125, 45 } } },   /* 'l' ($6C) */
    [0x6D] = { 1, { { 43, 170, 110 } } },   /* 'm' ($6D) */
    [0x6E] = { 1, { { 45, 130, 50 } } },   /* 'n' ($6E) */
    [0x6F] = { 1, { { 3, 210, 100 } } },   /* 'o' ($6F) */
    [0x70] = { 2, { { 55, 80, 60 }, { 56, 20, 20 } } },   /* 'p' ($70) */
    [0x71] = { 2, { { 46, 125, 55 }, { 1, 110, 45 } } },   /* 'q' ($71) */
    [0x72] = { 1, { { 18, 240, 50 } } },   /* 'r' ($72) */
    [0x73] = { 1, { { 63, 180, 80 } } },   /* 's' ($73) */
    [0x74] = { 1, { { 67, 160, 60 } } },   /* 't' ($74) */
    [0x75] = { 1, { { 68, 150, 40 } } },   /* 'u' ($75) */
    [0x76] = { 1, { { 46, 110, 50 } } },   /* 'v' ($76) */
    [0x77] = { 2, { { 50, 120, 50 }, { 49, 120, 40 } } },   /* 'w' ($77) */
    [0x78] = { 1, { { 1, 90, 40 } } },   /* 'x' ($78) */
    [0x79] = { 2, { { 51, 120, 50 }, { 5, 120, 40 } } },   /* 'y' ($79) */
    [0x7A] = { 1, { { 75, 70, 40 } } },   /* 'z' ($7A) */
    /* the only list with phoneme 7; only @X phonetic text reaches it */
    [0x83] = { 1, { { 7, 90, 50 } } },   /* $83 */
    /* the only list with phoneme 6; only @X phonetic text reaches it */
    [0x84] = { 1, { { 6, 90, 50 } } },   /* $84 */
    /* the only list with phoneme 22; only @X phonetic text reaches it */
    [0x87] = { 2, { { 21, 65, 50 }, { 22, 30, 20 } } },   /* $87 */
    /* the only list with phoneme 40; only @X phonetic text reaches it */
    [0x8C] = { 1, { { 40, 80, 40 } } },   /* $8C */
    /* the only list with phoneme 23; from letter-to-sound rules (MEGATON) */
    [0x98] = { 2, { { 21, 65, 50 }, { 23, 30, 20 } } },   /* $98 */
    /* the only list with phoneme 41, a long syllabic consonant; from letter-to-sound rules (LEVEL) */
    [0xC0] = { 1, { { 41, 220, 100 } } },   /* $C0 */
};

/* ph_dur_lengthen ($C67B): one lengthening step, x to about 1.1 * x (less than 1.5 off), 255 from
   x = 233 on. ph_decode_symbol applies it ph_lengthen_count times to a duration range. */
const uint8_t fw_ph_dur_lengthen[256] = {
      0,   1,   2,   3,   4,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  17,   /* x = 0-15 */
     18,  19,  20,  21,  22,  23,  24,  25,  26,  28,  29,  30,  31,  32,  33,  34,   /* x = 16-31 */
     35,  36,  37,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  50,  51,  52,   /* x = 32-47 */
     53,  54,  55,  56,  57,  58,  59,  61,  62,  63,  64,  65,  66,  67,  68,  69,   /* x = 48-63 */
     70,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  83,  84,  85,  86,  87,   /* x = 64-79 */
     88,  89,  90,  91,  92,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 105,   /* x = 80-95 */
    106, 107, 108, 109, 110, 111, 112, 113, 114, 116, 117, 118, 119, 120, 121, 122,   /* x = 96-111 */
    123, 124, 125, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 138, 139, 140,   /* x = 112-127 */
    141, 142, 143, 144, 145, 146, 147, 149, 150, 151, 152, 153, 154, 155, 156, 157,   /* x = 128-143 */
    158, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,   /* x = 144-159 */
    175, 176, 177, 178, 179, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 192,   /* x = 160-175 */
    193, 194, 195, 196, 197, 198, 199, 200, 201, 203, 204, 205, 206, 207, 208, 209,   /* x = 176-191 */
    210, 211, 212, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 225, 226, 227,   /* x = 192-207 */
    228, 229, 230, 231, 232, 233, 234, 236, 237, 238, 239, 240, 241, 242, 243, 244,   /* x = 208-223 */
    245, 247, 248, 249, 250, 251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255,   /* x = 224-239 */
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   /* x = 240-255 */
};
/* ph_dur_shorten ($C77B): one shortening step, x to about 0.9 * x (less than 1 off).
   ph_decode_symbol applies it (shortening count + 1) / 2 times to a duration range. */
const uint8_t fw_ph_dur_shorten[256] = {
      0,   1,   2,   3,   4,   5,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,   /* x = 0-15 */
     14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  23,  24,  25,  26,  27,  28,   /* x = 16-31 */
     29,  30,  31,  32,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  41,  42,   /* x = 32-47 */
     43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  53,  54,  55,  56,  57,   /* x = 48-63 */
     58,  59,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  68,  69,  70,  71,   /* x = 64-79 */
     72,  73,  74,  75,  76,  77,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,   /* x = 80-95 */
     86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  95,  96,  97,  98,  99, 100,   /* x = 96-111 */
    101, 102, 103, 104, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 113, 114,   /* x = 112-127 */
    115, 116, 117, 118, 119, 120, 121, 122, 122, 123, 124, 125, 126, 127, 128, 129,   /* x = 128-143 */
    130, 131, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 140, 141, 142, 143,   /* x = 144-159 */
    144, 145, 146, 147, 148, 149, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158,   /* x = 160-175 */
    158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 167, 168, 169, 170, 171, 172,   /* x = 176-191 */
    173, 174, 175, 176, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 185, 186,   /* x = 192-207 */
    187, 188, 189, 190, 191, 192, 193, 194, 195, 195, 196, 197, 198, 199, 200, 201,   /* x = 208-223 */
    202, 203, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 212, 213, 214, 215,   /* x = 224-239 */
    216, 217, 218, 219, 220, 221, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230,   /* x = 240-255 */
};

/* cmd_speed_map_data ($C898): per speed 1-9 (cmd_speed_maps) a map of 32 durations in frames:
   ph_decode_symbol reads entry duration / 8 into ph_duration. It ends where ph_append_phrase_pause
   begins. */
const uint8_t fw_cmd_speed_map_data[9][32] = {
    {   /* speed 1 ($C898) */
          1,   2,   4,   6,   8,  10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  30,   /* duration / 8 = 0-15 */
         32,  34,  36,  38,  40,  42,  44,  46,  48,  50,  52,  54,  56,  58,  60,  62,   /* duration / 8 = 16-31 */
    },
    {   /* speed 2 ($C8B8) */
          1,   1,   2,   2,   3,   4,   5,   6,   6,   7,   8,   9,  10,  10,  11,  12,   /* duration / 8 = 0-15 */
         13,  14,  14,  15,  16,  17,  18,  18,  19,  20,  21,  22,  22,  23,  24,  25,   /* duration / 8 = 16-31 */
    },
    {   /* speed 3 ($C8D8) */
          1,   1,   2,   2,   3,   3,   4,   5,   5,   6,   7,   7,   8,   8,   9,   9,   /* duration / 8 = 0-15 */
         10,  10,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  15,  16,  16,   /* duration / 8 = 16-31 */
    },
    {   /* speed 4 ($C8F8) */
          1,   1,   2,   2,   3,   3,   4,   4,   5,   5,   5,   6,   6,   7,   7,   7,   /* duration / 8 = 0-15 */
          8,   8,   8,   9,   9,   9,  10,  10,  10,  11,  11,  11,  12,  12,  12,  13,   /* duration / 8 = 16-31 */
    },
    {   /* speed 5 ($C918) */
          1,   1,   2,   2,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   /* duration / 8 = 0-15 */
          5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   /* duration / 8 = 16-31 */
    },
    {   /* speed 6 ($C938) */
          1,   1,   1,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   /* duration / 8 = 0-15 */
          4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   /* duration / 8 = 16-31 */
    },
    {   /* speed 7 ($C958) */
          1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   /* duration / 8 = 0-15 */
          3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   /* duration / 8 = 16-31 */
    },
    {   /* speed 8 ($C978) */
          1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   /* duration / 8 = 0-15 */
          2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   /* duration / 8 = 16-31 */
    },
    {   /* speed 9 ($C998) */
          1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   /* duration / 8 = 0-15 */
          1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   /* duration / 8 = 16-31 */
    },
};

/* ph_lengthen_steps ($D44A, the base ph_lengthen_count indexes with A = 0-30; annotated from $D44B):
   the number of ph_dur_lengthen steps for a half count A. Entry 0 is the RTS that ends
   ph_lengthen_count (LDA $D44A,Y / TAY / RTS): A = 0, from a lengthening count of $FF, reads the
   opcode as 96 steps. */
const uint8_t fw_ph_lengthen_steps[31] = {
    0x60,   /* A = 0: the RTS opcode at $D44A, read as data */
      1,   2,   3,   4,   4,   5,   6,   6,   7,   7,   8,   9,   9,   9,  10,   /* A = 1-15 */
     10,  10,  11,  11,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  15,   /* A = 16-30 */
};
