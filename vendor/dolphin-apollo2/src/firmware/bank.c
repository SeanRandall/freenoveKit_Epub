#include "bank.h"
#include "bank_addr.h"
#include "cmd_handlers.h"
#include "cmd_match.h"

void bank_load_ramcode(fw *f, fw_regs *r)
{
    fw_wr8(f, FW_TMP_78, r->y);                                    /* STY $78 */
    fw_wr8(f, FW_TMP_76, r->x);                                    /* STX $76 */
    fw_wr8(f, FW_TMP_75, fw_sfr8(f, SFR_P2));                      /* LDX P2 / STX $75 */
    uint8_t x = 0;
    do {                                                           /* LDA $ADFE,X / STA $0200,X / LDA $AEFD,X /
                                                                      STA $02FF,X / INX / BNE */
        fw_wr8(f, (uint16_t)(BANK_RAMCODE + x), fw_rom8(f, (uint16_t)(BANK_RAMCODE_IMAGE + x)));
        fw_wr8(f, (uint16_t)(BANK_RAMCODE + 0xFF + x), fw_rom8(f, (uint16_t)(BANK_RAMCODE_IMAGE + 0xFF + x)));
    } while (++x != 0);
    r->x = 0;
}

/* $0252 bank_restore, bank_scan's tail and bank_enter's exit for an absent bank: LDA $75 / STA P2, LDX $76, LDY $78,
   PLP, PLA (A as it came) */
static void bank_restore(fw *f, fw_regs *r)
{
    fw_wr_sfr8(f, SFR_P2, fw_rd8(f, FW_TMP_75));
    r->x = fw_rd8(f, FW_TMP_76);
    r->y = fw_rd8(f, FW_TMP_78);
}

void bank_enter(fw *f, fw_regs *r)
{
    fw_regs t = *r;                                                /* PHA / PHP / SEI */
    bank_select(f, &t);
    bank_probe(f, &t);
    if (fw_rd8(f, BANK_ABSENT) & 1) {                              /* BBC0 bank_absent not taken: JMP bank_restore */
        bank_restore(f, r);
        return;
    }
    uint8_t v = fw_rom8(f, BANK_HDR_VERSION + 1);                  /* LDX #$FF / TXS; LDA $FFE1 / CMP $181B */
    if (v == fw_rd8(f, BANK_VER_MINOR)) {
        v = fw_rom8(f, BANK_HDR_VERSION);                          /* LDA $FFE0 / CMP $1819 */
        if (v == fw_rd8(f, BANK_VER_MAJOR)) {
            fw_enter(f, BANK_ENTRY_WARM, v, BANK_STACK_TOP);       /* JMP $FFD4 */
            return;
        }
    }
    for (uint8_t x = BANK_ZP_CLEAR_LAST; x != 0; x--)               /* LDA #0 / LDX #$64; STA $00,X / DEX / BNE */
        fw_wr8(f, x, 0);
    fw_enter(f, BANK_ENTRY_COLD, 0, 0);                            /* JMP $FFD0 */
}

void bank_scan(fw *f, fw_regs *r)
{
    fw_regs t = *r;                                                /* PHA / PHP / SEI */
    for (t.a = BANK_COUNT; t.a != 0; t.a--) {                      /* LDA #7 ... DEC A / BNE */
        bank_select(f, &t);
        bank_probe(f, &t);
        bank_read_id(f, &t);
    }
    bank_restore(f, r);
}

void bank_select(fw *f, fw_regs *r)
{
    uint8_t v = fw_rd8(f, (uint16_t)(BANK_P2_TABLE + r->a));       /* TAX / LDA $0200,X */
    if (fw_rd8(f, BOOT_DIP) & DIP_ALT_BANKS)
        v = fw_rd8(f, (uint16_t)(BANK_P2_TABLE_ALT + r->a));       /* BBC1 boot_dip not taken: LDA $0208,X */
    uint8_t keep = fw_sfr8(f, SFR_P2) & P2_KEEP;                   /* LDA #$83 / AND P2 */
    fw_wr8(f, TN_RESUME_POS, keep);                                /* STA $77 (its scratch byte) */
    fw_wr_sfr8(f, SFR_P2, v | keep);                               /* TXA / ORA $77 / STA P2 */
    r->x = v;
}

void bank_probe(fw *f, fw_regs *r)
{
    (void)r;
    fw_wr8(f, BANK_ABSENT, fw_rd8(f, BANK_ABSENT) | 1);           /* SEB0 bank_absent */
    if (fw_rom8(f, BANK_ENTRY_COLD) == BANK_SIGNATURE)             /* LDA $FFD0 / EOR #$78 / BNE */
        fw_wr8(f, BANK_ABSENT, fw_rd8(f, BANK_ABSENT) & ~1);      /* CLB0 bank_absent */
}

void bank_read_id(fw *f, fw_regs *r)
{
    uint8_t x = r->a, sum = 0;                                     /* PHA / TAX / LDA #0 */
    while (--x != 0)                                               /* DEX / BEQ; CLC / ADC #14 / BRA */
        sum += BANK_ID_CHARS;
    fw_regs t = { 0, sum, 0, 0 };                                  /* TAX / LDY #0 */
    if (fw_rd8(f, BANK_ABSENT) & 1) {                              /* BBS0 bank_absent: 14 spaces */
        do {
            fw_wr8(f, (uint16_t)(BANK_ID_TABLE + t.x), ' ');
            t.x++;
            t.y++;
        } while (t.y < BANK_ID_CHARS);
    } else {
        do {
            t.a = fw_rom8(f, (uint16_t)(BANK_HDR_LANG_CODE + t.y));   /* LDA $FFE2,Y */
            bank_put_id_char(f, &t);
            t.y++;
        } while (t.y < BANK_LANG_CODE_CHARS);
        for (uint16_t i = 0; i < BANK_VER_TEXT_CHARS; i++) {       /* LDA $9800 ... $9803 */
            t.a = fw_rom8(f, (uint16_t)(BANK_HDR_ENGINE_VER_TEXT + i));
            bank_put_id_char(f, &t);
        }
        for (uint16_t i = 0; i < BANK_VER_TEXT_CHARS; i++) {       /* LDA $FFEA ... $FFED */
            t.a = fw_rom8(f, (uint16_t)(BANK_HDR_LANG_VER_TEXT + i));
            bank_put_id_char(f, &t);
        }
    }
    r->x = t.x;                                                    /* PLA: A as it came */
    r->y = t.y;
}

void bank_put_id_char(fw *f, fw_regs *r)
{
    bank_safe_char(f, r);
    fw_wr8(f, (uint16_t)(BANK_ID_TABLE + r->x), r->a);             /* STA $2026,X */
    r->x++;
}

void bank_safe_char(fw *f, fw_regs *r)
{
    (void)f;
    if (r->a < 0x20)                                               /* CMP #$20 / BCS */
        r->a = '*';
}

void bank_cmd_select(fw *f, uint8_t x, uint8_t a)
{
    fw_regs r = { cmd_hex_value(f, a), x, cmd_handler_y(f), 0 };  /* JSR cmd_hex_value: A the bank, X and Y kept */
    bank_load_ramcode(f, &r);                                      /* JSR bank_load_ramcode */
    bank_enter(f, &r);                                             /* JSR bank_enter: back only for an absent bank */
    if (f->unwound)
        return;
    bank_find_current(f, &r);                                      /* JSR bank_find_current; JMP cmd_run_done */
}

void bank_find_current(fw *f, fw_regs *r)
{
    (void)r;
    uint8_t bits = fw_sfr8(f, SFR_P2) & P2_BANK_BITS;              /* PHA / TXA / PHA; LDX #1 / LDA #$7C / AND P2 */
    for (uint8_t x = 1; x < BANK_COUNT + 1; x++) {                 /* INX / CPX #8 / BCC */
        uint16_t table = (fw_rd8(f, BOOT_DIP) & DIP_ALT_BANKS) ? BANK_P2_TABLE_ALT : BANK_P2_TABLE;
        if (fw_rd8(f, (uint16_t)(table + x)) == bits) {            /* CMP $0208,X or CMP $0200,X / BEQ */
            fw_wr8(f, BANK_CURRENT, x);                            /* STX bank_current */
            return;
        }
    }
}
