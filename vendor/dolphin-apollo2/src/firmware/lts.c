#include "lts.h"
#include "tn_rules.h"
#include "data/fw_data.h"

static uint8_t text_at(const fw *f, uint8_t x) { return fw_rd8(f, (uint16_t)(CMD_TEXT + x)); }
static uint8_t char_class(fw *f, uint8_t c) { return fw_rom8(f, (uint16_t)(LTS_CHAR_CLASS + c)); }
static uint8_t affix_byte(fw *f, uint8_t y) { return fw_ind_y(f, (uint8_t)LTS_AFFIX_PTR, y); }
static uint8_t clause_len(const fw *f) { return fw_rd8(f, BOOT_CLAUSE_LEN); }

/* INC or DEC lts_match_pos by lts_match_dir bit 0 (1 = forwards, for the right context) */
static void step_match_pos(fw *f)
{
    uint8_t p = fw_rd8(f, LTS_MATCH_POS);
    fw_wr8(f, LTS_MATCH_POS, (uint8_t)((fw_rd8(f, LTS_MATCH_DIR) & 1) ? p + 1 : p - 1));
}

uint8_t lts_affix_next(fw *f, uint8_t *y)
{
    uint8_t a;
    do {
        (*y)++;
        a = affix_byte(f, *y);
        if (a == 0)
            return 0;
    } while (a != 0x0A);
    (*y)++;
    return a;
}

uint8_t lts_affix_prev(fw *f, uint8_t *y)
{
    uint8_t a;
    do {
        (*y)--;
        a = affix_byte(f, *y);
        if (a == 0)
            return 0;
    } while (a != 0x0D);
    (*y)--;
    return a;
}

/* the class of the text character at lts_match_pos */
static uint8_t match_class(fw *f) { return char_class(f, text_at(f, fw_rd8(f, LTS_MATCH_POS))); }

/* . ! + and lts_sym_vowel: one character whose class has the bit; A = the masked class (0 = no) */
static uint8_t sym_class(fw *f, uint8_t bit)
{
    uint8_t a = match_class(f) & bit;
    step_match_pos(f);
    return a;
}

uint8_t lts_sym_vowel(fw *f) { return sym_class(f, CC_VOWEL); }

/* space: one character that is not a letter */
static uint8_t sym_nonletter(fw *f)
{
    uint8_t a = (match_class(f) & CC_LETTER) ? 0 : 1;
    step_match_pos(f);
    return a;
}

/* #: one vowel, then any further vowels inside the clause */
static uint8_t sym_vowels(fw *f)
{
    if (lts_sym_vowel(f) == 0)
        return 0;
    for (;;) {
        uint8_t x = fw_rd8(f, LTS_MATCH_POS);
        if (x >= clause_len(f) || !(char_class(f, text_at(f, x)) & CC_VOWEL))
            return 0xFF;
        step_match_pos(f);
    }
}

/* :  zero or more consonants inside the clause */
static uint8_t sym_consonants(fw *f)
{
    for (;;) {
        uint8_t x = fw_rd8(f, LTS_MATCH_POS);
        if (x >= clause_len(f) || !(char_class(f, text_at(f, x)) & CC_CONSONANT))
            return 0xFF;
        step_match_pos(f);
    }
}

/* any other byte: it must equal the text (the ROM reads it again from the rule) */
static uint8_t sym_literal(fw *f, uint8_t y)
{
    uint8_t a = tn_rule_byte(f, y);
    if (a != text_at(f, fw_rd8(f, LTS_MATCH_POS)))
        a = 0;
    step_match_pos(f);
    return a;
}

/* One suffix entry at *y against the text from *x: 1 = matched, 0 = the whole symbol fails (the list end, or
   a space entry before a letter), -1 = this entry does not match. */
static int suffix_entry(fw *f, uint8_t *y, uint8_t *x)
{
    for (;;) {
        uint8_t a = affix_byte(f, *y);
        if (a == 0)
            return 0;
        if (a == 0x0D)
            return 1;
        if (a == ' ')   /* needs a non-letter, which is tested but not consumed */
            return (char_class(f, text_at(f, *x)) & CC_LETTER) ? 0 : 1;
        if (a != text_at(f, *x))
            return -1;
        (*x)++;
        if (*x >= clause_len(f))
            return -1;
        (*y)++;
    }
}

/* %: a suffix of lts_suffixes forwards from lts_match_pos */
static uint8_t sym_suffix(fw *f, uint8_t y_saved)
{
    uint8_t y = 3;   /* the first entry: past the list's $00 and CR LF */
    fw_wr16(f, LTS_AFFIX_PTR, LTS_SUFFIXES);
    fw_wr8(f, FW_TMP_78, y_saved);
    for (;;) {
        uint8_t x = fw_rd8(f, LTS_MATCH_POS);
        int r = suffix_entry(f, &y, &x);
        if (r == 0)
            return 0;
        if (r == 1) {
            fw_wr8(f, LTS_MATCH_POS, x);
            return 1;
        }
        lts_affix_next(f, &y);
    }
}

/* & and @: the entries of a list matched backwards from y (the last character of its last entry) */
static uint8_t affix_match_back(fw *f, uint16_t list, uint8_t y, uint8_t y_saved)
{
    fw_wr16(f, LTS_AFFIX_PTR, list);
    fw_wr8(f, FW_TMP_78, y_saved);
    for (;;) {
        uint8_t x = fw_rd8(f, LTS_MATCH_POS);
        for (;;) {
            uint8_t a = affix_byte(f, y);
            if (a == 0)
                return 0;              /* the list start: nothing matched */
            if (a == 0x0A) {           /* an entry matched completely */
                fw_wr8(f, LTS_MATCH_POS, x);
                return a;
            }
            if (a != text_at(f, x))
                break;
            x--;
            y--;
        }
        lts_affix_prev(f, &y);
    }
}

uint8_t lts_match_symbol(fw *f, uint8_t sym, uint8_t y)
{
    uint8_t pos = fw_rd8(f, LTS_MATCH_POS);
    if (pos >= clause_len(f))
        return 0;
    if (char_class(f, sym) & CC_LETTER) {   /* a letter must equal the text */
        if (sym != text_at(f, pos))
            return 0;
        step_match_pos(f);
        return sym;
    }
    switch (sym) {
    case ' ': return sym_nonletter(f);
    case '#': return sym_vowels(f);
    case '.': return sym_class(f, CC_VOICED);
    case '%': return sym_suffix(f, y);
    case '&': return affix_match_back(f, LTS_SIBILANTS, 0x1F, y);       /* from the H of SH */
    case '@': return affix_match_back(f, LTS_AT_CONSONANTS, 0x14, y);   /* from the H of SH */
    case '!': return sym_class(f, CC_CONSONANT);
    case '+': return sym_class(f, CC_FRONT_VOWEL);
    case ':': return sym_consonants(f);
    default: return sym_literal(f, y);
    }
}

bool lts_match_list(fw *f)
{
    for (;;) {
        uint8_t y = 0, a;
        bool ok = true;
        fw_wr8(f, LTS_MATCH_POS, (uint8_t)(fw_rd8(f, TN_KEY_POS) - 1));
        fw_wr8(f, LTS_MATCH_DIR, 0);
        while ((a = tn_rule_byte(f, y)) >= 0x1E) {   /* the left context, backwards */
            if (lts_match_symbol(f, a, y) == 0) {
                ok = false;
                break;
            }
            y++;
        }
        if (!ok) {
            a = tn_rule_skip_text(f, &y);         /* the rest of the left context */
            tn_rule_skip_field(f, a, &y);         /* the count and the consumed characters */
        } else {
            uint8_t pos = (uint8_t)(fw_rd8(f, TN_KEY_POS) + 1);
            fw_wr8(f, LTS_MATCH_DIR, 1);
            if (fw_rd8(f, LTS_KEY_NEXT))
                pos++;                            /* a two-letter key: the right context starts after both */
            fw_wr8(f, LTS_MATCH_POS, pos);
            a = tn_rule_byte(f, y++);             /* the count n */
            fw_wr8(f, TN_POS, (uint8_t)(a + pos));   /* set now: it stays even when the right context fails */
            while ((a = tn_rule_byte(f, y)) >= 0x1E) {  /* the right context, forwards */
                if (lts_match_symbol(f, a, y) == 0) {
                    ok = false;
                    break;
                }
                y++;
            }
            if (ok) {
                uint8_t x = 0;                    /* not an output of lts_match_list */
                tn_append_phonetic(f, a, &x, &y); /* a = the output length (the ROM jumps there) */
                return true;
            }
        }
        a = tn_rule_skip_text(f, &y);             /* the rest of the right context */
        tn_rule_skip_field(f, a, &y);             /* the length and the output */
        if (tn_rule_byte(f, y) == 0xFF)
            return false;
    }
}

/* the two-letter list of c and the next character; false when there is none or no rule matched */
static bool two_letter(fw *f, uint8_t c)
{
    uint8_t y = (uint8_t)((c - 'A') * 2);
    fw_wr8(f, LTS_PAIR_TABLE, fw_rom8(f, (uint16_t)(LTS_PAIR_INDEX + y)));
    fw_wr8(f, LTS_PAIR_TABLE + 1, fw_rom8(f, (uint16_t)(LTS_PAIR_INDEX + y + 1)));
    uint8_t next = fw_rd8(f, (uint16_t)(CMD_TEXT + 1 + fw_rd8(f, TN_KEY_POS)));
    fw_wr8(f, LTS_KEY_NEXT, next);
    if (next < 'A' || next > 'Z')
        return false;
    y = (uint8_t)((next - 'A') * 2);
    fw_wr8(f, TN_RULE_PTR, fw_ind_y(f, (uint8_t)LTS_PAIR_TABLE, y));
    uint8_t hi = fw_ind_y(f, (uint8_t)LTS_PAIR_TABLE, (uint8_t)(y + 1));
    if (hi == 0)
        return false;   /* no list for this pair; the low byte is already stored, as in the ROM */
    fw_wr8(f, TN_RULE_PTR + 1, hi);
    if (fw_rd8(f, LTS_LOW_BANK) & 1)
        fw_set_low_window(f, true);   /* SEB7 P2: the K-Z lists are in the LOW overlay */
    return lts_match_list(f);
}

void lts_convert_char(fw *f, uint8_t c)
{
    fw_wr8(f, LTS_LOW_BANK, (uint8_t)(fw_rd8(f, LTS_LOW_BANK) & ~1));
    if ((fw_rd8(f, CMD_SPELL) & 1) && fw_rd8(f, TN_KEY_POS) != 0) {   /* spell mode: the character's name */
        tn_say_char(f, c);
        return;
    }
    fw_wr8(f, LTS_KEY_CHAR, c);
    if (c >= 'A') {
        if (c >= 'K')   /* also the codes after Z, which go on to their one-character list */
            fw_wr8(f, LTS_LOW_BANK, (uint8_t)(fw_rd8(f, LTS_LOW_BANK) | 1));
        if (c <= 'Z' && two_letter(f, c))
            return;
    }
    fw_set_low_window(f, false);      /* CLB7 P2 */
    uint8_t k = text_at(f, fw_rd8(f, TN_KEY_POS));
    fw_wr8(f, LTS_KEY_CHAR, k);
    fw_wr8(f, LTS_KEY_NEXT, 0);
    fw_wr8(f, TN_RULE_PTR, fw_rom8(f, (uint16_t)(TN_CHAR_RULES_LO + k)));
    fw_wr8(f, TN_RULE_PTR + 1, fw_rom8(f, (uint16_t)(TN_CHAR_RULES_HI + k)));
    lts_match_list(f);
}
