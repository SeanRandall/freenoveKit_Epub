#include "isr_dsp.h"
#include "fw_ops.h"
#include "isr_addr.h"

#define P3_KEPT         0x87   /* isr_set_volume $ACF1 LDA #$87 / AND P3: the bits the volume leaves */
#define P3_TICK         0      /* isr_timer_start $B369 SEB0 P3 */
#define P2_DSP_RESET    0      /* P2.0: the DSP reset line */
#define ICR2_T2_EN      2      /* ICR2: timer 2's enable and request bits ... */
#define ICR2_T2_REQ     3
#define ICR2_T3_EN      4      /* ... and timer 3's */
#define ICR2_T3_REQ     5
#define ICR2_T3_ONLY    0x10   /* $B385: timer 3's interrupt enabled, nothing else */
#define T2_RELOAD_SYNC  4      /* $B36B */
#define T3_RELOAD_SYNC  0x28   /* $B375 */
#define T3_RELOAD_TICK  0x31   /* $B382: 4 x 50 CPU cycles, the 10 kHz tick */
#define ISR_TICKS       10     /* isr_tick_div's start: ticks per DSP word */
#define ISR_WORDS       10     /* isr_words_left's start: words per frame */
#define ISR_FRAME_A_LO  0xA5   /* the low bytes of the ping-pong buffers isr_frame_a $16A5 ... */
#define ISR_FRAME_B_LO  0xB9   /* ... and isr_frame_b $16B9 */
#define ISR_SWAP_PHASES 2      /* $B313: isr_swap_phase counts 2 -> 1 -> 2 */

/* ---- isr_timer_dsp $B2D9 ---- */

void isr_timer_dsp(fw *f, fw_regs *r)
{
    (void)r;                                         /* PHA / TYA / PHA ... PLA / TAY / PLA; RTI: A, X, Y kept */
    fw_sfr_clb(f, SFR_ICR2, ICR2_T3_REQ);            /* CLB5 ICR2 */
    uint8_t t = (uint8_t)(fw_rd8(f, ISR_TICK_DIV) - 1);   /* DEC isr_tick_div; BEQ */
    fw_wr8(f, ISR_TICK_DIV, t);
    if (t != 0)
        return;                                      /* $B2DF RTI */
    uint8_t y = fw_rd8(f, ISR_WORD_OFS);             /* LDY isr_word_ofs */
    fw_dsp_write(f, fw_ind_y(f, ISR_PLAY_PTR, y));   /* LDA (isr_play_ptr),Y; STA DSP_DATA */
    y = (uint8_t)(y + 1);                            /* INY */
    fw_dsp_write(f, fw_ind_y(f, ISR_PLAY_PTR, y));
    y = (uint8_t)(y + 1);
    fw_wr8(f, ISR_WORD_OFS, y);                      /* STY isr_word_ofs */
    fw_wr8(f, ISR_TICK_DIV, ISR_TICKS);              /* LDM #$0A */
    uint8_t w = (uint8_t)(fw_rd8(f, ISR_WORDS_LEFT) - 1);   /* DEC isr_words_left; BNE */
    fw_wr8(f, ISR_WORDS_LEFT, w);
    if (w != 0)
        return;
    fw_wr8(f, ISR_WORDS_LEFT, ISR_WORDS);            /* LDM #$0A */
    fw_wr8(f, ISR_WORD_OFS, 0);                      /* LDM #0 */
    if (fw_rd8(f, ISR_FRAME_FLAG) & 1)               /* BBC0 isr_frame_flag: still set, the frame repeats */
        return;
    fw_frame_done(f);                                /* $B306 isr_swap_buffers: the frame event */
    fw_wr8(f, ISR_FRAME_FLAG, 1);                    /* LDM #1 */
    uint8_t phase = (uint8_t)(fw_rd8(f, ISR_SWAP_PHASE) - 1);   /* DEC isr_swap_phase; BNE */
    fw_wr8(f, ISR_SWAP_PHASE, phase);
    if (phase == 0) {
        fw_wr8(f, ISR_PLAY_PTR, ISR_FRAME_A_LO);     /* LDM: the low bytes only */
        fw_wr8(f, ISR_FILL_PTR, ISR_FRAME_B_LO);
        fw_wr8(f, ISR_SWAP_PHASE, ISR_SWAP_PHASES);
    } else {
        fw_wr8(f, ISR_PLAY_PTR, ISR_FRAME_B_LO);     /* $B319 */
        fw_wr8(f, ISR_FILL_PTR, ISR_FRAME_A_LO);
    }
}

/* ---- isr_set_volume $ACE8, isr_volume_half $B46C ---- */

void isr_set_volume(fw *f, fw_regs *r)
{
    fw_wr8(f, FW_TMP_75, util_shl3((uint8_t)((r->a ^ 0x0F) & 0x0F)));   /* EOR #$F; AND #$F; JSR util_shl3; STA $75 */
    r->a = (uint8_t)((fw_sfr8(f, SFR_P3) & P3_KEPT) | fw_rd8(f, FW_TMP_75));   /* LDA #$87; AND P3; ORA $75 */
    fw_wr_sfr8(f, SFR_P3, r->a);                                          /* STA P3 */
}

void isr_volume_half(fw *f, fw_regs *r)
{
    r->a = (uint8_t)(fw_rd8(f, CMD_VOLUME) >> 1);                         /* LDA cmd_volume; LSR A */
    isr_set_volume(f, r);
}

/* ---- isr_dsp_reset $B38B, isr_timer_start $B35D, isr_speech_stop $B398 ---- */

void isr_dsp_reset(fw *f, fw_regs *r)
{
    (void)r;                                         /* PHP; SEI ... PLP; NOP: registers and flags kept */
    fw_sfr_clb(f, SFR_P2, P2_DSP_RESET);             /* CLB0 P2; NOP */
    fw_sfr_seb(f, SFR_P2, P2_DSP_RESET);             /* SEB0 P2; NOP */
    fw_sfr_clb(f, SFR_P2, P2_DSP_RESET);             /* CLB0 P2 */
}

void isr_timer_start(fw *f, fw_regs *r)
{
    (void)r;                                         /* PHP; SEI ... PLP; NOP: registers and flags kept */
    fw_wr_sfr8(f, SFR_T2_PRE, 0);                    /* LDM #0,T2_PRE */
    fw_wr_sfr8(f, SFR_T3_PRE, 0);                    /* LDM #0,T3_PRE */
    fw_wr8(f, ISR_TICK_DIV, (uint8_t)(fw_rd8(f, ISR_TICK_DIV) - 1));   /* DEC isr_tick_div, twice */
    fw_wr8(f, ISR_TICK_DIV, (uint8_t)(fw_rd8(f, ISR_TICK_DIV) - 1));
    fw_sfr_seb(f, SFR_P3, P3_TICK);                  /* SEB0 P3 */
    fw_wr_sfr8(f, SFR_T2_RELOAD, T2_RELOAD_SYNC);    /* LDM #4,T2_RELOAD */
    fw_sfr_clb(f, SFR_ICR2, ICR2_T2_REQ);            /* CLB3 ICR2 */
    fw_sfr_clb(f, SFR_ICR2, ICR2_T2_EN);             /* CLB2 ICR2 */
    fw_wait(f, FW_WAIT_TIMER2, FW_WAIT_TIMER2_LOOP); /* $B372 BBC3 ICR2,$B372 */
    if (f->fault)
        return;                                      /* the wait failed (without a trap): the bit would never come */
    fw_wr_sfr8(f, SFR_T3_RELOAD, T3_RELOAD_SYNC);    /* LDM #$28,T3_RELOAD */
    fw_sfr_clb(f, SFR_ICR2, ICR2_T3_REQ);            /* CLB5 ICR2 */
    fw_sfr_clb(f, SFR_ICR2, ICR2_T3_EN);             /* CLB4 ICR2 */
    fw_wr_sfr8(f, SFR_P23_FUNC, 1);                  /* LDM #1,P23_FUNC */
    fw_wait(f, FW_WAIT_TIMER3, FW_WAIT_TIMER3_LOOP); /* $B37F BBC5 ICR2,$B37F */
    if (f->fault)
        return;
    fw_wr_sfr8(f, SFR_T3_RELOAD, T3_RELOAD_TICK);    /* LDM #$31,T3_RELOAD */
    fw_wr_sfr8(f, SFR_ICR2, ICR2_T3_ONLY);           /* LDM #$10,ICR2 */
}

void isr_speech_stop(fw *f, fw_regs *r)
{
    fw_wr_sfr8(f, SFR_P23_FUNC, 0);                  /* LDM #0,P23_FUNC */
    fw_wr_sfr8(f, SFR_ICR2, 0);                      /* LDM #0,ICR2 */
    isr_dsp_reset(f, r);
    fw_clb(f, BOOT_SPEAKING, 0);                     /* CLB0 boot_speaking */
}

/* ---- isr_speech_start $B323, isr_wait_frames $B356, isr_wait_frame $B465 ---- */

/* ASL A; ROL addr: the bit A shifts out goes into bit 0 of the RAM byte; the A it leaves. */
static uint8_t asl_rol(fw *f, uint16_t addr, uint8_t a)
{
    fw_wr8(f, addr, (uint8_t)(fw_rd8(f, addr) << 1 | a >> 7));
    return (uint8_t)(a << 1);
}

void isr_speech_start(fw *f, fw_regs *r)
{
    r->x = 0;                                        /* LDX #0; STX frm_source_index; STX frm_filter_index */
    fw_wr8(f, FRM_SOURCE_INDEX, r->x);
    fw_wr8(f, FRM_FILTER_INDEX, r->x);
    r->a = fw_rd8(f, CMD_SOURCE_PROSODY);
    r->a = asl_rol(f, FRM_SOURCE_INDEX, r->a);       /* bit 7 */
    r->a = asl_rol(f, FRM_SOURCE_INDEX, r->a);       /* bit 6 */
    r->a = asl_rol(f, FRM_SOURCE_INDEX, r->a);       /* bit 5 */
    r->a = asl_rol(f, FRM_FILTER_INDEX, r->a);       /* bit 4 */
    fw_wr8(f, ISR_TICK_DIV, ISR_TICKS);              /* LDM #$0A */
    fw_wr8(f, ISR_WORDS_LEFT, ISR_WORDS);            /* LDM #$0A */
    r->a = 0;                                        /* LDA #0; STA isr_word_ofs; STA isr_frame_flag */
    fw_wr8(f, ISR_WORD_OFS, r->a);
    fw_wr8(f, ISR_FRAME_FLAG, r->a);
    isr_dsp_reset(f, r);
    isr_timer_start(f, r);
    r->x = 1;                                        /* LDX #1 */
    isr_wait_frames(f, r);
}

void isr_wait_frames(fw *f, fw_regs *r)
{
    do {
        isr_wait_frame(f, r);
        r->x = (uint8_t)(r->x - 1);                  /* DEX; BNE isr_wait_frames */
    } while (r->x != 0);
}

void isr_wait_frame(fw *f, fw_regs *r)
{
    (void)r;
    while (!(fw_rd8(f, ISR_FRAME_FLAG) & 1)) {       /* BBC0 isr_frame_flag,isr_wait_frame */
        fw_wait(f, FW_WAIT_INTERRUPT, ROM_ISR_WAIT_FRAME);
        if (f->fault)
            return;                                  /* the wait failed (without a trap): the flag would never come */
    }
    fw_wr8(f, ISR_FRAME_FLAG, 0);                    /* LDM #0,isr_frame_flag */
}
