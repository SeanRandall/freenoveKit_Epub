/* Lays the converted tables out at their ROM addresses (M4.0 spec section 4.3). Every rule table must fill
   exactly its annotated size, no byte may be written twice, and every pointer table is computed from where the
   lists landed, so no address is stated twice in the data. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "fw_data.h"
#include "../bank_addr.h"

#define MAX_LISTS 640

typedef struct {
    fw_image *img;
    const fw_rule_list *list[MAX_LISTS];
    uint16_t addr[MAX_LISTS];
    size_t nlists;
    char *err;
    size_t errlen;
    bool ok;
} enc;

static void fail(enc *e, const char *fmt, ...)
{
    if (e->ok && e->errlen) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(e->err, e->errlen, fmt, ap);
        va_end(ap);
    }
    e->ok = false;
}

static void put(enc *e, bool low, uint32_t addr, uint8_t v)
{
    if (addr < 0x4000 || addr > 0xFFFF || (low && addr >= 0x8000)) {
        fail(e, "byte at $%04X is outside its space", (unsigned)addr);
        return;
    }
    uint8_t *mem = low ? e->img->low : e->img->rom, *ok = low ? e->img->low_ok : e->img->rom_ok;
    uint32_t i = low ? addr - 0x4000 : addr;
    if (ok[i >> 3] & (1u << (i & 7))) {
        fail(e, "$%04X%s written twice", (unsigned)addr, low ? " (LOW)" : "");
        return;
    }
    mem[i] = v;
    ok[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static uint32_t put_text(enc *e, bool low, uint32_t at, const char *s, bool context)
{
    for (; *s; s++) {
        if (context && (uint8_t)*s < 0x1E)
            fail(e, "context byte %02X at $%04X is below $1E", (uint8_t)*s, (unsigned)at);
        put(e, low, at++, (uint8_t)*s);
    }
    return at;
}

static uint32_t put_rule_list(enc *e, bool low, uint32_t at, const fw_rule_list *l)
{
    for (uint16_t i = 0; i < l->count; i++) {
        const fw_rule *r = &l->rules[i];
        for (size_t n = strlen(r->left); n-- > 0;) {   /* stored nearest character first */
            if ((uint8_t)r->left[n] < 0x1E)
                fail(e, "context byte below $1E at $%04X", (unsigned)at);
            put(e, low, at++, (uint8_t)r->left[n]);
        }
        size_t nc = strlen(r->consumed), no = strlen(r->output);
        if (nc >= 0x1E || no >= 0x1E)
            fail(e, "rule at $%04X: count or length >= $1E", (unsigned)at);
        put(e, low, at++, (uint8_t)nc);
        at = put_text(e, low, at, r->consumed, true);
        at = put_text(e, low, at, r->right, true);
        put(e, low, at++, (uint8_t)no);
        at = put_text(e, low, at, r->output, false);
    }
    put(e, low, at++, 0xFF);
    return at;
}

static uint16_t list_addr(enc *e, const fw_rule_list *l)
{
    for (size_t i = 0; i < e->nlists; i++)
        if (e->list[i] == l)
            return e->addr[i];
    fail(e, "a pointer table refers to a list in no rule table");
    return 0;
}

static void put_word(enc *e, uint32_t at, uint16_t v)
{
    put(e, false, at, (uint8_t)v);
    put(e, false, at + 1, (uint8_t)(v >> 8));
}

/* Command pattern records from at, then the $FF end; addr[i] = where record i landed. Returns the end's address. */
static uint32_t put_cmd_patterns(enc *e, uint32_t at, const fw_cmd_pattern *p, size_t n, uint16_t *addr)
{
    for (size_t i = 0; i < n; i++) {
        if (!p[i].pattern[0] || (uint8_t)p[i].pattern[0] == 0xFF)
            fail(e, "command pattern at $%04X is empty or starts with $FF", (unsigned)at);
        addr[i] = (uint16_t)at;
        at = put_text(e, false, at, p[i].pattern, false);
        put(e, false, at++, 0x00);
        put_word(e, at, p[i].handler);
        at += 2;
    }
    put(e, false, at, 0xFF);
    return at;
}

/* A pattern pointer table at at: the records in order (indices into addr; order NULL: each record once), then the
   end's address. Returns the address after the table. */
static uint32_t put_cmd_table(enc *e, uint32_t at, const uint16_t *addr, size_t n, const uint8_t *order,
                              size_t entries, uint32_t end)
{
    for (size_t i = 0; i < entries; i++) {
        size_t k = order ? order[i] : i;
        if (k >= n) {
            fail(e, "pointer table entry at $%04X names pattern %u of %u", (unsigned)at, (unsigned)k, (unsigned)n);
            return at;
        }
        put_word(e, at, addr[k]);
        at += 2;
    }
    put_word(e, at, (uint16_t)end);
    return at + 2;
}

static void check_end(enc *e, const char *what, uint32_t at, uint32_t next)
{
    if (at != next)
        fail(e, "%s ends at $%04X, the next table starts at $%04X", what, (unsigned)at, (unsigned)next);
}

/* A voice table (par_table) from at, in the ROM's row order: the rank row, the spare row, then per parameter
   P1..P10 its target, fixed, prop, ext_dur and int_dur rows, each PAR_PHONEMES bytes indexed by phoneme. Returns
   the address after the table. */
static uint32_t put_par_table(enc *e, uint32_t at, const fw_par_phoneme *t)
{
    for (uint32_t ph = 0; ph < PAR_PHONEMES; ph++) {
        put(e, false, at + ph, t[ph].rank);
        put(e, false, at + PAR_PHONEMES + ph, t[ph].spare);
        for (uint32_t k = 0; k < 10; k++) {
            const fw_par_rule *r = &t[ph].p[k];
            uint32_t row = at + (2 + 5 * k) * PAR_PHONEMES + ph;
            put(e, false, row, r->target);
            put(e, false, row + PAR_PHONEMES, (uint8_t)r->fixed);
            put(e, false, row + 2 * PAR_PHONEMES, r->prop);
            put(e, false, row + 3 * PAR_PHONEMES, r->ext_dur);
            put(e, false, row + 4 * PAR_PHONEMES, r->int_dur);
        }
    }
    return at + 52 * PAR_PHONEMES;
}

/* One @T array of n bytes at *at; records where it landed (arr/addr, *na entries) for cmd_sound_table. */
static void put_sound_array(enc *e, uint32_t *at, const uint8_t *v, unsigned n, const uint8_t **arr,
                            uint16_t *addr, size_t *na)
{
    arr[*na] = v;
    addr[(*na)++] = (uint16_t)*at;
    for (unsigned i = 0; i < n; i++)
        put(e, false, (*at)++, v[i]);
}

/* The opcodes of the code listings (M4.6c1 spec 3.5), from the 740 opcode table the emulator uses
   (tests/oracle/cpu740/optable.c, after MAME's dm740.lst); test_firmware_data checks each against it. */
const fw_opcode fw_opcodes[] = {
    { FWI_ADC, FWM_IMM, 0x69 }, { FWI_AND, FWM_IMM, 0x29 }, { FWI_AND, FWM_ZP, 0x25 },
    { FWI_BBC, FWM_BIT_A_REL, 0x13 }, { FWI_BBC, FWM_BIT_ZP_REL, 0x17 }, { FWI_BBS, FWM_BIT_ZP_REL, 0x07 },
    { FWI_BCC, FWM_REL, 0x90 }, { FWI_BCS, FWM_REL, 0xB0 }, { FWI_BEQ, FWM_REL, 0xF0 }, { FWI_BNE, FWM_REL, 0xD0 },
    { FWI_BRA, FWM_REL, 0x80 }, { FWI_CLB, FWM_BIT_ZP, 0x1F }, { FWI_CLC, FWM_IMP, 0x18 }, { FWI_CMP, FWM_ABS, 0xCD },
    { FWI_CMP, FWM_ABX, 0xDD }, { FWI_CMP, FWM_IMM, 0xC9 }, { FWI_CPX, FWM_IMM, 0xE0 }, { FWI_CPX, FWM_ZP, 0xE4 },
    { FWI_CPY, FWM_IMM, 0xC0 }, { FWI_DEC, FWM_ACC, 0x1A }, { FWI_DEC, FWM_ZP, 0xC6 }, { FWI_DEX, FWM_IMP, 0xCA },
    { FWI_DEY, FWM_IMP, 0x88 }, { FWI_EOR, FWM_IMM, 0x49 }, { FWI_INC, FWM_ZP, 0xE6 }, { FWI_INX, FWM_IMP, 0xE8 },
    { FWI_INY, FWM_IMP, 0xC8 }, { FWI_JMP, FWM_ABS, 0x4C }, { FWI_JSR, FWM_ABS, 0x20 }, { FWI_LDA, FWM_ABS, 0xAD },
    { FWI_LDA, FWM_ABX, 0xBD }, { FWI_LDA, FWM_ABY, 0xB9 }, { FWI_LDA, FWM_IMM, 0xA9 }, { FWI_LDA, FWM_IZY, 0xB1 },
    { FWI_LDA, FWM_ZP, 0xA5 }, { FWI_LDX, FWM_IMM, 0xA2 }, { FWI_LDX, FWM_ZP, 0xA6 }, { FWI_LDY, FWM_IMM, 0xA0 },
    { FWI_LDY, FWM_ZP, 0xA4 }, { FWI_NOP, FWM_IMP, 0xEA }, { FWI_ORA, FWM_ZP, 0x05 }, { FWI_PHA, FWM_IMP, 0x48 },
    { FWI_PHP, FWM_IMP, 0x08 }, { FWI_PLA, FWM_IMP, 0x68 }, { FWI_PLP, FWM_IMP, 0x28 }, { FWI_RTS, FWM_IMP, 0x60 },
    { FWI_SEB, FWM_BIT_ZP, 0x0F }, { FWI_SEI, FWM_IMP, 0x78 }, { FWI_STA, FWM_ABX, 0x9D }, { FWI_STA, FWM_ZP, 0x85 },
    { FWI_STA, FWM_ZPX, 0x95 }, { FWI_STX, FWM_ZP, 0x86 }, { FWI_STY, FWM_ZP, 0x84 }, { FWI_TAX, FWM_IMP, 0xAA },
    { FWI_TAY, FWM_IMP, 0xA8 }, { FWI_TXA, FWM_IMP, 0x8A }, { FWI_TXS, FWM_IMP, 0x9A },
};
const size_t fw_opcode_count = sizeof fw_opcodes / sizeof fw_opcodes[0];

const char *const fw_mnem_names[FWI_COUNT] = {
    "ADC", "AND", "BBC", "BBS", "BCC", "BCS", "BEQ", "BNE", "BRA", "CLB", "CLC", "CMP", "CPX", "CPY", "DEC", "DEX",
    "DEY", "EOR", "INC", "INX", "INY", "JMP", "JSR", "LDA", "LDX", "LDY", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP",
    "RTS", "SEB", "SEI", "STA", "STX", "STY", "TAX", "TAY", "TXA", "TXS",
};

/* The length of an instruction in mode: the opcode and its operand bytes. */
static unsigned insn_length(uint8_t mode)
{
    switch (mode) {
    case FWM_IMP: case FWM_ACC:
        return 1;
    case FWM_ABS: case FWM_ABX: case FWM_ABY: case FWM_BIT_ZP_REL:
        return 3;
    default:
        return 2;
    }
}

/* A branch's displacement from next (the address after the instruction) to target, which must fit a signed byte. */
static uint8_t branch(enc *e, const fw_insn *r, uint32_t next)
{
    int32_t d = (int32_t)r->target - (int32_t)next;
    if (d < -128 || d > 127)
        fail(e, "listing row $%04X: the branch to $%04X is out of reach", (unsigned)r->addr, (unsigned)r->target);
    return (uint8_t)d;
}

/* Assembles the n rows of a listing from at (each row's address + offset must be where the previous one ended) with
   the opcodes of fw_opcodes; returns the address after the last instruction. */
static uint32_t put_listing(enc *e, uint32_t at, const fw_insn *rows, size_t n, uint16_t offset)
{
    for (size_t i = 0; i < n; i++) {
        const fw_insn *r = &rows[i];
        if ((uint16_t)(r->addr + offset) != at)
            fail(e, "listing row $%04X lands at $%04X", (unsigned)r->addr, (unsigned)at);
        size_t k = 0;
        while (k < fw_opcode_count && (fw_opcodes[k].mnem != r->mnem || fw_opcodes[k].mode != r->mode))
            k++;
        if (k == fw_opcode_count) {
            fail(e, "listing row $%04X: no opcode for mnemonic %u in mode %u", (unsigned)r->addr, r->mnem, r->mode);
            return at;
        }
        bool bit_op = r->mode == FWM_BIT_ZP || r->mode == FWM_BIT_A_REL || r->mode == FWM_BIT_ZP_REL;
        if (r->bit > 7 || (r->bit && !bit_op))
            fail(e, "listing row $%04X: bit %u", (unsigned)r->addr, r->bit);
        unsigned len = insn_length(r->mode);
        uint32_t next = r->addr + len;   /* in the listing's own addresses, as the branch targets are */
        bool byte_operand = r->mode == FWM_IMM || r->mode == FWM_ZP || r->mode == FWM_ZPX || r->mode == FWM_IZY ||
                            r->mode == FWM_BIT_ZP || r->mode == FWM_BIT_ZP_REL;
        if (byte_operand && r->operand > 0xFF)
            fail(e, "listing row $%04X: operand $%04X is not a byte", (unsigned)r->addr, (unsigned)r->operand);
        put(e, false, at, (uint8_t)(fw_opcodes[k].opcode + 0x20 * r->bit));
        switch (r->mode) {
        case FWM_IMP: case FWM_ACC:
            break;
        case FWM_ABS: case FWM_ABX: case FWM_ABY:
            put_word(e, at + 1, r->operand);
            break;
        case FWM_REL: case FWM_BIT_A_REL:
            put(e, false, at + 1, branch(e, r, next));
            break;
        case FWM_BIT_ZP_REL:
            put(e, false, at + 1, (uint8_t)r->operand);
            put(e, false, at + 2, branch(e, r, next));
            break;
        default:
            put(e, false, at + 1, (uint8_t)r->operand);
            break;
        }
        at += len;
    }
    return at;
}

/* 12. The bank code (M4.6c1 spec 3.5): bank_ramcode_image from $ADFE, the two bank tables, the bank routines at their
   RAM addresses + BANK_RAMCODE_OFFSET, then the letter-to-sound code at its ROM addresses (to its last instruction's
   end, $AFFD); the main bank's header: bank_entry_cold assembled, bank_hdr_version, bank_hdr_lang_code and
   bank_hdr_lang_ver_text, and bank_hdr_engine_ver_text at $9800. The texts without C's terminating 0. */
static void put_bank_code(enc *e)
{
    uint32_t at = BANK_RAMCODE_IMAGE;
    for (int i = 0; i < 8; i++)
        put(e, false, at++, fw_bank_p2_table[i]);
    for (int i = 0; i < 8; i++)
        put(e, false, at++, fw_bank_p2_table_alt[i]);
    check_end(e, "bank_p2_table_alt", at, BANK_ENTER + BANK_RAMCODE_OFFSET);
    at = put_listing(e, at, fw_bank_ramcode, fw_bank_ramcode_count, BANK_RAMCODE_OFFSET);
    check_end(e, "the bank routines", at, BANK_RAMCODE_LTS + BANK_RAMCODE_OFFSET);
    at = put_listing(e, at, fw_bank_lts_code, fw_bank_lts_code_count, 0);
    /* + 2: the copy's last byte $AFFC is the opcode of LDA #1, whose operand $AFFD lies one byte past the copy */
    check_end(e, "the letter-to-sound code", at, BANK_RAMCODE_IMAGE_END + 2);
    at = put_listing(e, BANK_ENTRY_COLD, fw_bank_entry_code, fw_bank_entry_code_count, 0);
    check_end(e, "bank_entry_cold", at, BANK_ENTRY_WARM);
    put(e, false, BANK_HDR_VERSION, fw_bank_hdr_version[0]);
    put(e, false, BANK_HDR_VERSION + 1, fw_bank_hdr_version[1]);
    check_end(e, "bank_hdr_lang_code", put_text(e, false, BANK_HDR_LANG_CODE, fw_bank_hdr_lang_code, false),
              BANK_HDR_LANG_CODE + BANK_LANG_CODE_CHARS);
    check_end(e, "bank_hdr_lang_ver_text", put_text(e, false, BANK_HDR_LANG_VER_TEXT, fw_bank_hdr_lang_ver_text, false),
              BANK_HDR_LANG_VER_TEXT + BANK_VER_TEXT_CHARS);
    check_end(e, "bank_hdr_engine_ver_text",
              put_text(e, false, BANK_HDR_ENGINE_VER_TEXT, fw_bank_hdr_engine_ver_text, false), CMD_VERSION_ENGINE_TEXT);
}

bool fw_image_build(fw_image *img, char *err, size_t errlen)
{
    static enc e;
    memset(img, 0, sizeof *img);
    memset(&e, 0, sizeof e);
    e.img = img;
    e.err = err;
    e.errlen = errlen;
    e.ok = true;

    /* 1. The rule tables, each at its annotated address and exactly its annotated size. */
    for (size_t t = 0; t < fw_rule_table_count; t++) {
        const fw_rule_table *tab = fw_rule_tables[t];
        uint32_t at = tab->addr;
        for (uint16_t i = 0; i < tab->count; i++) {
            if (e.nlists == MAX_LISTS) {
                fail(&e, "more than %d rule lists", MAX_LISTS);
                return false;
            }
            e.list[e.nlists] = tab->lists[i];
            e.addr[e.nlists++] = (uint16_t)at;
            at = put_rule_list(&e, tab->low, at, tab->lists[i]);
        }
        if (at - tab->addr != tab->size)
            fail(&e, "%s: %u bytes, the annotation says %u", tab->name, (unsigned)(at - tab->addr), tab->size);
    }
    /* 2. tn_char_rules_lo/hi: each character's list. */
    for (int c = 0; c < 256; c++) {
        uint16_t a = list_addr(&e, tn_char_lists[c]);
        put(&e, false, TN_CHAR_RULES_LO + (uint32_t)c, (uint8_t)a);
        put(&e, false, TN_CHAR_RULES_HI + (uint32_t)c, (uint8_t)(a >> 8));
    }
    /* 3. lts_pair_index and lts_pairs_a..z ($0000 = no list for the pair). */
    for (int k = 0; k < 26; k++) {
        uint32_t tab = LTS_PAIRS_A + 52u * (uint32_t)k;
        put_word(&e, LTS_PAIR_INDEX + 2u * (uint32_t)k, (uint16_t)tab);
        for (int s = 0; s < 26; s++) {
            const fw_rule_list *l = lts_pair_lists[k][s];
            put_word(&e, tab + 2u * (uint32_t)s, l ? list_addr(&e, l) : 0);
        }
    }
    /* 4. The affix lists of the context symbols % & @, and the stray $00 before them. */
    put(&e, false, UNUSED_9824, 0x00);
    for (size_t i = 0; i < fw_affix_list_count; i++) {
        const fw_affix_list *a = &fw_affix_lists[i];
        uint32_t at = a->addr;
        put(&e, false, at++, 0x00);
        put(&e, false, at++, 0x0D);
        put(&e, false, at++, 0x0A);
        for (uint8_t n = 0; n < a->count; n++) {
            at = put_text(&e, false, at, a->entries[n], false);
            put(&e, false, at++, 0x0D);
            put(&e, false, at++, 0x0A);
        }
        put(&e, false, at, 0x00);
    }
    /* 5. lts_char_class. */
    for (int c = 0; c < 256; c++)
        put(&e, false, LTS_CHAR_CLASS + (uint32_t)c, lts_char_class[c]);
    /* 6. The length-prefixed phonetic texts (M4.1 spec 4). */
    for (size_t i = 0; i < fw_phonetic_text_count; i++) {
        const fw_phonetic_text *t = &fw_phonetic_texts[i];
        size_t n = strlen(t->text);
        if (n == 0 || n > 0xFF)
            fail(&e, "%s: text length %u", t->name, (unsigned)n);
        uint32_t at = t->addr;
        put(&e, false, at++, (uint8_t)n);
        at = put_text(&e, false, at, t->text, false);
        at = put_text(&e, false, at, t->tail, false);
        if (at - t->addr != t->size)
            fail(&e, "%s: %u bytes, the annotation says %u", t->name, (unsigned)(at - t->addr), t->size);
    }
    /* 7. The command data (M4.2 spec 4): the pattern records with each table's $FF end and pointer table, the
       query table, cmd_case_map, the speed tables and the two voice counts. Each block ends where the next
       begins. */
    {
        uint16_t pat[64];
        if (fw_cmd_imm_pattern_count > 64 || fw_cmd_pattern_count > 64) {
            fail(&e, "more than 64 command patterns");
            return false;
        }
        uint32_t end = put_cmd_patterns(&e, CMD_IMM_PATTERNS, fw_cmd_imm_patterns, fw_cmd_imm_pattern_count, pat);
        check_end(&e, "cmd_imm_patterns", end + 1, CMD_IMM_TABLE);
        uint32_t at = put_cmd_table(&e, CMD_IMM_TABLE, pat, fw_cmd_imm_pattern_count, NULL,
                                    fw_cmd_imm_pattern_count, end);
        check_end(&e, "cmd_imm_table", at, CMD_PATTERNS);
        end = put_cmd_patterns(&e, CMD_PATTERNS, fw_cmd_patterns, fw_cmd_pattern_count, pat);
        check_end(&e, "fw_cmd_patterns", end + 1, CMD_TABLE);
        at = put_cmd_table(&e, CMD_TABLE, pat, fw_cmd_pattern_count, fw_cmd_table, fw_cmd_table_count, end);
        check_end(&e, "cmd_table", at, CMD_QUERY_TABLE);
        at = CMD_QUERY_TABLE;
        for (size_t i = 0; i < fw_cmd_query_count; i++) {
            if (!fw_cmd_query_table[i].letter)
                fail(&e, "cmd_query_table entry %u has letter 0", (unsigned)i);
            put(&e, false, at++, (uint8_t)fw_cmd_query_table[i].letter);
            put(&e, false, at++, fw_cmd_query_table[i].setting);
        }
        put(&e, false, at++, 0x00);
        check_end(&e, "cmd_query_table", at, CMD_CASE_MAP);
        uint8_t map[256];
        bool folded[256] = { false };
        for (int c = 0; c < 256; c++)
            map[c] = (uint8_t)c;
        for (size_t i = 0; i < fw_cmd_case_fold_count; i++) {
            const fw_case_fold *f = &fw_cmd_case_folds[i];
            if (f->first > f->last)
                fail(&e, "case fold %u: first above last", (unsigned)i);
            for (int c = f->first; c <= f->last; c++) {
                int to = f->shift ? f->to + (c - f->first) : f->to;
                if (to > 0xFF || to == c || folded[c])
                    fail(&e, "case fold %u: $%02X folded twice, to itself or past $FF", (unsigned)i, (unsigned)c);
                folded[c] = true;
                map[c] = (uint8_t)to;
            }
        }
        for (int c = 0; c < 256; c++)
            put(&e, false, CMD_CASE_MAP + (uint32_t)c, map[c]);
        for (int i = 0; i < 10; i++)
            put_word(&e, CMD_SPEED_MAPS + 2u * (uint32_t)i, fw_cmd_speed_maps[i]);
        for (int i = 0; i < 9; i++)
            put(&e, false, CMD_SPEED_LIMITS + (uint32_t)i, fw_cmd_speed_limits[i]);
        put(&e, false, PAR_VOICE_A_COUNT, fw_par_voice_a_count);
        put(&e, false, PAR_VOICE_B_COUNT, fw_par_voice_b_count);
    }
    /* 8. The prosody data (M4.3a spec 3): the symbol classes and features, the phoneme lists end to end with the
       list pointers computed from where they land, the duration steps, the speed maps and ph_lengthen_steps. */
    {
        for (int s = 0; s < 256; s++) {
            put(&e, false, PH_SYMBOL_CLASS + (uint32_t)s, fw_ph_symbol_class[s]);
            put(&e, false, PH_SYMBOL_FEATURES + (uint32_t)s, fw_ph_symbol_features[s]);
        }
        /* The pointer slots of the symbols $00-$1F are the features of $C0-$FF, written just above: those symbols
           must have no class and no list, so that the decoder never reads them as pointers. */
        check_end(&e, "ph_symbol_features", PH_SYMBOL_FEATURES + 256, PH_LIST_PTRS + 2 * 0x20);
        for (int s = 0; s < 0x20; s++)
            if (fw_ph_symbol_class[s] || fw_ph_lists[s].count || fw_ph_lists[s].word_pause)
                fail(&e, "symbol $%02X has a class or a list, but its pointer slot is a feature byte", (unsigned)s);
        uint32_t at = PH_LISTS;
        for (int s = 0x20; s < 256; s++) {
            const fw_ph_list *l = &fw_ph_lists[s];
            put_word(&e, PH_LIST_PTRS + 2u * (uint32_t)s, (uint16_t)at);
            if (l->count > 3 || (l->word_pause && l->count))
                fail(&e, "phoneme list of $%02X: %u triples%s", (unsigned)s, (unsigned)l->count,
                     l->word_pause ? " after the word-pause item" : "");
            if (l->word_pause)
                put(&e, false, at++, PH_WORD_PAUSE);
            for (unsigned i = 0; i < l->count && i < 3; i++) {
                const fw_ph_triple *t = &l->item[i];
                if (t->phoneme == 0xFF || t->phoneme == PH_WORD_PAUSE || t->max < t->min)
                    fail(&e, "phoneme list of $%02X, triple %u: phoneme $%02X, max %u, min %u", (unsigned)s, i,
                         (unsigned)t->phoneme, (unsigned)t->max, (unsigned)t->min);
                put(&e, false, at++, t->phoneme);
                put(&e, false, at++, t->max);
                put(&e, false, at++, t->min);
            }
            put(&e, false, at++, 0xFF);
        }
        check_end(&e, "the phoneme lists", at, UNUSED_A397);
        for (int x = 0; x < 256; x++) {
            put(&e, false, PH_DUR_LENGTHEN + (uint32_t)x, fw_ph_dur_lengthen[x]);
            put(&e, false, PH_DUR_SHORTEN + (uint32_t)x, fw_ph_dur_shorten[x]);
        }
        for (int k = 0; k < 9; k++)
            for (int d = 0; d < 32; d++)
                put(&e, false, CMD_SPEED_MAP_DATA + 32u * (uint32_t)k + (uint32_t)d, fw_cmd_speed_map_data[k][d]);
        for (int i = 0; i < 10; i++) {   /* M4.2's cmd_speed_maps must point at the start of a map */
            uint32_t p = fw_cmd_speed_maps[i];
            if (p < CMD_SPEED_MAP_DATA || (p - CMD_SPEED_MAP_DATA) % 32 || p - CMD_SPEED_MAP_DATA >= 9 * 32)
                fail(&e, "cmd_speed_maps entry %d ($%04X) is not the start of a speed map", i, (unsigned)p);
        }
        for (int i = 0; i < 31; i++)
            put(&e, false, PH_LENGTHEN_STEPS + (uint32_t)i, fw_ph_lengthen_steps[i]);
        check_end(&e, "ph_lengthen_steps", PH_LENGTHEN_STEPS + 31, PAR_VOICE_A_COUNT);
    }
    /* 9. The playback data (M4.4a spec 3): the two voice tables, each after its count byte (step 7) and filling the
       space up to the next table; par_target_bias, frm_snapshot_map and (M4.4b spec 3) par_slot_map; the @T sound
       effects end to end in cmd_sound_data, exactly filling it, and cmd_sound_table computed from where their
       arrays landed. */
    {
        if (fw_par_voice_a_count != PAR_PHONEMES || fw_par_voice_b_count != PAR_PHONEMES)
            fail(&e, "the voice counts are %u and %u, the tables have %u phonemes", (unsigned)fw_par_voice_a_count,
                 (unsigned)fw_par_voice_b_count, (unsigned)PAR_PHONEMES);
        check_end(&e, "par_voice_a_count", PAR_VOICE_A_COUNT + 1, PAR_VOICE_A);
        check_end(&e, "par_voice_a", put_par_table(&e, PAR_VOICE_A, fw_par_voice_a), PAR_VOICE_B_COUNT);
        check_end(&e, "par_voice_b_count", PAR_VOICE_B_COUNT + 1, PAR_VOICE_B);
        check_end(&e, "par_voice_b", put_par_table(&e, PAR_VOICE_B, fw_par_voice_b), CMD_VERSION_LANG_TEXT);
        for (int k = 0; k < 10; k++) {
            put(&e, false, PAR_TARGET_BIAS + (uint32_t)k, fw_par_target_bias[k]);
            if (fw_frm_snapshot_map[k] > FRAME_PITCH)
                fail(&e, "frm_snapshot_map entry %d: frame byte %u", k + 1, (unsigned)fw_frm_snapshot_map[k]);
            put(&e, false, FRM_SNAPSHOT_MAP + (uint32_t)k, fw_frm_snapshot_map[k]);
            if (fw_par_slot_map[k] > FRAME_V)   /* b10, the pitch, is the pitch track's */
                fail(&e, "par_slot_map entry %d: frame byte %u", k + 1, (unsigned)fw_par_slot_map[k]);
            put(&e, false, PAR_SLOT_MAP + (uint32_t)k, fw_par_slot_map[k]);
        }
        const uint8_t *arr[1 + 3 * CMD_SOUND_EFFECTS];
        uint16_t addr[1 + 3 * CMD_SOUND_EFFECTS];
        size_t na = 0;
        uint32_t at = CMD_SOUND_DATA;
        put_sound_array(&e, &at, fw_cmd_sound_flags, CMD_SOUND_MAX, arr, addr, &na);
        for (int x = 0; x < CMD_SOUND_EFFECTS; x++) {
            const fw_cmd_sound_effect *s = &fw_cmd_sound_effects[x];
            unsigned n = s->count;
            if (n > CMD_SOUND_MAX) {
                fail(&e, "@T effect %d: %u phonemes, at most %d", x, n, CMD_SOUND_MAX);
                n = CMD_SOUND_MAX;
            }
            for (unsigned i = 0; i < n; i++)
                if (s->phoneme[i] == CMD_SOUND_END)
                    fail(&e, "@T effect %d: phoneme %u is the end $%02X", x, i, CMD_SOUND_END);
            put_sound_array(&e, &at, s->phoneme, n, arr, addr, &na);
            put(&e, false, at++, CMD_SOUND_END);
            put_sound_array(&e, &at, s->duration, n, arr, addr, &na);
            put_sound_array(&e, &at, s->pitch, n, arr, addr, &na);
        }
        check_end(&e, "cmd_sound_data", at, PH_LISTS);
        for (int x = 0; x < CMD_SOUND_EFFECTS; x++) {
            const fw_cmd_sound *t = &fw_cmd_sound_table[x];
            const uint8_t *ref[4] = { t->phoneme, t->duration, t->pitch, t->flags };
            for (int k = 0; k < 4; k++) {
                size_t i = 0;
                while (i < na && arr[i] != ref[k])
                    i++;
                if (i == na)
                    fail(&e, "cmd_sound_table entry %d, pointer %d: not one of the @T arrays", x, k);
                else
                    put_word(&e, CMD_SOUND_TABLE + 8u * (uint32_t)x + 2u * (uint32_t)k, addr[i]);
            }
        }
        check_end(&e, "cmd_sound_table", CMD_SOUND_TABLE + 8 * CMD_SOUND_EFFECTS, CMD_SOUND_DATA);
    }
    /* 10. The frame builder's data (M4.5 spec 3): two tables expanded from their formulas, frm_product_limit[e] =
       255 / e (truncated), the largest level difference d whose product d * e fits a byte (entry 0, which nothing
       reads, is FRM_PRODUCT_LIMIT_0), and frm_b14_15_lo/hi[i] = the low and the high byte of the b14-b15 word
       FRM_B14_15_STEP * i, ending where frm_snapshot_map starts; frm_template, ending where par_slot_map starts;
       frm_source_codes and frm_filter_codes, back to back. */
    {
        for (uint32_t i = 0; i < 256; i++) {
            put(&e, false, FRM_PRODUCT_LIMIT + i, (uint8_t)(i ? 255 / i : FRM_PRODUCT_LIMIT_0));
            uint16_t w = (uint16_t)(FRM_B14_15_STEP * i);
            put(&e, false, FRM_B14_15_LO + i, (uint8_t)w);
            put(&e, false, FRM_B14_15_HI + i, (uint8_t)(w >> 8));
        }
        check_end(&e, "frm_b14_15_hi", FRM_B14_15_HI + 256, FRM_SNAPSHOT_MAP);
        for (uint32_t b = 0; b < FRAME_SIZE; b++)
            put(&e, false, FRM_TEMPLATE + b, fw_frm_template[b]);
        check_end(&e, "frm_template", FRM_TEMPLATE + FRAME_SIZE, PAR_SLOT_MAP);
        for (uint32_t k = 0; k < 4; k++)
            put(&e, false, FRM_SOURCE_CODES + k, fw_frm_source_codes[k]);
        check_end(&e, "frm_source_codes", FRM_SOURCE_CODES + 4, FRM_FILTER_CODES);
        for (uint32_t k = 0; k < 2; k++)
            put(&e, false, FRM_FILTER_CODES + k, fw_frm_filter_codes[k]);
    }
    /* 11. The serial code's data (M4.6a spec 4.6): ser_hex_chars, the 16 digits without C's terminating 0, ending
       where ser_tx_hex's code starts. */
    {
        uint32_t at = SER_HEX_CHARS;
        for (const char *s = fw_ser_hex_chars; *s; s++)
            put(&e, false, at++, (uint8_t)*s);
        check_end(&e, "ser_hex_chars", at, SER_HEX_CHARS_END);
    }
    put_bank_code(&e);
    /* 13. The boot data (M4.6c2 spec 2.4): the self-test's two command strings back to back, each with its zero end,
       ending where boot_loop_play's code starts; boot_prosody_defaults. */
    {
        uint32_t at = put_text(&e, false, BOOT_SELFTEST_OK_CMDS, fw_boot_selftest_ok_cmds, false);
        put(&e, false, at++, 0);
        check_end(&e, "boot_selftest_ok_cmds", at, BOOT_SELFTEST_FAIL_CMDS);
        at = put_text(&e, false, at, fw_boot_selftest_fail_cmds, false);
        put(&e, false, at++, 0);
        check_end(&e, "boot_selftest_fail_cmds", at, BOOT_SELFTEST_CMDS_END);
        for (uint32_t i = 0; i < BOOT_PROSODY_PARAMS; i++)
            put(&e, false, BOOT_PROSODY_DEFAULTS + i, fw_boot_prosody_defaults[i]);
    }
    /* 14. The vector table (M5a spec 3.4): the six handler addresses, each low byte first, from FW_VECTORS to the top
       of the address space. */
    {
        const uint16_t vec[] = { fw_vec_int4_timerx, fw_vec_int3_step, fw_vec_int2_timers, fw_vec_int1_serial,
                                 fw_vec_int0, fw_vec_reset };
        uint32_t at = FW_VECTORS;
        for (size_t k = 0; k < sizeof vec / sizeof vec[0]; k++, at += 2)
            put_word(&e, at, vec[k]);
        check_end(&e, "the vector table", at, 0x10000);
    }
    return e.ok;
}

const fw_image *fw_image_get(void)
{
    static fw_image img;
    static int state;   /* 0 = not built, 1 = built, -1 = failed */
    if (state == 0) {
        char err[160] = "";
        state = fw_image_build(&img, err, sizeof err) ? 1 : -1;
        if (state < 0)
            fprintf(stderr, "firmware data image: %s\n", err);
    }
    return state > 0 ? &img : NULL;
}
