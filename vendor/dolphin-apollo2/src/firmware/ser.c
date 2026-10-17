#include "ser.h"
#include "cmd_imm.h"
#include "fw_ops.h"
#include "data/fw_data.h"

#define SER_TX_PAGE       0x39   /* the output ring, $3900-$39FF */
#define SER_RX_PAGE_FIRST 0x21   /* the input ring, $2100-$38FF */
#define SER_RX_PAGE_END   0x39
#define SER_CMD_BUF_LEN   5      /* the bytes ser_rx_cmd_start clears */
#define SER_CR            0x0D   /* ser_tx_records' last byte */

/* ---- the receive path ---- */

void ser_rx_byte(fw *f, fw_regs *r)
{
    if (r->a == SER_CTRL_A || r->a == SER_CTRL_X) {              /* ser_rx_mute $C391 */
        fw_seb(f, SER_ABORT, 0);
        fw_seb(f, CMD_INDEX_RESET, 0);
        fw_wr8(f, CMD_CHAR, fw_rd8(f, SER_CMD_CHAR));
        fw_seb(f, SER_RX_IDLE, 0);
        fw_seb(f, SER_RX_IDLE_SAVED, 0);
        fw_clb(f, SER_CMD_MODE, 0);
        fw_wr8(f, SER_RX_READ, fw_rd8(f, SER_RX_WRITE));
        r->a = fw_rd8(f, SER_RX_WRITE + 1);
        fw_wr8(f, SER_RX_READ + 1, r->a);
        return;
    }
    if (r->a == fw_rd8(f, SER_CMD_CHAR)) {                       /* ser_rx_cmd_start $C3AC */
        fw_seb(f, SER_CMD_MODE, 0);
        r->y = fw_rd8(f, SER_RX_IDLE);
        fw_wr8(f, SER_RX_IDLE_SAVED, r->y);
        fw_wr16(f, SER_CMD_PTR, SER_CMD_BUF);                    /* LDM #$20 twice */
        r->y = 0;
        fw_wr8(f, SER_CMD_TABLE_OFS, 0);
        for (uint16_t i = 0; i < SER_CMD_BUF_LEN; i++)
            fw_wr8(f, (uint16_t)(SER_CMD_BUF + i), 0);
        return;
    }
    if (fw_rd8(f, SER_CMD_MODE) & 1)                             /* BBS0 ser_cmd_mode */
        cmd_imm_match(f, r);
    else
        ser_rx_put(f, r);
}

void ser_rx_put(fw *f, fw_regs *r)
{
    fw_clb(f, SER_RX_IDLE, 0);
    r->y = 0;
    fw_wr_ind_y(f, (uint8_t)SER_RX_WRITE, 0, r->a);              /* STA (ser_rx_write),Y */
    uint8_t lo = (uint8_t)(fw_rd8(f, SER_RX_WRITE) + 1);         /* INC ser_rx_write; BEQ */
    fw_wr8(f, SER_RX_WRITE, lo);
    if (lo != 0)
        return;
    r->y = fw_rd8(f, SER_RX_WRITE + 1);                          /* $C3D7 */
    ser_rx_next_page(f, r);
    fw_wr8(f, SER_RX_WRITE + 1, r->y);
}

void ser_rx_flush_cmd(fw *f, fw_regs *r)
{
    fw_regs t = *r;                                              /* PHA / TXA / PHA ... PLA / TAX / PLA */
    t.a = fw_rd8(f, SER_CMD_CHAR);
    ser_rx_put(f, &t);
    t.x = 0;
    for (;;) {
        t.a = fw_rd8(f, (uint16_t)(SER_CMD_BUF + t.x));          /* $C404 LDA $2020,X; BEQ */
        if (t.a == 0)
            break;
        ser_rx_put(f, &t);
        t.x++;
    }
    r->y = t.y;
}

void ser_rx_next_page(fw *f, fw_regs *r)
{
    (void)f;
    r->y++;                                                      /* INY */
    if (r->y == SER_RX_PAGE_END)                                 /* CPY #$39 */
        r->y = SER_RX_PAGE_FIRST;
}

/* ---- the reply output ---- */

void ser_tx_next_page(fw *f, fw_regs *r)
{
    (void)f;
    r->y++;                                                      /* INY */
    if (r->y == SER_TX_PAGE + 1)                                 /* CPY #$3A */
        r->y = SER_TX_PAGE;
}

/* INC ptr; BNE; LDY ptr+1; JSR ser_tx_next_page; STY ptr+1 ($C4C0-$C4C9, $C4EA-$C4F3): the output ring pointer at
   ptr moves on; at a page end Y = the new page. */
static void tx_step(fw *f, uint16_t ptr, fw_regs *r)
{
    uint8_t lo = (uint8_t)(fw_rd8(f, ptr) + 1);
    fw_wr8(f, ptr, lo);
    if (lo != 0)
        return;
    r->y = fw_rd8(f, (uint16_t)(ptr + 1));
    ser_tx_next_page(f, r);
    fw_wr8(f, (uint16_t)(ptr + 1), r->y);
}

void ser_tx_get(fw *f, fw_regs *r)
{
    r->a = SER_NAK;                                              /* $C4B5-$C4B7: PHP, SEI, LDA #$15 */
    if (fw_rd8(f, SER_TX_PENDING) & 1) {                         /* BBC0 ser_tx_pending */
        r->y = 0;
        r->a = fw_ind_y(f, (uint8_t)SER_TX_READ, 0);             /* LDA (ser_tx_read),Y */
        tx_step(f, SER_TX_READ, r);
        r->y = fw_rd8(f, SER_TX_COUNT);                          /* $C4CB: tested before the DEY */
        if (r->y != 0) {
            r->y--;
            fw_wr8(f, SER_TX_COUNT, r->y);
            if (r->y != 0)
                return;                                          /* PLP, RTS */
        }
    }
    fw_wr8(f, SER_TX_PENDING, 0);                                /* $C4D7 LDM #0 */
}

void ser_tx_put(fw *f, fw_regs *r)
{
    fw_regs t = *r;                                              /* PHA / TXA / PHA / TYA / PHA ... PLA: kept */
    fw_wr_ind_y(f, (uint8_t)SER_TX_WRITE, 0, r->a);              /* LDA $0103,X (the pushed A); STA (ser_tx_write),Y */
    t.y = 0;
    tx_step(f, SER_TX_WRITE, &t);
    fw_inc8(f, SER_TX_COUNT);
    fw_wr8(f, SER_TX_PENDING, 1);                                /* LDM #1 */
}

void ser_tx_send(fw *f, fw_regs *r)
{
    fw_wait(f, FW_WAIT_UART_TX, FW_WAIT_UART_TX_LOOP);           /* $C505 JSR ser_tx_wait (inside PHP ... PLP) */
    fw_wr_sfr8(f, SFR_UART_TX, r->a);                            /* $C508 STA UART_TX */
}

void ser_hex_digit(fw *f, fw_regs *r)
{
    r->y = r->a & 0x0F;                                          /* AND #$0F; TAY */
    r->a = fw_rom8(f, (uint16_t)(SER_HEX_CHARS + r->y));         /* LDA ser_hex_chars,Y */
}

void ser_hex_digits(fw *f, fw_regs *r, uint8_t pair[2])
{
    fw_regs t = *r;                                              /* TXA / PHA / TYA / PHA ... PLA / TAY / PLA / TAX */
    t.a = pair[0];                                               /* LDA $0105,X */
    ser_hex_digit(f, &t);
    pair[0] = t.a;
    t.a = (uint8_t)(pair[1] >> 4);                               /* LDA $0106,X; LSR A x4 */
    ser_hex_digit(f, &t);
    pair[1] = t.a;
    r->a = r->x;
}

void ser_tx_hex(fw *f, fw_regs *r)
{
    uint8_t pair[2] = { r->a, r->a };                            /* PHA x3: the last two become the digits */
    fw_regs t = *r;
    ser_hex_digits(f, &t, pair);
    t.a = pair[0];                                               /* PLA: the low nibble's digit first */
    ser_tx_put(f, &t);
    t.a = pair[1];
    ser_tx_put(f, &t);                                           /* the last PLA: A as it came */
}

bool ser_tx_service(fw *f, fw_regs *r)
{
    if (fw_rd8(f, BOOT_DIP) & 1)                                 /* $C582 BBS0 boot_dip: parallel mode */
        return false;
    if (!fw_sfr_bit(f, SFR_P0, P0_HOST_LINE))                    /* $C585 BBC2 P0: the host line low */
        return false;
    fw_sfr_clb(f, SFR_P0, P0_BUSY);                              /* CLB5 P0 */
    fw_regs t = *r;
    ser_tx_get(f, &t);
    ser_tx_send(f, &t);
    uint8_t pending = fw_rd8(f, SER_TX_PENDING);                 /* LDA ser_tx_pending */
    fw_wr8(f, SER_STATUS_DUE, pending);
    if (pending == 0) {                                          /* BNE: the whole byte */
        fw_wait(f, FW_WAIT_UART_TX, FW_WAIT_UART_TX_LOOP);       /* $C596 JSR ser_tx_wait */
        if (fw_sfr_bit(f, SFR_P0, P0_HOST_LINE))                 /* PHP, SEI, BBC2 P0 */
            fw_sfr_seb(f, SFR_P0, P0_BUSY);                      /* SEB5 P0 */
    }
    r->y = t.y;                                                  /* $C5A2 PLA: A as it came */
    return true;
}

void ser_tx_records(fw *f, fw_regs *r)
{
    fw_regs t = *r;                                              /* PHA / TXA / PHA / TYA / PHA ... PLA: kept */
    t.a = fw_rd8(f, SER_REPLY_ID);
    ser_tx_put(f, &t);
    t.a = fw_rd8(f, SER_REPLY_COUNT);
    ser_tx_hex(f, &t);
    t.a = fw_rd8(f, SER_REPLY_SIZE);
    ser_tx_hex(f, &t);
    t.y = 0;
    uint8_t n;
    do {
        t.x = fw_rd8(f, SER_REPLY_SIZE);                         /* $C5BA LDX ser_reply_size */
        do {
            t.a = fw_ind_y(f, (uint8_t)SER_REPLY_PTR, t.y);      /* LDA (ser_reply_ptr),Y */
            ser_tx_put(f, &t);
            t.y++;
            t.x--;
        } while (t.x != 0);                                      /* DEX; BNE */
        n = (uint8_t)(fw_rd8(f, SER_REPLY_COUNT) - 1);           /* $C5C5 LDX ser_reply_size; DEC ser_reply_count */
        fw_wr8(f, SER_REPLY_COUNT, n);
    } while (n != 0);
    t.a = SER_CR;
    ser_tx_put(f, &t);
}
