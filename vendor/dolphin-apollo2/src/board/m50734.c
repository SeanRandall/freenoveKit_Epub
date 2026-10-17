#include <string.h>
#include "board/m50734.h"

/* 16 crystal clocks per prescaler step; one CPU cycle = 4 crystal clocks. */
static int32_t timer_period(const m50734 *p, int n)
{
    return 4 * (int32_t)(p->timer_pre[n] + 1) * (int32_t)(p->timer_reload[n] + 1);
}

void m50734_reset(m50734 *p, const uint8_t port_in[5])
{
    memset(p, 0, sizeof *p);
    memcpy(p->port_in, port_in, sizeof p->port_in);
    memset(p->timer_pre, 0xFF, sizeof p->timer_pre);
    memset(p->timer_reload, 0xFF, sizeof p->timer_reload);
}

bool m50734_is_modelled(uint8_t a)
{
    return (a >= 0xDC && a <= 0xE1) || (a >= 0xE4 && a <= 0xE7) || (a >= 0xE9 && a <= 0xEB) ||
           (a >= 0xED && a <= 0xF7) || a == 0xFB || a >= 0xFD;
}

bool m50734_is_write_only(uint8_t addr)
{
    return addr == M50734_UART_TX;
}

uint8_t m50734_port_pins(const m50734 *p, int port)
{
    return (uint8_t)((p->port_in[port] & ~p->port_dir[port]) | (p->port_latch[port] & p->port_dir[port]));
}

/* Data/direction pairs: P3 $EE/$EF, P2 $F0/$F1, P1 $F3/$F4, P0 $F6/$F7. Returns -1 otherwise. */
static int port_of(uint8_t a, bool *is_dir)
{
    switch (a) {
    case M50734_P3: *is_dir = false; return 3;
    case M50734_P3_DIR: *is_dir = true; return 3;
    case M50734_P2: *is_dir = false; return 2;
    case M50734_P2_DIR: *is_dir = true; return 2;
    case M50734_P1: *is_dir = false; return 1;
    case M50734_P1_DIR: *is_dir = true; return 1;
    case M50734_P0: *is_dir = false; return 0;
    case M50734_P0_DIR: *is_dir = true; return 0;
    default: return -1;
    }
}

uint8_t m50734_read(m50734 *p, uint8_t a)
{
    bool is_dir;
    int port;
    if (a < M50734_SFR_FIRST)
        return 0;
    port = port_of(a, &is_dir);
    if (port >= 0)
        return is_dir ? p->port_dir[port] : m50734_port_pins(p, port);
    if (a >= 0xDC && a <= 0xE1)
        return (a & 1) ? p->timer_reload[(a - 0xDC) >> 1] : p->timer_pre[(a - 0xDC) >> 1];
    switch (a) {
    case M50734_UART_RX: p->uart_rx_full = false; return p->uart_rx;
    case M50734_UART_CTRL: return p->uart_ctrl;
    case M50734_UART_STATUS: return (uint8_t)(0x02 | (p->uart_rx_full ? 0x01 : 0x00));
    case M50734_AD_CTRL: return p->ad_ctrl;
    case M50734_AD_DATA: return 0x00;
    case M50734_P4: return (uint8_t)(p->port_in[4] & 0x0F);
    case M50734_P23_FUNC: return p->p23_func;
    case M50734_P1_LATCH: return p->port_in[1];
    case M50734_P0_FUNC: return p->p0_func;
    case M50734_UART_BAUD: return p->uart_baud;
    case M50734_ICR3: return p->icr3;
    case M50734_ICR2: return p->icr2;
    case M50734_ICR1: return p->icr1;
    default: return p->other[a - M50734_SFR_FIRST];
    }
}

void m50734_write(m50734 *p, uint8_t a, uint8_t v)
{
    bool is_dir;
    int port;
    if (a < M50734_SFR_FIRST)
        return;
    port = port_of(a, &is_dir);
    if (port >= 0) {
        if (is_dir)
            p->port_dir[port] = v;
        else
            p->port_latch[port] = v;
        return;
    }
    if (a >= 0xDC && a <= 0xE1) {
        int n = (a - 0xDC) >> 1;
        if (a & 1)
            p->timer_reload[n] = v;
        else
            p->timer_pre[n] = v;
        p->timer_running[n] = true;          /* a write (re)starts the timer with a full period */
        p->timer_count[n] = timer_period(p, n);
        return;
    }
    switch (a) {
    case M50734_UART_TX: p->uart_tx_last = v; p->uart_tx_count++; break;
    case M50734_UART_RX: case M50734_UART_STATUS: case M50734_AD_DATA:
    case M50734_P4: case M50734_P1_LATCH:
        break;                               /* read-only */
    case M50734_UART_CTRL: p->uart_ctrl = v; break;
    case M50734_AD_CTRL: p->ad_ctrl = (uint8_t)((v & 0x03) | 0x04); break;
    case M50734_P23_FUNC: p->p23_func = (uint8_t)(v & 0xC7); break;
    case M50734_P0_FUNC: p->p0_func = v; break;
    case M50734_UART_BAUD: p->uart_baud = v; break;
    case M50734_ICR3: p->icr3 = v; break;
    case M50734_ICR2: p->icr2 = (uint8_t)(v & 0x3F); break;
    case M50734_ICR1: p->icr1 = v; break;
    default: p->other[a - M50734_SFR_FIRST] = v; break;
    }
}

void m50734_tick(m50734 *p, int cpu_cycles)
{
    for (int n = 0; n < 3; n++) {
        if (!p->timer_running[n])
            continue;
        p->timer_count[n] -= cpu_cycles;
        while (p->timer_count[n] <= 0) {
            p->icr2 |= (uint8_t)(1u << (2 * n + 1));   /* timer n+1 request bit */
            p->timer_count[n] += timer_period(p, n);
        }
    }
}

uint32_t m50734_irq_lines(const m50734 *p)
{
    uint32_t lines = 0;
    if (p->icr1 & (p->icr1 >> 1) & 0x04)
        lines |= 1u << M50734_LINE_INT2PIN;
    if ((p->icr1 & (p->icr1 >> 1) & 0x01) || (p->uart_rx_full && (p->uart_ctrl & 0x10)))
        lines |= 1u << M50734_LINE_INT1;
    if (p->icr2 & (p->icr2 >> 1) & 0x15)
        lines |= 1u << M50734_LINE_TIMER;
    if (p->icr1 & (p->icr1 >> 1) & 0x50)
        lines |= 1u << M50734_LINE_STEP;
    if (p->icr3 & (p->icr3 >> 1) & 0x50)
        lines |= 1u << M50734_LINE_TX_CNTR;
    return lines;
}

bool m50734_uart_receive(m50734 *p, uint8_t byte)
{
    if (p->uart_rx_full)
        return false;
    p->uart_rx = byte;
    p->uart_rx_full = true;
    return true;
}
