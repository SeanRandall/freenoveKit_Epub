#include "isr_serial.h"
#include "fw_ops.h"
#include "ser.h"

#define UART_ERRORS      0x1C   /* UART_STATUS bits 2-4 */
#define UART_RX_IRQ      4      /* UART_CTRL bit 4: the receive interrupt */
#define ICR1_INT1_EN     0      /* ICR1: the _INT1 enable and request bits */
#define ICR1_INT1_REQ    1
#define ICR3_INT4_EN     6      /* ICR3: the INT4 enable and request bits */
#define ICR3_INT4_REQ    7
#define ICR3_HOST_HIGH   0x40
#define ICR3_HOST_LOW    0x48
#define P0_ACK           0      /* P0.0: the host's acknowledge (parallel mode) */
#define P0_READY         3      /* P0.3: ready for the next host byte */
#define P0_STROBE        4      /* P0.4: the host's strobe (parallel mode) */
#define PAR_REPLY_CODE   5      /* P1_LATCH: the host asks for a reply byte */
#define PAR_DATA_MIN     7      /* P1_LATCH: a data byte from here on */

/* ser_int1_serial $C33C-$C37D, ser_int1_latch $C367: PHA / TXA / PHA / TYA / PHA ... PLA / TAY / PLA / TAX / PLA. */
static void int1_serial(fw *f, const fw_regs *r)
{
    fw_regs t = *r;
    if (fw_sfr_bit(f, SFR_ICR1, ICR1_INT1_REQ)) {                /* BBS1 ICR1: ser_int1_latch */
        t.a = fw_sfr8(f, SFR_P1_LATCH);
        fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_EN);
        fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_REQ);
        ser_rx_byte(f, &t);                                      /* PHP; CLI ... PLP; NOP */
        fw_sfr_seb(f, SFR_ICR1, ICR1_INT1_EN);
    } else {
        fw_sfr_clb(f, SFR_UART_CTRL, UART_RX_IRQ);
        fw_sfr_seb(f, SFR_P0, P0_BUSY);
        fw_sfr_clb(f, SFR_ICR3, ICR3_INT4_EN);
        fw_sfr_clb(f, SFR_ICR3, ICR3_INT4_REQ);
        if (!(fw_sfr8(f, SFR_UART_STATUS) & UART_ERRORS)) {      /* AND #$1C; BNE */
            t.a = fw_sfr8(f, SFR_UART_RX);
            ser_rx_byte(f, &t);                                  /* PHP; CLI ... PLP; NOP */
        }
        fw_sfr_seb(f, SFR_ICR3, ICR3_INT4_EN);                   /* $C35B */
        fw_wr_sfr8(f, SFR_UART_RX, fw_sfr8(f, SFR_UART_RX));     /* LDA UART_RX; STA UART_RX */
        fw_sfr_seb(f, SFR_UART_CTRL, UART_RX_IRQ);
        fw_sfr_clb(f, SFR_P0, P0_BUSY);
    }
    fw_sfr_clb(f, SFR_P0, P0_READY);                             /* $C37B */
}

/* $C2BA-$C33B: parallel mode. */
static void int1_parallel(fw *f, const fw_regs *r)
{
    if (fw_sfr_bit(f, SFR_P0, P0_ACK)) {                         /* BBC0 P0: $C2BD */
        fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_REQ);
        return;
    }
    if (!fw_sfr_bit(f, SFR_P0, P0_STROBE)) {                     /* BBS4 P0 */
        bool done = (fw_rd8(f, ISR_PAR_STATE) & 1) != 0;         /* BBS0 isr_par_state */
        fw_wr_sfr8(f, SFR_P1, 0);
        fw_wr_sfr8(f, SFR_P1_DIR, 0xFF);
        if (!done) {                                             /* $C2C6 */
            fw_sfr_clb(f, SFR_P0, P0_READY);
            fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_REQ);
            fw_sfr_seb(f, SFR_P0, P0_READY);
        } else {                                                 /* $C2D6 */
            fw_clb(f, ISR_PAR_STATE, 0);
            fw_sfr_clb(f, SFR_P0, P0_READY);
            fw_wr_sfr8(f, SFR_P1_LATCH, 0);
            fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_REQ);
        }
        fw_wr_sfr8(f, SFR_P1_DIR, 0);
        return;
    }
    fw_regs t = *r;                                              /* $C2E9 */
    fw_sfr_clb(f, SFR_P0, P0_READY);
    fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_REQ);
    fw_wr_sfr8(f, SFR_P1, 1);
    fw_wr_sfr8(f, SFR_P1_DIR, 0xFF);
    fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_EN);                       /* after PHA ... PHP; then CLI */
    t.a = fw_sfr8(f, SFR_P1_LATCH);
    if (t.a >= PAR_DATA_MIN) {                                   /* CMP #7; BCS $C315 */
        if (!(fw_rd8(f, ISR_PAR_STATE) & 1)) {                   /* BBS0 isr_par_state */
            t.y = t.a;                                           /* TAY */
            uint8_t ack = (uint8_t)(t.a ^ 0x01);
            fw_wr8(f, ISR_PAR_STATE, 1);
            if (fw_rd8(f, SER_TX_PENDING) & 1) {                 /* BBC0 ser_tx_pending */
                fw_wr8(f, ISR_PAR_STATE, 3);
                ack ^= 0x02;
            }
            fw_wr_sfr8(f, SFR_P1, ack);
            fw_wr_sfr8(f, SFR_P1_DIR, 0xFF);
            fw_sfr_seb(f, SFR_P0, P0_READY);
            t.a = t.y;                                           /* TYA */
            ser_rx_byte(f, &t);
        }
    } else if (t.a == PAR_REPLY_CODE) {                          /* CMP #5; BNE */
        fw_clb(f, ISR_PAR_STATE, 1);
        fw_seb(f, ISR_PAR_STATE, 0);
        ser_tx_get(f, &t);
        fw_wr_sfr8(f, SFR_P1, t.a);
        fw_wr_sfr8(f, SFR_P1_DIR, 0xFF);
    }
    fw_sfr_seb(f, SFR_ICR1, ICR1_INT1_EN);                       /* $C331: SEI; SEB0 ICR1; PLP ... PLA */
}

void isr_int1(fw *f, fw_regs *r)
{
    if (fw_rd8(f, BOOT_DIP) & 1)                                 /* BBS0 boot_dip */
        int1_parallel(f, r);
    else
        int1_serial(f, r);                                       /* JMP ser_int1_serial */
}

void isr_int4_host_line(fw *f, fw_regs *r)
{
    fw_regs t = *r;
    if (fw_sfr_bit(f, SFR_P0, P0_HOST_LINE))                     /* BBS2 P0 */
        isr_host_line_high(f, &t);
    else
        isr_host_line_low(f, &t);
}

void isr_host_line_high(fw *f, fw_regs *r)
{
    (void)r;
    fw_wr_sfr8(f, SFR_ICR3, ICR3_HOST_HIGH);
    fw_sfr_seb(f, SFR_P0, P0_BUSY);
    fw_wr8(f, SER_STATUS_DUE, 1);
}

void isr_host_line_low(fw *f, fw_regs *r)
{
    (void)r;
    fw_wr_sfr8(f, SFR_ICR3, ICR3_HOST_LOW);
    fw_sfr_clb(f, SFR_P0, P0_BUSY);
}

void isr_par_ready(fw *f, fw_regs *r)
{
    (void)r;                                                     /* PHA ... PLA ($C5EA): A kept */
    if (fw_rd8(f, BOOT_DIP) & 1)                                 /* BBC0 boot_dip,$C5EA */
        fw_sfr_seb(f, SFR_P0, P0_READY);                         /* SEB3 P0; BRA $C5EA */
}

void isr_par_hold(fw *f, fw_regs *r)
{
    (void)r;                                                     /* PHA ... PLA ($C5EA): A kept */
    if (!(fw_rd8(f, BOOT_DIP) & 1))                              /* BBC0 boot_dip,$C5EA */
        return;
    fw_sfr_clb(f, SFR_P0, P0_READY);                             /* PHP; SEI; CLB3 P0 */
    fw_sfr_clb(f, SFR_ICR1, ICR1_INT1_REQ);                      /* CLB1 ICR1; PLP; NOP */
}
