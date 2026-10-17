#ifndef M50734_H
#define M50734_H
/* Mitsubishi M50734 on-chip peripherals at zero page $DA-$FF.
   Register semantics follow MAME's m50734.cpp; the UART is inferred from the Apollo ROM. */
#include <stdbool.h>
#include <stdint.h>

/* IRQ lines as seen by cpu740 (vector = 0xFFFC - 2*line) */
enum {
    M50734_LINE_INT2PIN = 0,
    M50734_LINE_INT1 = 1,      /* _INT1 pin and UART receive: $FFFA */
    M50734_LINE_TIMER = 2,     /* timers 1-3: $FFF8 */
    M50734_LINE_STEP = 3,      /* stepper counters: $FFF6 */
    M50734_LINE_TX_CNTR = 4,   /* Timer X / CNTR: $FFF4 */
    M50734_LINE_COUNT = 5,
};

enum {
    M50734_SFR_FIRST = 0xDA,
    M50734_UART_TX = 0xE4, M50734_UART_RX = 0xE5, M50734_UART_CTRL = 0xE6, M50734_UART_STATUS = 0xE7,
    M50734_AD_CTRL = 0xE9, M50734_AD_DATA = 0xEA, M50734_P4 = 0xEB, M50734_P23_FUNC = 0xED,
    M50734_P3 = 0xEE, M50734_P3_DIR = 0xEF, M50734_P2 = 0xF0, M50734_P2_DIR = 0xF1,
    M50734_P1_LATCH = 0xF2, M50734_P1 = 0xF3, M50734_P1_DIR = 0xF4, M50734_P0_FUNC = 0xF5,
    M50734_P0 = 0xF6, M50734_P0_DIR = 0xF7, M50734_UART_BAUD = 0xFB,
    M50734_ICR3 = 0xFD, M50734_ICR2 = 0xFE, M50734_ICR1 = 0xFF,
};

typedef struct m50734 {
    uint8_t port_latch[4], port_dir[4];   /* P0..P3 */
    uint8_t port_in[5];                   /* board input levels for P0..P4 */
    uint8_t p0_func, p23_func, uart_baud, ad_ctrl;
    uint8_t icr1, icr2, icr3;             /* $FF, $FE, $FD: enable (even) / request (odd) bit pairs */
    uint8_t timer_pre[3], timer_reload[3];
    int32_t timer_count[3];               /* CPU cycles until the next expiry */
    bool timer_running[3];
    uint8_t uart_ctrl, uart_rx, uart_tx_last;
    bool uart_rx_full;
    uint32_t uart_tx_count;
    uint8_t other[0x100 - M50734_SFR_FIRST];   /* storage for registers that are not modelled */
} m50734;

void m50734_reset(m50734 *p, const uint8_t port_in[5]);
bool m50734_is_modelled(uint8_t addr);
bool m50734_is_write_only(uint8_t addr);   /* $E4 UART TX: reading it is an unmodelled access */
uint8_t m50734_read(m50734 *p, uint8_t addr);
void m50734_write(m50734 *p, uint8_t addr, uint8_t v);
void m50734_tick(m50734 *p, int cpu_cycles);
uint32_t m50734_irq_lines(const m50734 *p);
uint8_t m50734_port_pins(const m50734 *p, int port);
bool m50734_uart_receive(m50734 *p, uint8_t byte);

#endif
