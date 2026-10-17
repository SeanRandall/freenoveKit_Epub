#ifndef FW_DATA_H
#define FW_DATA_H
/* Readable firmware tables converted from the ROM (M4.0 spec section 4), each pinned to its annotated ROM
   address. fw_data.c encodes them into the data image the C code reads (fw_rom8). */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../fw.h"

/* One rule (format: the annotation of tn_rules_punct). left: the context before the key, in reading order
   (the ROM stores it nearest character first); consumed: the characters after the key the rule consumes (the
   first n of its right context); right: the rest of the right context; output: phonetic text for tn_phonetic. */
typedef struct { const char *left, *consumed, *right, *output; } fw_rule;
typedef struct { const fw_rule *rules; uint16_t count; } fw_rule_list;   /* the ROM ends each list with $FF */
/* Rule lists laid out back to back from addr (in the LOW overlay when low), size bytes in all. */
typedef struct {
    const char *name;
    uint16_t addr, size;
    bool low;
    const fw_rule_list *const *lists;
    uint16_t count;
} fw_rule_table;
/* A context-symbol list (% & @): $00 CR LF, then each entry followed by CR LF, then $00. */
typedef struct { const char *name; uint16_t addr; const char *const *entries; uint8_t count; } fw_affix_list;
/* A length-prefixed phonetic text (M4.1 spec 4): the length byte strlen(text), the text, then tail, the bytes the
   ROM keeps after the counted text (a CR, or nothing). size: the annotated bytes in all. */
typedef struct { const char *name; uint16_t addr, size; const char *text, *tail; } fw_phonetic_text;

/* A command pattern record (M4.2 spec 4): the pattern (* any character, # a digit, both matched on the case-mapped
   text), its 0 end, then the handler's ROM address (cmd_addr.h), low byte first. */
typedef struct { const char *pattern; uint16_t handler; } fw_cmd_pattern;
/* A cmd_query_table entry: a setting letter and the zero-page address of the setting (fw_ram.h). */
typedef struct { char letter; uint8_t setting; } fw_cmd_query;
/* A fold of cmd_case_map: the codes first..last map to `to`, or with shift each code c to to + (c - first). */
typedef struct { uint8_t first, last, to; bool shift; } fw_case_fold;

/* A phoneme-list triple (M4.3a spec 3): the phoneme (a row of the voice tables) and its duration range, max and min,
   before the speed map. */
typedef struct { uint8_t phoneme, max, min; } fw_ph_triple;
/* The phoneme list of one symbol: count triples (at most 3), then the $FF end; word_pause: the list starts with the
   word-pause item PH_WORD_PAUSE (the space's, with no triples). */
typedef struct { uint8_t count; fw_ph_triple item[3]; bool word_pause; } fw_ph_list;

/* One parameter of a phoneme in a voice table (M4.4a spec 3), its five rows in the ROM's order: the target; the
   signed fixed contribution and the proportion 0-8 (eighths of the other phoneme's target) of the boundary value;
   the transition durations in frames in the dominated neighbour (ext_dur) and in this phoneme (int_dur), both
   when this phoneme dominates. */
typedef struct { uint8_t target; int8_t fixed; uint8_t prop, ext_dur, int_dur; } fw_par_rule;
/* One phoneme of a voice table: its rank, its spare byte and P1..P10 (p[0..9]). */
typedef struct { uint8_t rank, spare; fw_par_rule p[10]; } fw_par_phoneme;

#define CMD_SOUND_EFFECTS   17     /* @T0-@TF, and @TG or @TX */
#define CMD_SOUND_MAX       18     /* the most phonemes in an effect, and the length of the shared flag array */
#define CMD_SOUND_END       0xFF   /* ends the phoneme array of an effect */
/* The segment arrays of a @T sound effect (M4.4a spec 3): count phonemes (rows of the voice tables), then per
   phoneme its duration in frames and its pitch target. The encoder writes the end CMD_SOUND_END after the
   phonemes. */
typedef struct {
    uint8_t count;
    uint8_t phoneme[CMD_SOUND_MAX], duration[CMD_SOUND_MAX], pitch[CMD_SOUND_MAX];
} fw_cmd_sound_effect;
/* A cmd_sound_table entry: the arrays cmd_ev_sound_start points pros_phoneme_ptr, pros_duration_ptr,
   pros_pitch_ptr and pros_flags_ptr at, as references into fw_cmd_sound_effects and fw_cmd_sound_flags. */
typedef struct { const uint8_t *phoneme, *duration, *pitch, *flags; } fw_cmd_sound;

/* A code listing (M4.6c1 spec 3.5): one row per instruction at its address, its mnemonic (FWI_*) and addressing mode
   (FWM_*), the operand (an address, a zero-page address or an immediate) and, for a branch, the target address; a bit
   instruction (BBC, BBS, SEB, CLB) names its bit. The encoder (fw_data.c, step 12) assembles the rows with the
   opcodes of fw_opcodes. */
typedef enum {
    FWI_ADC, FWI_AND, FWI_BBC, FWI_BBS, FWI_BCC, FWI_BCS, FWI_BEQ, FWI_BNE, FWI_BRA, FWI_CLB, FWI_CLC, FWI_CMP,
    FWI_CPX, FWI_CPY, FWI_DEC, FWI_DEX, FWI_DEY, FWI_EOR, FWI_INC, FWI_INX, FWI_INY, FWI_JMP, FWI_JSR, FWI_LDA,
    FWI_LDX, FWI_LDY, FWI_NOP, FWI_ORA, FWI_PHA, FWI_PHP, FWI_PLA, FWI_PLP, FWI_RTS, FWI_SEB, FWI_SEI, FWI_STA,
    FWI_STX, FWI_STY, FWI_TAX, FWI_TAY, FWI_TXA, FWI_TXS, FWI_COUNT
} fw_mnem;
typedef enum {
    FWM_IMP,          /* no operand */
    FWM_ACC,          /* A (DEC A) */
    FWM_IMM,          /* #imm */
    FWM_ZP,           /* zp */
    FWM_ZPX,          /* zp,X */
    FWM_ABS,          /* abs */
    FWM_ABX,          /* abs,X */
    FWM_ABY,          /* abs,Y */
    FWM_IZY,          /* (zp),Y */
    FWM_REL,          /* a branch to target */
    FWM_BIT_ZP,       /* SEB/CLB bit,zp */
    FWM_BIT_A_REL,    /* BBC/BBS bit,A,target */
    FWM_BIT_ZP_REL,   /* BBC/BBS bit,zp,target */
} fw_mode;
typedef struct { uint16_t addr; uint8_t mnem, mode; uint16_t operand, target; uint8_t bit; } fw_insn;
/* The opcode of a mnemonic in an addressing mode; for a bit instruction, that of bit 0 (bit n adds $20 x n). */
typedef struct { uint8_t mnem, mode, opcode; } fw_opcode;

/* Pinned addresses (ghidra740/annotations: text_norm.txt, lts.txt) */
#define TN_CHAR_RULES_LO   0x4000   /* low byte of each character's list address */
#define TN_CHAR_RULES_HI   0x4100
#define LTS_PAIR_INDEX     0x4200   /* the address of lts_pairs_a..z per key letter */
#define LTS_PAIRS_A        0x4234   /* lts_pairs_a..z: 26 tables of 26 words, 52 bytes apart */
#define UNUSED_9824        0x9824   /* a $00 before lts_suffixes that no code reads */
#define LTS_SUFFIXES       0x9825
#define LTS_SIBILANTS      0x9861
#define LTS_AT_CONSONANTS  0x9884
#define LTS_CHAR_CLASS     0x9AB4

/* Pinned addresses of the command data (ghidra740/annotations: command.txt, params.txt; M4.2 spec 4) */
#define CMD_IMM_PATTERNS    0x989C   /* the immediate pattern records, then their $FF end */
#define CMD_IMM_TABLE       0x98A7   /* ptr[3]: each immediate record, then the end */
#define CMD_PATTERNS        0x98AD   /* the deferred pattern records, then their $FF end */
#define CMD_TABLE           0x994F   /* ptr[32]: the records in match order, then the end */
#define CMD_QUERY_TABLE     0x998F   /* 18 x (letter, zero-page setting), then 0 */
#define CMD_CASE_MAP        0x99B4   /* byte[256] */
#define CMD_SPEED_MAPS      0xC87B   /* ptr[10], indexed by speed - 1 */
#define CMD_SPEED_LIMITS    0xC88F   /* byte[9], indexed by speed - 1 */
#define CMD_SPEED_MAP_DATA  0xC898   /* byte[9][32], converted with the prosody data: cmd_speed_maps point into it */
#define PAR_VOICE_A_COUNT   0xD469   /* byte: the phoneme count heading the voice A table (M4.4) */
#define PAR_VOICE_A         0xD46A   /* par_table of voice A: rank row, spare row, five rows per parameter, N = 86 */
#define PAR_VOICE_B_COUNT   0xE5E2   /* byte: the same for voice B */
#define PAR_VOICE_B         0xE5E3   /* par_table of voice B */
#define PAR_PHONEMES        86       /* N, the phoneme count of both voices */

/* The phonetic texts converted in M4.1 (fw_phonetic_texts), passed by tn_rule_ptr: the version texts cmd_say_version
   says, and the baud texts boot_init_baud says with X = 1 */
#define CMD_VERSION_ENGINE_TEXT 0x9804   /* cmd_version_engine_text */
#define BOOT_BAUD_TEXT_19200    0xACC7   /* boot_baud_text_19200: silent */
#define BOOT_BAUD_TEXT_300      0xACCA   /* boot_baud_text_300 */
#define BOOT_BAUD_TEXT_1200     0xACD8   /* boot_baud_text_1200 */
#define CMD_VERSION_LANG_TEXT   0xF75B   /* cmd_version_lang_text */

/* Pinned addresses of the prosody data (ghidra740/annotations: phoneme.txt, command.txt, unused.txt; M4.3a spec 3) */
#define PH_SYMBOL_CLASS     0x9BB4   /* byte[256]: PHC_* bits per symbol code */
#define PH_SYMBOL_FEATURES  0x9CB4   /* byte[256]: PHF_* bits per symbol code */
#define PH_LIST_PTRS        0x9D74   /* base: the list pointer of symbol s ($20-$FF) at PH_LIST_PTRS + 2 * s; the
                                        slots of $00-$1F are the last 64 bytes of ph_symbol_features. The ROM reads
                                        the pointers of $80-$FF from $9E74 + 2 * (s - $80), the same addresses */
#define PH_LISTS            0xA187   /* the phoneme lists of the symbols $20-$FF, end to end in code order */
#define UNUSED_A397         0xA397   /* the $FF padding after the lists (617 bytes), not converted */
#define PH_DUR_LENGTHEN     0xC67B   /* byte[256]: one lengthening step */
#define PH_DUR_SHORTEN      0xC77B   /* byte[256]: one shortening step */
#define PH_LENGTHEN_STEPS   0xD44A   /* byte[31], the base ph_lengthen_count indexes; entry 0 is its own RTS opcode */
#define PH_WORD_PAUSE       0x5F     /* the word-pause item of a phoneme list (the space's) */

/* Pinned addresses of the playback data (ghidra740/annotations: command.txt, frame.txt, params.txt; M4.4a spec 3) */
#define CMD_SOUND_TABLE     0x9F74   /* ptr[17][4]: per @T effect its phoneme, duration, pitch-target and flag arrays */
#define CMD_SOUND_DATA      0x9FFC   /* the @T segment arrays (395 bytes) */
#define FRM_SNAPSHOT_MAP    0xB773   /* byte[10]: frame byte offset per snapshot entry */
#define PAR_SLOT_MAP        0xB797   /* byte[10]: frame byte per parameter P1..P10 (read as $B796,X, X = 1-10) */
#define PAR_TARGET_BIAS     0xB7A7   /* byte[10]: per parameter P1..P10 */

/* Pinned addresses of the frame builder's data (ghidra740/annotations: frame.txt; M4.5 spec 3) */
#define FRM_PRODUCT_LIMIT   0xB473   /* byte[256]: per F1 excess e, 255 / e (a formula the encoder expands) */
#define FRM_B14_15_LO       0xB573   /* byte[256]: per product index i, the low byte of FRM_B14_15_STEP * i */
#define FRM_B14_15_HI       0xB673   /* byte[256]: its high byte */
#define FRM_TEMPLATE        0xB783   /* byte[FRAME_SIZE]: the rest-frame template (read as $B782,Y, Y = 1-20) */
#define FRM_SOURCE_CODES    0xBAEC   /* byte[4]: b16 per frm_source_index */
#define FRM_FILTER_CODES    0xBAF0   /* byte[2]: b17 per frm_filter_index */
#define FRM_PRODUCT_LIMIT_0 72       /* frm_product_limit[0], which nothing reads (e >= 1 where it is indexed) */
#define FRM_B14_15_STEP     82       /* the b14-b15 word per product index (parent spec 1.4, rev 6) */

/* Pinned address of the serial code's data (ghidra740/annotations: serial.txt; M4.6a spec 4.6) */
#define SER_HEX_CHARS       0xC561   /* char[16]: the hex digit per nibble; ser_tx_hex's code follows at $C571 */
#define SER_HEX_CHARS_END   0xC571

/* Pinned addresses of the boot data (ghidra740/annotations: boot.txt; M4.6c2 spec 2.4) */
#define BOOT_SELFTEST_OK_CMDS     0xA7BE   /* char[7]: queued when the self-test passes ... */
#define BOOT_SELFTEST_FAIL_CMDS   0xA7C5   /* char[15]: ... and when it fails; boot_loop_play's code follows at $A7D4 */
#define BOOT_SELFTEST_CMDS_END    0xA7D4
#define BOOT_PROSODY_DEFAULTS     0xC65B   /* byte[26]: pros_params at power-up; the unconverted unused_C675 follows */
#define BOOT_PROSODY_PARAMS       26

/* Pinned address of the vector table (ghidra740/annotations: boot.txt; M5a spec 3.4) */
#define FW_VECTORS                0xFFF4   /* vec_int4_timerx: the first of six handler addresses, to $FFFF */

/* lts_char_class bits */
#define CC_LETTER       0x80   /* also the apostrophe and the underscore */
#define CC_VOWEL        0x40   /* A E I O U Y (also the underscore and lower-case x) */
#define CC_FRONT_VOWEL  0x20   /* E I Y */
#define CC_NASAL        0x10   /* M N */
#define CC_VOICED       0x08   /* B D G L M N R V W */
#define CC_CONSONANT    0x04
#define CC_HIGH         0x02   /* some codes above $7F only */
#define CC_PUNCT        0x01   /* punctuation or symbol */

/* ph_symbol_class bits (phoneme.txt) */
#define PHC_PHONETIC    0x80   /* a phonetic symbol: decoded through its phoneme list */
#define PHC_VOWEL       0x40   /* a syllable nucleus */
#define PHC_KIND_3      0x08   /* kind of punctuation, bit 3: . : ; ? */
#define PHC_KIND_2      0x04   /* kind of punctuation, bit 2: , ? (and the symbols $E0-$E2) */
#define PHC_KIND_1      0x02   /* kind of punctuation, bit 1: . : ; */
#define PHC_PUNCT       0x01   /* punctuation: a phrase pause */

/* ph_symbol_features bits (phoneme.txt) */
#define PHF_VOWEL       0x80
#define PHF_CONSONANT   0x40
#define PHF_STOP        0x20
#define PHF_FRICATIVE   0x10
#define PHF_NASAL       0x08
#define PHF_LIQUID      0x04   /* liquid or glide */
#define PHF_LONG        0x02   /* long vowel, diphthong or syllabic consonant */
#define PHF_VOICED      0x01

/* The bytes of a DSP frame (frame20: ghidra740/annotations/types.txt; parent spec 1.4): the values of
   frm_snapshot_map and par_slot_map, and the bytes the frame builder writes (M4.5) */
#define FRAME_ALF       0    /* b0 low-frequency amplitude */
#define FRAME_FN        1    /* b1 nasal (low) resonance frequency */
#define FRAME_A1        2
#define FRAME_F1        3
#define FRAME_A2        4
#define FRAME_F2        5
#define FRAME_A3        6
#define FRAME_F3        7
#define FRAME_AHF       8    /* b8 high-frequency amplitude */
#define FRAME_V         9    /* b9 degree of voicing */
#define FRAME_PITCH     10   /* b10 pitch code */
#define FRAME_MARK_SPACE 11  /* b11 mark-space ratio (@M) */
#define FRAME_B12       12   /* b12, always $17 */
#define FRAME_FLAGS     13   /* b13 transition flags */
#define FRAME_B14       14   /* b14-b15: a little-endian word (frm_b14_15_lo/hi) */
#define FRAME_B15       15
#define FRAME_SOURCE    16   /* b16 voicing source (@$) */
#define FRAME_FILTER    17   /* b17 voice filter (@$) */
#define FRAME_END_MARKER 18  /* b18-b19, always $FF $FF */
#define FRAME_SIZE      20

extern const fw_rule_table tn_rules_ctrl, tn_rules_punct, tn_rules_digits, tn_rules_punct2, lts_rules_letters,
    tn_rules_symbols3, tn_rules_lower, tn_rules_symbols4, tn_rules_high;
extern const fw_rule_table lts_rules_a, lts_rules_b, lts_rules_c, lts_rules_d, lts_rules_e, lts_rules_f,
    lts_rules_g, lts_rules_h, lts_rules_i, lts_rules_j, lts_rules_k, lts_rules_l, lts_rules_m, lts_rules_n,
    lts_rules_o, lts_rules_p, lts_rules_q, lts_rules_r, lts_rules_s, lts_rules_t, lts_rules_u, lts_rules_v,
    lts_rules_w, lts_rules_x, lts_rules_y, lts_rules_z;
extern const fw_rule_list *const lts_pairs_a[26], *const lts_pairs_b[26], *const lts_pairs_c[26],
    *const lts_pairs_d[26], *const lts_pairs_e[26], *const lts_pairs_f[26], *const lts_pairs_g[26],
    *const lts_pairs_h[26], *const lts_pairs_i[26], *const lts_pairs_j[26], *const lts_pairs_k[26],
    *const lts_pairs_l[26], *const lts_pairs_m[26], *const lts_pairs_n[26], *const lts_pairs_o[26],
    *const lts_pairs_p[26], *const lts_pairs_q[26], *const lts_pairs_r[26], *const lts_pairs_s[26],
    *const lts_pairs_t[26], *const lts_pairs_u[26], *const lts_pairs_v[26], *const lts_pairs_w[26],
    *const lts_pairs_x[26], *const lts_pairs_y[26], *const lts_pairs_z[26];

extern const fw_rule_list *const tn_char_lists[256];          /* tn_char_rules_lo/hi */
extern const fw_rule_list *const *const lts_pair_lists[26];   /* [key - 'A'][second - 'A'], NULL = none */
extern const fw_rule_table *const fw_rule_tables[];           /* every rule table, in address order per space */
extern const size_t fw_rule_table_count;
extern const fw_affix_list fw_affix_lists[];
extern const size_t fw_affix_list_count;
extern const fw_phonetic_text fw_phonetic_texts[];
extern const size_t fw_phonetic_text_count;
extern const uint8_t lts_char_class[256];
extern const fw_cmd_pattern fw_cmd_imm_patterns[], fw_cmd_patterns[];
extern const size_t fw_cmd_imm_pattern_count, fw_cmd_pattern_count;
extern const uint8_t fw_cmd_table[];   /* cmd_table in match order, as indices into fw_cmd_patterns */
extern const size_t fw_cmd_table_count;
extern const fw_cmd_query fw_cmd_query_table[];
extern const size_t fw_cmd_query_count;
extern const fw_case_fold fw_cmd_case_folds[];
extern const size_t fw_cmd_case_fold_count;
extern const uint16_t fw_cmd_speed_maps[10];
extern const uint8_t fw_cmd_speed_limits[9];
extern const uint8_t fw_par_voice_a_count, fw_par_voice_b_count;
extern const uint8_t fw_ph_symbol_class[256], fw_ph_symbol_features[256];
extern const fw_ph_list fw_ph_lists[256];   /* by symbol; $00-$1F have no list */
extern const uint8_t fw_ph_dur_lengthen[256], fw_ph_dur_shorten[256];
extern const uint8_t fw_cmd_speed_map_data[9][32];
extern const uint8_t fw_ph_lengthen_steps[31];
extern const fw_par_phoneme fw_par_voice_a[PAR_PHONEMES], fw_par_voice_b[PAR_PHONEMES];
extern const uint8_t fw_par_target_bias[10], fw_frm_snapshot_map[10];
extern const uint8_t fw_par_slot_map[10];
extern const uint8_t fw_frm_template[FRAME_SIZE], fw_frm_source_codes[4], fw_frm_filter_codes[2];
extern const char fw_ser_hex_chars[17];   /* the 16 digits, then C's terminating 0 (not encoded) */
extern const char fw_boot_selftest_ok_cmds[7], fw_boot_selftest_fail_cmds[15];   /* each with its zero end */
extern const uint8_t fw_boot_prosody_defaults[BOOT_PROSODY_PARAMS];
extern const uint16_t fw_vec_int4_timerx, fw_vec_int3_step, fw_vec_int2_timers, fw_vec_int1_serial, fw_vec_int0,
    fw_vec_reset;
extern const uint8_t fw_cmd_sound_flags[CMD_SOUND_MAX];
extern const fw_cmd_sound_effect fw_cmd_sound_effects[CMD_SOUND_EFFECTS];
extern const fw_cmd_sound fw_cmd_sound_table[CMD_SOUND_EFFECTS];
extern const uint8_t fw_bank_p2_table[8], fw_bank_p2_table_alt[8];
extern const fw_insn fw_bank_ramcode[];      /* the bank routines $0210-$0312, at their RAM addresses */
extern const size_t fw_bank_ramcode_count;
extern const fw_insn fw_bank_lts_code[];     /* the letter-to-sound code the copy takes along, $AF11-$AFFD */
extern const size_t fw_bank_lts_code_count;
extern const fw_insn fw_bank_entry_code[];   /* bank_entry_cold $FFD0-$FFD3 */
extern const size_t fw_bank_entry_code_count;
extern const uint8_t fw_bank_hdr_version[2];
extern const char fw_bank_hdr_lang_code[7], fw_bank_hdr_lang_ver_text[5], fw_bank_hdr_engine_ver_text[5];
extern const fw_opcode fw_opcodes[];         /* every mnemonic and mode the listings use */
extern const size_t fw_opcode_count;
extern const char *const fw_mnem_names[FWI_COUNT];

/* Encodes every table into img at its ROM address. False with a reason in err on any inconsistency. */
bool fw_image_build(fw_image *img, char *err, size_t errlen);
/* The image, built on first use; NULL (reason printed to stderr) if it does not build. */
const fw_image *fw_image_get(void);

#endif
