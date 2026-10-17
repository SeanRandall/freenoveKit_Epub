#include "boot.h"
#include "bank.h"
#include "bank_addr.h"
#include "boot_addr.h"
#include "boot_input.h"
#include "cmd_apply.h"
#include "cmd_handlers.h"
#include "isr_dsp.h"
#include "isr_serial.h"
#include "par_play.h"
#include "pros_make.h"
#include "ser.h"
#include "data/fw_data.h"

#define DIP_SWITCHES     0x0F   /* boot_read_dip: P4 bits 0-3 */
#define P3_JUMPER_1      1      /* P3.1 and P3.2: the baud jumpers */
#define P3_JUMPER_2      2
#define BOOT_TX_PAGE     0x39   /* boot_clear_input_state: the output ring $3900 ... */
#define BOOT_RX_PAGE     0x21   /* ... and the input ring $2100 */
#define BOOT_TMP_07      0x0007 /* zero page, unnamed in the annotations: boot_clear_input_state clears it ($ABFE) */
#define BOOT_CMD_CHAR    '@'    /* boot_set_defaults: cmd_char and ser_cmd_char */
#define BOOT_SAY_RATE    1      /* boot_init_baud: X = 1 speaks the rate */
#define BOOT_ZP_CLEAR    0xC9   /* boot_reset clears $C9 down to $01 */

typedef struct { uint16_t addr; uint8_t value; } setting;

/* The power-up @-settings of boot_set_defaults ($A742-$A769, an LDM each), before and after the two command
   characters */
static const setting k_defaults[] = {
    { CMD_PITCH, 8 }, { CMD_PROSODY, 3 }, { CMD_SPEED_LIMIT, 7 }, { CMD_SPEED, 3 }, { CMD_VOLUME, 6 },
    { CMD_WORD_PAUSE, 0 },
};
static const setting k_defaults2[] = {
    { CMD_PITCH_STEP, 8 }, { FW_UTT_CONTENT, 4 }, { CMD_PHRASE_PAUSE, 0x0B }, { CMD_VOICE, 1 },
    { CMD_MARK_SPACE, 0x14 }, { CMD_VOICING, 8 },
};
/* ... and the ones it clears ($A76C-$A782, STA of A = 0) */
static const uint16_t k_cleared[] = {
    CMD_SOURCE, CMD_PHONETIC, CMD_SPELL, CMD_PUNCT, CMD_HYPER, CMD_EV_COUNT, CMD_COUNT, CMD_SOUND_ACTIVE, CMD_SPEAKER,
    FW_CLAUSE_CARRY,
};
/* ... and boot_clear_input_state's ($ABF2-$AC04, STA of A = 0) */
static const uint16_t k_input_cleared[] = {
    SER_ABORT, BOOT_SPEAKING, SER_RX_IDLE, SER_RX_IDLE_SAVED, CMD_INDEX_COUNT, SER_TX_PENDING, BOOT_TMP_07,
    SER_TX_COUNT, ISR_PAR_STATE, SER_CMD_MODE,
};

void boot_read_dip(fw *f, fw_regs *r)
{
    fw_wr_sfr8(f, SFR_AD_CTRL, 0);                               /* $ADE0 LDM #0,AD_CTRL; NOP */
    r->a = fw_sfr8(f, SFR_P4) & DIP_SWITCHES;                    /* LDA P4; AND #$0F */
    fw_wr8(f, BOOT_DIP, r->a);
}

/* $AC5E-$AC66 (and boot_reset's first three LDMs, $A61C-$A622): P3 and its function. */
static void init_p3(fw *f)
{
    fw_wr_sfr8(f, SFR_P3_DIR, 0xF9);
    fw_wr_sfr8(f, SFR_P23_FUNC, 0);
    fw_wr_sfr8(f, SFR_P3, 0x78);
}

void boot_init_ports(fw *f, fw_regs *r)
{
    if (fw_rd8(f, BOOT_DIP) & DIP_PARALLEL) {                    /* $AC22 BBC0 boot_dip */
        r->a = 0;
        fw_wr_sfr8(f, SFR_P0_FUNC, r->a);
        fw_wr_sfr8(f, SFR_P0_DIR, 0x08);
        fw_wr_sfr8(f, SFR_P0, 0x00);
    } else {
        r->a = 0x48;                                             /* $AC31 */
        fw_wr_sfr8(f, SFR_P0_FUNC, r->a);
        fw_wr_sfr8(f, SFR_P0_DIR, 0x78);
        fw_wr_sfr8(f, SFR_P0, 0x20);
    }
    if (fw_rd8(f, BOOT_DIP) & DIP_PARALLEL) {                    /* $AC3B NOP; $AC3C BBC0 boot_dip */
        fw_wr_sfr8(f, SFR_P1, 0x01);
        fw_wr_sfr8(f, SFR_P1_DIR, 0xFF);
    } else {
        r->a = 0;                                                /* $AC47 */
        fw_wr_sfr8(f, SFR_P1_LATCH, r->a);
        fw_wr_sfr8(f, SFR_P1_DIR, r->a);
    }
    fw_wr_sfr8(f, SFR_P2, P2_MAIN_BANK);                         /* $AC4D BBC0: $AC50 or $AC58, the same two LDMs */
    fw_wr_sfr8(f, SFR_P2_DIR, 0xFD);
    init_p3(f);
}

void boot_clear_input_state(fw *f, fw_regs *r)
{
    r->a = 0;                                                    /* $ABF0 LDA #0 */
    for (unsigned i = 0; i < sizeof k_input_cleared / sizeof k_input_cleared[0]; i++)
        fw_wr8(f, k_input_cleared[i], r->a);
    fw_wr8(f, CMD_INDEX_RESET, 1);                               /* $AC06 LDM #1 */
    fw_wr8(f, SER_TX_WRITE, 0);                                  /* $AC09-$AC1E: the low bytes, then the pages */
    fw_wr8(f, SER_TX_READ, 0);
    fw_wr8(f, SER_RX_READ, 0);
    fw_wr8(f, SER_RX_WRITE, 0);
    fw_wr8(f, SER_TX_WRITE + 1, BOOT_TX_PAGE);
    fw_wr8(f, SER_TX_READ + 1, BOOT_TX_PAGE);
    fw_wr8(f, SER_RX_READ + 1, BOOT_RX_PAGE);
    fw_wr8(f, SER_RX_WRITE + 1, BOOT_RX_PAGE);
}

void boot_load_prosody_defaults(fw *f, fw_regs *r)
{
    r->y = 0;                                                    /* $A7F0 LDY #0 */
    do {
        r->a = fw_rom8(f, (uint16_t)(BOOT_PROSODY_DEFAULTS + r->y));   /* LDA $C65B,Y; STA $168B,Y */
        fw_wr8(f, (uint16_t)(PROS_PARAMS + r->y), r->a);
        r->y++;
    } while (r->y != BOOT_PROSODY_PARAMS);                       /* INY; CPY #$1A; BNE */
}

void boot_set_defaults(fw *f, fw_regs *r)
{
    for (unsigned i = 0; i < sizeof k_defaults / sizeof k_defaults[0]; i++)
        fw_wr8(f, k_defaults[i].addr, k_defaults[i].value);
    r->a = BOOT_CMD_CHAR;                                        /* $A754 LDA #$40 */
    fw_wr8(f, CMD_CHAR, r->a);
    fw_wr8(f, SER_CMD_CHAR, r->a);
    for (unsigned i = 0; i < sizeof k_defaults2 / sizeof k_defaults2[0]; i++)
        fw_wr8(f, k_defaults2[i].addr, k_defaults2[i].value);
    r->a = 0;                                                    /* $A76C LDA #0 */
    for (unsigned i = 0; i < sizeof k_cleared / sizeof k_cleared[0]; i++)
        fw_wr8(f, k_cleared[i], r->a);
    cmd_clear_formant(f);                                        /* $A785: A and Y kept */
    boot_load_prosody_defaults(f, r);
}

void boot_init_settings(fw *f, fw_regs *r)
{
    boot_clear_input_state(f, r);
    boot_set_defaults(f, r);
    r->a = fw_rom8(f, BANK_HDR_VERSION);                         /* $ADF1 LDA $FFE0 */
    fw_wr8(f, BANK_VER_MAJOR, r->a);
    r->a = fw_rom8(f, BANK_HDR_VERSION + 1);                     /* $ADF7 LDA $FFE1 */
    fw_wr8(f, BANK_VER_MINOR, r->a);
}

void boot_init_baud(fw *f, fw_regs *r)
{
    uint8_t baud;
    uint16_t text;
    fw_wr8(f, TN_PHON_LEN, 0);                                   /* $AC7E LDM #0 */
    if (fw_sfr_bit(f, SFR_P3, P3_JUMPER_1)) {                   /* $AC81 BBS1 P3 */
        if (fw_sfr_bit(f, SFR_P3, P3_JUMPER_2)) {                /* $AC9A BBS2 P3 */
            fw_wr_sfr8(f, SFR_UART_BAUD, 0x05);                  /* $ACC3: both jumpers, no text */
            return;
        }
        baud = 0xBF;
        text = BOOT_BAUD_TEXT_300;
    } else if (fw_sfr_bit(f, SFR_P3, P3_JUMPER_2)) {             /* $AC84 BBS2 P3 */
        baud = 0x2F;
        text = BOOT_BAUD_TEXT_1200;
    } else {
        baud = 0x02;
        text = BOOT_BAUD_TEXT_19200;
    }
    fw_wr_sfr8(f, SFR_UART_BAUD, baud);
    if (r->x != BOOT_SAY_RATE)                                   /* CPX #1; BNE */
        return;
    r->a = (uint8_t)text;                                        /* LDA #lo; STA tn_rule_ptr; LDA #hi; STA */
    fw_wr8(f, TN_RULE_PTR, r->a);
    r->a = (uint8_t)(text >> 8);
    fw_wr8(f, TN_RULE_PTR + 1, r->a);
    cmd_say_phonetic(f, r);
}

void boot_init_host_lines(fw *f, fw_regs *r)
{
    fw_wr_sfr8(f, SFR_UART_CTRL, 0xD1);                          /* $AC68 LDM #$D1 */
    r->a = 0x4C;                                                 /* LDA #$4C; STA P0_FUNC */
    fw_wr_sfr8(f, SFR_P0_FUNC, r->a);
    if (fw_sfr_bit(f, SFR_P0, P0_HOST_LINE)) {                   /* $AC6F BBC2 P0 */
        fw_sfr_clb(f, SFR_P0, P0_BUSY);
        isr_host_line_high(f, r);
    } else {
        fw_sfr_seb(f, SFR_P0, P0_BUSY);                          /* $AC78 */
        isr_host_line_low(f, r);
    }
}

void cmd_say_phonetic(fw *f, fw_regs *r)
{
    r->x = cmd_put_phonetic(f, r->x);                            /* $A876: A and Y kept */
    pros_make_segments(f, r);
    boot_loop_play(f, r);
}

void boot_selftest(fw *f, fw_regs *r)
{
    (void)r;
    fw_wr8(f, BOOT_SELFTEST_FAIL, 0);                            /* $C605 LDM #0 */
    if (!(fw_rd8(f, BOOT_DIP) & DIP_SELFTEST))                   /* BBC1 boot_dip */
        return;
    if (!fw_sfr_bit(f, SFR_P2, P2_SELFTEST_PIN))                 /* BBC1 P2 */
        return;
    fw_wr8(f, BOOT_SELFTEST_FAIL, 1);
}

void boot_selftest_sound(fw *f, fw_regs *r)
{
    fw_wr16(f, CMD_TEXT_PTR, BOOT_SELFTEST_OK_CMDS);             /* $A79F LDM, LDM */
    boot_selftest(f, r);
    if (fw_rd8(f, BOOT_SELFTEST_FAIL) & 1)                       /* $A7A8 BBC0 boot_selftest_fail */
        fw_wr16(f, CMD_TEXT_PTR, BOOT_SELFTEST_FAIL_CMDS);
    boot_queue_string(f, r);
    boot_loop_text(f, r);
    if (f->fault || f->stopped)
        return;                                                  /* abandoned (without a trap: a unit test) */
    pros_make_segments(f, r);
    boot_loop_play(f, r);
}

/* $A681-$A68A: the main loop, for ever (JMP $A681). It returns only when abandoned without a trap (a unit test). */
static void main_loop(fw *f, fw_regs *r)
{
    for (;;) {
        boot_loop_text(f, r);
        if (f->fault || f->stopped || f->unwound)
            return;
        pros_make_segments(f, r);
        boot_loop_play(f, r);
        if (f->fault || f->stopped)
            return;
    }
}

void boot_reentry_warm(fw *f, fw_regs *r)
{
    fw_wr8(f, SER_ABORT, 0);                                     /* $A66B LDM #0 */
    fw_set_interrupts(f, true, BOOT_CLI_WARM);                   /* $A66E CLI */
    isr_par_hold(f, r);
    isr_par_ready(f, r);
    r->a = cmd_apply_speaker(f, &r->x, &r->y);                   /* $A675 */
    bank_load_ramcode(f, r);
    bank_scan(f, r);
    bank_find_current(f, r);
    main_loop(f, r);
}

void boot_reentry_cold(fw *f, fw_regs *r)
{
    boot_read_dip(f, r);                                         /* $A665 */
    boot_init_settings(f, r);
    boot_reentry_warm(f, r);                                     /* falls into $A66B */
}

void bank_entry_cold(fw *f, fw_regs *r)
{
    fw_set_interrupts(f, false, BANK_ENTRY_COLD);                /* $FFD0 SEI */
    boot_reentry_cold(f, r);                                     /* JMP boot_reentry_cold */
}

void bank_entry_warm(fw *f, fw_regs *r) { boot_reentry_warm(f, r); }   /* $FFD4 JMP boot_reentry_warm */

void boot_reset(fw *f)
{
    init_p3(f);                                                  /* $A61C-$A622 */
    fw_reset_cpu(f);                                             /* $A625-$A62C */
    fw_regs r = { 0, 0, 0, 0 };                                  /* LDA #0; TAY; TAX */
    fw_wr_sfr8(f, SFR_ICR1, r.a);
    fw_wr_sfr8(f, SFR_ICR2, r.a);
    fw_wr_sfr8(f, SFR_ICR3, r.a);
    for (r.x = BOOT_ZP_CLEAR; r.x != 0; r.x--)                   /* $A637 LDX #$C9; STA 0,X; DEX; BNE */
        fw_wr8(f, r.x, r.a);
    fw_wr8(f, CMD_VOLUME, r.a);                                  /* $A63E LDA #0; STA cmd_volume */
    isr_set_volume(f, &r);
    boot_read_dip(f, &r);
    boot_init_ports(f, &r);
    fw_wr_sfr8(f, SFR_ICR1, 1);                                  /* $A64B LDM #1,ICR1 */
    boot_init_settings(f, &r);
    r.a = cmd_apply_speaker(f, &r.x, &r.y);                      /* $A651 */
    boot_selftest_sound(f, &r);
    if (f->fault || f->stopped)
        return;
    if (!(fw_rd8(f, BOOT_DIP) & DIP_PARALLEL)) {                 /* $A657 BBS0 boot_dip */
        r.x = 0;
        boot_init_baud(f, &r);
        boot_init_host_lines(f, &r);
    }
    boot_reentry_warm(f, &r);                                    /* $A662 JMP boot_reentry_warm */
}

void isr_unused(fw *f, fw_regs *r)
{
    (void)f;
    (void)r;                                                     /* $C3DF RTI */
}
