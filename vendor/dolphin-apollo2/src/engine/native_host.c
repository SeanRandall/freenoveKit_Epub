#include "engine/native_host.h"
#include "firmware/data/fw_data.h"

/* sync_p2: the firmware's window and bank read back from the board's pins after a wait (an interrupt handler may have
   written P2). */
static void sync_p2(native_host *h)
{
    uint8_t p2 = m50734_port_pins(&h->b->sfr, 2);
    h->f.low_window = (p2 & P2_LOW_WINDOW) != 0;
    h->f.bank = p2 & P2_BANK_BITS;
}

/* The hybrid's rom_call, rom_call_unhooked: there is no ROM. The C faults at the address it called (the NUL fallback's
   $A893, cmd_play_event's unreachable call of an event vector). */
static fw_call_result host_rom_call(void *ctx, uint16_t addr, fw_regs *r, uint64_t budget)
{
    (void)ctx;
    (void)addr;
    (void)r;
    (void)budget;
    return FW_CALL_FAILED;
}

/* SEB7/CLB7 $F0: a read-modify-write of P2 through the board, as the hybrid host's through the bus. */
static void host_set_low_window(void *ctx, bool on)
{
    board *b = ((native_host *)ctx)->b;
    uint8_t v = board_read_sfr(b, SFR_P2);
    board_write_sfr(b, SFR_P2, (uint8_t)(on ? v | P2_LOW_WINDOW : v & ~P2_LOW_WINDOW));
}

static uint8_t host_read_sfr(void *ctx, uint8_t addr) { return board_read_sfr(((native_host *)ctx)->b, addr); }

static void host_write_sfr(void *ctx, uint8_t addr, uint8_t value)
{
    board_write_sfr(((native_host *)ctx)->b, addr, value);
}

/* A wait that did not end (wait_ended): at the phase deadline the run stops there; otherwise a fault. */
static fw_wait_result wait_ended(native_host *h)
{
    if (!h->deadline || h->b->fault != BOARD_FAULT_NONE || h->b->cycles < h->deadline)
        return FW_WAIT_FAILED;
    h->b->waiting = false;
    return FW_WAIT_STOPPED;
}

/* The hybrid's host_wait, each kind on the board (board.h) within the same bound: FW_WAIT_UART_TX at most
   FW_ROM_CALL_BUDGET and the cycles left before the deadline (rom_call), the others the cycles left before the
   deadline (FW_ROM_CALL_BUDGET without one); none once it has passed. The window and the bank are read back after the
   wait, as the hybrid does (not when it never started). */
static fw_wait_result host_wait(void *ctx, fw_wait_kind kind, uint16_t at)
{
    native_host *h = ctx;
    board *b = h->b;
    (void)at;
    uint64_t budget = kind == FW_WAIT_UART_TX || !h->deadline ? FW_ROM_CALL_BUDGET : UINT64_MAX;
    if (h->deadline) {
        if (b->cycles >= h->deadline)
            return wait_ended(h);
        if (budget > h->deadline - b->cycles)
            budget = h->deadline - b->cycles;
    }
    uint64_t end = b->cycles + budget;
    bool ok;
    switch (kind) {
    case FW_WAIT_UART_TX:
        ok = board_wait_uart_tx(b, end);
        break;
    case FW_WAIT_INTERRUPT:
        ok = board_wait_interrupt(b, end);
        break;
    case FW_WAIT_TIMER2:
        ok = board_wait_timer(b, 3, end);
        break;
    case FW_WAIT_TIMER3:
        ok = board_wait_timer(b, 5, end);
        break;
    case FW_WAIT_INPUT: {
        board_input_result r = board_wait_input(b, end, h->idle, h->idle_user);
        sync_p2(h);
        if (r == BOARD_INPUT_STOPPED)
            return FW_WAIT_STOPPED;   /* at the idle point */
        return r == BOARD_INPUT_HANDLED ? FW_WAIT_RETURNED : wait_ended(h);
    }
    default:
        return FW_WAIT_FAILED;
    }
    sync_p2(h);
    return ok ? FW_WAIT_RETURNED : wait_ended(h);
}

static void host_dsp_write(void *ctx, uint8_t byte) { board_dsp_write(((native_host *)ctx)->b, byte); }

static void host_frame_done(void *ctx) { board_capture_frame(((native_host *)ctx)->b); }

/* Another bank's ROM byte: the option sockets are empty (parent spec 9), every byte reads $FF. */
static uint8_t host_read_bank(void *ctx, uint16_t addr)
{
    (void)ctx;
    (void)addr;
    return 0xFF;
}

/* A bank entry: recorded for the engine, which runs the entry's C afresh; the I flag set (bank_enter's SEI). */
static void host_enter(void *ctx, uint16_t addr, uint8_t a, uint8_t x)
{
    native_host *h = ctx;
    h->entered = true;
    h->entry = addr;
    h->entry_a = a;
    h->entry_x = x;
    h->b->i = true;
}

/* boot_reset's CPU set-up: of SEI, SP $FF, CLT, CLD, CLC and CLV only the I flag has a counterpart. */
static void host_reset_cpu(void *ctx) { ((native_host *)ctx)->b->i = true; }

/* CLI or SEI. */
static void host_set_interrupts(void *ctx, bool enabled) { ((native_host *)ctx)->b->i = !enabled; }

bool native_host_init(native_host *h, board *b)
{
    const fw_image *img = fw_image_get();
    h->b = b;
    h->deadline = 0;
    h->idle = NULL;
    h->idle_user = NULL;
    h->entered = false;
    h->entry = 0;
    h->entry_a = h->entry_x = 0;
    h->host.ctx = h;
    h->host.rom_call = host_rom_call;
    h->host.set_low_window = host_set_low_window;
    h->host.read_sfr = host_read_sfr;
    h->host.write_sfr = host_write_sfr;
    h->host.wait = host_wait;
    h->host.dsp_write = host_dsp_write;
    h->host.frame_done = host_frame_done;
    h->host.read_bank = host_read_bank;
    h->host.enter = host_enter;
    h->host.rom_call_unhooked = host_rom_call;
    h->host.reset_cpu = host_reset_cpu;
    h->host.set_interrupts = host_set_interrupts;
    fw_init(&h->f, b->ram, img, &h->host);
    b->rom = img ? img->rom : NULL;
    b->fw = &h->f;
    return img != NULL;
}
