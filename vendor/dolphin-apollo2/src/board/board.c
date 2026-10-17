#include <string.h>
#include "board/board.h"
#include "firmware/fw.h"
#include "firmware/boot.h"
#include "firmware/boot_addr.h"
#include "firmware/bank_addr.h"
#include "firmware/isr_addr.h"
#include "firmware/isr_dsp.h"
#include "firmware/isr_serial.h"
#include "firmware/ser_addr.h"

void board_init(board *b, const uint8_t port_in[5], uint32_t cycle_num, uint32_t cycle_den)
{
    memset(b, 0, sizeof *b);
    b->cycle_num = cycle_num ? cycle_num : 1;
    b->cycle_den = cycle_den ? cycle_den : 1;
    m50734_reset(&b->sfr, port_in);
    b->dsp_reset_level = (m50734_port_pins(&b->sfr, 2) & BOARD_P2_DSP_RESET) != 0;
    b->i = true;   /* cpu740_reset: P = I | B */
}

void board_set_outputs(board *b, board_frame_fn on_frame, void *user)
{
    b->on_frame = on_frame;
    b->user = user;
}

void board_set_raw_output(board *b, board_frame_fn on_raw, void *user)
{
    b->on_raw = on_raw;
    b->raw_user = user;
}

bool board_feed_serial(board *b, const uint8_t *bytes, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        size_t next = (b->q_tail + 1) % BOARD_QUEUE_SIZE;
        if (next == b->q_head)
            return false;
        b->queue[b->q_tail] = bytes[i];
        b->q_tail = next;
    }
    return true;
}

bool board_main_bank(const board *b)
{
    return (m50734_port_pins(&b->sfr, 2) & BOARD_P2_BANK_MASK) == BOARD_P2_MAIN_BANK;
}

static void note_unmodelled(board *b, uint16_t addr, bool write)
{
    for (unsigned i = 0; i < b->unmodelled_count; i++)
        if (b->unmodelled[i].addr == addr && b->unmodelled[i].write == write)
            return;
    if (b->unmodelled_count < BOARD_MAX_UNMODELLED) {
        b->unmodelled[b->unmodelled_count].addr = addr;
        b->unmodelled[b->unmodelled_count].write = write;
        b->unmodelled_count++;
    } else {
        b->unmodelled_dropped++;
    }
}

uint8_t board_read_sfr(board *b, uint8_t addr)
{
    if (!m50734_is_modelled(addr) || m50734_is_write_only(addr))
        note_unmodelled(b, addr, false);
    return m50734_read(&b->sfr, addr);
}

void board_write_sfr(board *b, uint8_t addr, uint8_t v)
{
    if (!m50734_is_modelled(addr))
        note_unmodelled(b, addr, true);
    m50734_write(&b->sfr, addr, v);
    if ((b->sfr.p0_func & 0x20) && (b->sfr.port_dir[0] & 0x20))
        b->fault = BOARD_FAULT_DME;   /* data-memory strobe: never used by the Apollo ROM */
    bool reset = (m50734_port_pins(&b->sfr, 2) & BOARD_P2_DSP_RESET) != 0;
    if (reset && !b->dsp_reset_level) {   /* rising edge: the DSP restarts at word 0 */
        b->dsp_resets++;
        b->raw_discarded += b->raw_pos;
        b->raw_pos = 0;
    }
    b->dsp_reset_level = reset;
}

void board_dsp_write(board *b, uint8_t v)
{
    b->port_writes++;
    b->raw_buf[b->raw_pos++] = v;
    if (b->raw_pos == BOARD_FRAME_BYTES) {
        b->raw_pos = 0;
        b->raw_frames++;
        if (b->on_raw)
            b->on_raw(b->raw_user, b->raw_buf);
    }
}

uint8_t board_peek(const board *b, uint16_t addr)
{
    if (addr >= M50734_SFR_FIRST && addr <= 0xFF)
        return 0;
    if (addr < BOARD_RAM_SIZE)
        return b->ram[addr];
    if (!b->rom || (addr >= 0x8000 && !board_main_bank(b)))
        return 0xFF;
    if (addr < 0x8000 && (m50734_port_pins(&b->sfr, 2) & P2_LOW_WINDOW))
        return 0xFF;   /* the lower half: not in the image the board holds (the C never points a frame there) */
    return b->rom[addr];
}

void board_capture_frame(board *b)
{
    uint8_t frame[BOARD_FRAME_BYTES];
    uint16_t buf = (uint16_t)(b->ram[BOARD_PRODUCER_PTR] | b->ram[BOARD_PRODUCER_PTR + 1] << 8);
    for (int i = 0; i < BOARD_FRAME_BYTES; i++)
        frame[i] = board_peek(b, (uint16_t)(buf + i));
    b->frames_produced++;
    if (b->on_frame)
        b->on_frame(b->user, frame);
}

static bool at_idle(const board *b) { return b->waiting && b->idle_point; }

/* maybe_deliver: one byte per idle WIT, after the previous byte was read, while P0.5 is not busy. */
static void maybe_deliver(board *b)
{
    if (!at_idle(b))
        return;
    uint8_t busy = (uint8_t)(m50734_port_pins(&b->sfr, 0) & BOARD_P0_BUSY);
    if (!b->busy_level_known) {   /* the "not busy" level is the one seen at the first idle */
        b->busy_idle_level = busy;
        b->busy_level_known = true;
    }
    if (b->sfr.uart_rx_full || b->q_head == b->q_tail || busy != b->busy_idle_level)
        return;
    m50734_uart_receive(&b->sfr, b->queue[b->q_head]);
    b->q_head = (b->q_head + 1) % BOARD_QUEUE_SIZE;
}

void board_prepare(board *b)
{
    maybe_deliver(b);
    b->irq_lines = m50734_irq_lines(&b->sfr);
}

bool board_idle(const board *b)
{
    return at_idle(b) && b->q_head == b->q_tail && !b->sfr.uart_rx_full && !(b->sfr.icr2 & BOARD_ICR2_T3_ENABLE) &&
           !b->irq_lines;
}

uint16_t board_vector(const board *b, uint16_t addr)
{
    if (!b->rom || !board_main_bank(b))
        return BOARD_EMPTY_VECTOR;
    return (uint16_t)(b->rom[addr] | b->rom[addr + 1] << 8);
}

/* The C of the reset vector, which takes no registers. */
static void reset_handler(fw *f, fw_regs *r)
{
    (void)r;
    boot_reset(f);
}

/* The dispatch table (M5a spec 3.4): each handler address of the vector table and each entry, and its C. */
static const struct { uint16_t addr; void (*fn)(fw *f, fw_regs *r); } k_dispatch[] = {
    { ROM_ISR_UNUSED, isr_unused },                  /* lines 0 and 3 */
    { ROM_ISR_INT1, isr_int1 },                      /* line 1 */
    { ROM_ISR_TIMER_DSP, isr_timer_dsp },            /* line 2 */
    { ROM_ISR_INT4_HOST_LINE, isr_int4_host_line },  /* line 4 */
    { ROM_BOOT_RESET, reset_handler },               /* the reset vector */
    { BANK_ENTRY_COLD, bank_entry_cold },            /* the bank entries (fw_enter) */
    { BANK_ENTRY_WARM, bank_entry_warm },
};

bool board_has_handler(uint16_t addr)
{
    for (size_t i = 0; i < sizeof k_dispatch / sizeof k_dispatch[0]; i++)
        if (k_dispatch[i].addr == addr)
            return true;
    return false;
}

bool board_call(board *b, uint16_t addr, uint8_t a, uint8_t x)
{
    for (size_t i = 0; b->fw && i < sizeof k_dispatch / sizeof k_dispatch[0]; i++) {
        if (k_dispatch[i].addr != addr)
            continue;
        uint8_t p2 = m50734_port_pins(&b->sfr, 2);   /* sync_p2: the window and the bank from the pins */
        b->fw->low_window = (p2 & P2_LOW_WINDOW) != 0;
        b->fw->bank = p2 & BOARD_P2_BANK_MASK;
        fw_regs r = { a, x, 0, 0 };
        k_dispatch[i].fn(b->fw, &r);
        return true;
    }
    b->fault = BOARD_FAULT_VECTOR;
    b->fault_addr = addr;
    return false;
}

/* The peripheral timers advance by the step's cycles through the scale (exec's tail). */
static void tick(board *b, int cycles)
{
    b->cycles += (uint64_t)cycles;
    uint64_t acc = b->cycle_rem + (uint64_t)cycles * b->cycle_num;
    b->cycle_rem = acc % b->cycle_den;
    m50734_tick(&b->sfr, (int)(acc / b->cycle_den));
}

/* The instruction a wait mirrors at its current point: its cycles, *done set when the wait's code is over. */
typedef int (*board_insn)(board *b, void *ctx, bool *done);

/* One step after prepare, as machine.c's exec: code outside the main ROM faults (pc_escaped: every point of a wait is
   at $8000 or above); the handler of a taken interrupt runs (the hook: 0 cycles, then its RTI restores I); else
   cpu740_step's start (a waiting CPU with no line spends a WIT step; any line ends the wait; a line with I clear is
   taken, the lowest first: I saved and set, the vector read) or the instruction, then the tick. */
static bool exec(board *b, board_insn insn, void *ctx, bool *done)
{
    if (!board_main_bank(b)) {
        b->fault = BOARD_FAULT_ESCAPED;
        return false;
    }
    if (b->handler_due) {
        b->handler_due = false;
        if (board_call(b, b->handler, 0, 0))
            b->i = b->handler_i;   /* its RTI */
        return b->fault == BOARD_FAULT_NONE;
    }
    int cycles;
    if (b->waiting && !b->irq_lines) {
        cycles = BOARD_WIT_CYCLES;
    } else {
        b->waiting = false;   /* any raised line ends WIT, even while I is set */
        if (b->irq_lines && !b->i) {
            int line = 0;
            while (!(b->irq_lines & (1u << line)))
                line++;
            b->handler_i = b->i;
            b->i = true;
            b->handler = board_vector(b, (uint16_t)(0xFFFC - 2 * line));
            b->handler_due = true;
            cycles = BOARD_IRQ_CYCLES;
        } else if (insn) {
            cycles = insn(b, ctx, done);
        } else {
            return false;   /* no instruction here: a wait for an interrupt never runs one */
        }
    }
    tick(b, cycles);
    return b->fault == BOARD_FAULT_NONE;
}

bool board_wait_interrupt(board *b, uint64_t end)
{
    b->waiting = true;   /* a WIT at the C's point: not the idle point, so no byte is delivered */
    b->idle_point = false;
    while (b->cycles < end) {
        board_prepare(b);
        if (b->waiting && b->irq_lines && b->i)
            break;   /* the CPU would wake and run on without taking the interrupt */
        if (!exec(b, NULL, NULL, NULL))
            break;
        if (!b->waiting && !b->handler_due)
            return true;   /* the handler's RTI is back */
    }
    b->waiting = false;
    b->handler_due = false;
    return false;
}

board_input_result board_wait_input(board *b, uint64_t end, board_idle_fn idle, void *user)
{
    b->waiting = true;   /* the WIT at $C602 executed: the CPU waits at the idle point */
    b->idle_point = true;
    while (b->cycles < end) {
        board_prepare(b);
        if (board_idle(b)) {
            if (!idle || idle(b, user) == BOARD_STOP)
                return BOARD_INPUT_STOPPED;   /* left at the idle point */
            continue;                         /* fed: the next prepare delivers a byte */
        }
        if (b->waiting && b->irq_lines && b->i)
            break;
        if (!exec(b, NULL, NULL, NULL))
            break;
        if (!b->waiting && !b->handler_due) {
            b->idle_point = false;
            return BOARD_INPUT_HANDLED;
        }
    }
    b->waiting = false;
    b->idle_point = false;
    b->handler_due = false;
    return BOARD_INPUT_FAILED;
}

/* BBC bit,ICR2 to itself ($B372 BBC3, $B37F BBC5): one read of ICR2; set, the loop is over. */
static int bbc_icr2(board *b, void *ctx, bool *done)
{
    unsigned bit = *(const unsigned *)ctx;
    *done = ((board_read_sfr(b, BOARD_ICR2) >> bit) & 1) != 0;
    return *done ? BOARD_BBC_EXIT : BOARD_BBC_PASS;
}

bool board_wait_timer(board *b, unsigned bit, uint64_t end)
{
    bool i = b->i, ok = true, done = false;
    b->i = true;   /* inside isr_timer_start's PHP / SEI */
    while (ok && !done) {
        ok = b->cycles < end;
        if (ok) {
            board_prepare(b);
            ok = exec(b, bbc_icr2, &bit, &done);
        }
    }
    b->i = i;
    return ok;
}

/* ser_tx_wait $C500: BBC1 UART_STATUS to itself (one read), then RTS ($C503). ctx: true once at the RTS. */
static int ser_tx_wait(board *b, void *ctx, bool *done)
{
    bool *at_rts = ctx;
    if (*at_rts) {
        *done = true;
        return BOARD_RTS_CYCLES;
    }
    *at_rts = ((board_read_sfr(b, BOARD_UART_STATUS) >> 1) & 1) != 0;
    return *at_rts ? BOARD_BBC_EXIT : BOARD_BBC_PASS;
}

bool board_wait_uart_tx(board *b, uint64_t end)
{
    bool at_rts = false, done = false;
    while (b->cycles < end) {
        board_prepare(b);
        if (!exec(b, ser_tx_wait, &at_rts, &done))
            break;
        if (done)
            return true;   /* the RTS */
    }
    b->handler_due = false;
    return false;
}

void board_print_fault(const board *b, FILE *out)
{
    unsigned long long cycles = (unsigned long long)b->cycles;
    switch (b->fault) {
    case BOARD_FAULT_NONE:
        fprintf(out, "no firmware fault after %llu cycles\n", cycles);
        break;
    case BOARD_FAULT_DME:
        fprintf(out, "firmware fault: DME (data-memory strobe) enabled after %llu cycles\n", cycles);
        break;
    case BOARD_FAULT_ESCAPED:
        fprintf(out, "firmware fault: code run from a deselected ROM bank after %llu cycles\n", cycles);
        break;
    case BOARD_FAULT_VECTOR:
        fprintf(out, "firmware fault: unmapped vector $%04X after %llu cycles\n", b->fault_addr, cycles);
        break;
    case BOARD_FAULT_INPUT_OVERFLOW:
        fprintf(out, "firmware fault: input longer than the serial queue after %llu cycles\n", cycles);
        break;
    case BOARD_FAULT_FIRMWARE:
        fprintf(out, "firmware fault at $%04X after %llu cycles\n", b->fault_addr, cycles);
        break;
    }
}

void board_print_unmodelled(const board *b, FILE *out)
{
    for (unsigned i = 0; i < b->unmodelled_count; i++)
        fprintf(out, "unmodelled %s $%04X\n", b->unmodelled[i].write ? "write" : "read", b->unmodelled[i].addr);
    if (b->unmodelled_dropped)
        fprintf(out, "unmodelled log full: %u more accesses not listed\n", b->unmodelled_dropped);
}
