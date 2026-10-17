#ifndef BOARD_BOARD_H
#define BOARD_BOARD_H
/* The CPU-less board (M5a spec 3.2-3.4): what the machine (tests/oracle/machine/machine.c) holds outside the CPU,
   with the machine's rules, so that the native host can give the C firmware everything the emulator gives it and the
   two can be compared field by field (diff_native). Time passes only in the waits, which charge exactly the cycles the
   all-C hybrid's emulated CPU spends in them (cpu740.c): a WIT step 4, an interrupt entry 7, the loops' instructions
   at their cycles. Each step mirrors machine_step: prepare (serial delivery at the idle point, then the IRQ lines),
   act (a WIT step, an interrupt entry, the handler of a taken interrupt in 0 cycles, or the instruction the wait
   mirrors), then the tick of the peripheral timers through the timer scale. Nothing here includes a machine/ or
   cpu740/ header. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "board/m50734.h"

#define BOARD_RAM_SIZE        0x4000   /* $0000-$3FFF; $00DA-$00FF are SFRs instead (MACHINE_RAM_SIZE) */
#define BOARD_QUEUE_SIZE      4096     /* the serial input ring (MACHINE_QUEUE_SIZE) */
#define BOARD_MAX_UNMODELLED  64       /* MACHINE_MAX_UNMODELLED */
#define BOARD_FRAME_BYTES     20       /* APOLLO_FRAME_BYTES */
#define BOARD_PRODUCER_PTR    0x2C     /* APOLLO_PRODUCER_PTR: the zero-page pointer to the frame the main loop filled */
#define BOARD_P2_BANK_MASK    0x7C     /* P2 bits 2-6: the selected ROM bank ... */
#define BOARD_P2_MAIN_BANK    0x70     /* ... the main ROM's */
#define BOARD_P2_DSP_RESET    0x01     /* P2.0: the DSP reset line (a rising edge restarts the DSP at word 0) */
#define BOARD_P0_BUSY         0x20     /* P0.5: the busy line to the host */
#define BOARD_ICR2_T3_ENABLE  0x10     /* ICR2 bit 4: timer 3's interrupt (the DSP tick) */
#define BOARD_ICR2            0xFE     /* the SFR the timer loops test */
#define BOARD_UART_STATUS     0xE7     /* the SFR the transmit loop tests */

/* The cycles of cpu740.c the waits charge (its k_mode_cycles and per-instruction adjustments) */
#define BOARD_WIT_CYCLES      4        /* a waiting CPU with no line raised */
#define BOARD_IRQ_CYCLES      7        /* an interrupt entry (take_irq) */
#define BOARD_BBC_EXIT        5        /* BBC n,zp,rel (AM_BIT_ZP_REL) with the bit set: not taken */
#define BOARD_BBC_PASS        7        /* ... with the bit clear: taken, 2 more */
#define BOARD_RTS_CYCLES      6

/* The ROM addresses the board stands for */
#define BOARD_VEC_FIRST       0xFFF4   /* the vector table: line n's at $FFFC - 2n ... */
#define BOARD_VEC_RESET       0xFFFE   /* ... and the reset vector */
#define BOARD_EMPTY_VECTOR    0xFFFF   /* a vector read with another bank selected (an empty socket reads $FF) */

struct board;
struct fw;   /* the firmware context the handlers run on (firmware/fw.h) */

typedef void (*board_frame_fn)(void *user, const uint8_t frame[BOARD_FRAME_BYTES]);
typedef struct { uint16_t addr; bool write; } board_access;

/* How the board stopped (machine_fault's board-side counterparts; M5a spec 3.6): the DME strobe (MFAULT_DME); code
   run from a deselected bank (MFAULT_ESCAPED_PC: a wait, a handler or an entry while P2 selects another bank than the
   main ROM); a handler address with no C (an unmapped vector, fault_addr = the address); input longer than the serial
   queue (MFAULT_INPUT_OVERFLOW); the firmware C (MFAULT_FIRMWARE: a read outside the converted data, a failed ROM call,
   fault_addr = where). */
typedef enum { BOARD_FAULT_NONE, BOARD_FAULT_DME, BOARD_FAULT_ESCAPED, BOARD_FAULT_VECTOR, BOARD_FAULT_INPUT_OVERFLOW,
               BOARD_FAULT_FIRMWARE } board_fault;

/* The idle handler of the input wait (machine_idle_fn): it queued more input, or the run ends there. */
typedef enum { BOARD_STOP = 0, BOARD_FEED } board_idle_action;
typedef board_idle_action (*board_idle_fn)(struct board *b, void *user);
/* How board_wait_input ended (machine_input_result). */
typedef enum { BOARD_INPUT_FAILED = 0, BOARD_INPUT_HANDLED, BOARD_INPUT_STOPPED } board_input_result;

typedef struct board {
    uint8_t ram[BOARD_RAM_SIZE];           /* the firmware's RAM (f->ram); the SFR bytes $DA-$FF are not data */
    m50734 sfr;
    bool i;                                /* the CPU's I flag */
    bool waiting;                          /* the CPU at a WIT ... */
    bool idle_point;                       /* ... the idle WIT $C602, waiting at $C603: serial bytes are delivered */
    bool handler_due;                      /* an interrupt was taken: its handler runs at the next step ... */
    bool handler_i;                        /* ... and its RTI restores this I flag (the P the entry pushed) */
    uint16_t handler;                      /* the handler's address, read from the vector */
    uint32_t irq_lines;                    /* the lines as the last prepare saw them (bit n = line n) */
    uint64_t cycles;                       /* the CPU cycles spent (cpu.cycles) */
    uint32_t cycle_num, cycle_den;         /* the peripheral timer scale (machine_config: TEST-ONLY), never 0 */
    uint64_t cycle_rem;                    /* remainder of the scaled ticks, < cycle_den */
    board_fault fault;
    uint16_t fault_addr;                   /* BOARD_FAULT_VECTOR, BOARD_FAULT_FIRMWARE: the address */
    uint8_t queue[BOARD_QUEUE_SIZE];       /* serial input ring buffer */
    size_t q_head, q_tail;
    bool busy_level_known;
    uint8_t busy_idle_level;               /* P0.5 level at the first idle = "not busy" */
    board_frame_fn on_frame;               /* the frame event (board_capture_frame) */
    void *user;
    board_frame_fn on_raw;                 /* raw DSP-port frames */
    void *raw_user;
    uint8_t raw_buf[BOARD_FRAME_BYTES];
    unsigned raw_pos;                      /* bytes of the current raw frame received so far */
    bool dsp_reset_level;                  /* last P2.0 pin level */
    uint64_t raw_frames;                   /* complete raw frames delivered */
    uint64_t dsp_resets;                   /* P2.0 rising edges */
    uint64_t raw_discarded;                /* partial-frame bytes dropped at a DSP reset */
    uint64_t frames_produced;
    uint64_t port_writes;                  /* bytes written to the DSP port */
    board_access unmodelled[BOARD_MAX_UNMODELLED];
    unsigned unmodelled_count;
    unsigned unmodelled_dropped;           /* accesses not logged because the log was full (as the machine counts) */
    const uint8_t *rom;                    /* the converted image by CPU address (fw_image.rom): the vectors; NULL: none */
    struct fw *fw;                         /* the firmware a handler runs on; NULL: a handler is an unmapped vector */
} board;

/* machine_init without the CPU and the ROM: everything zeroed, m50734_reset with the pins, the P2.0 level, I set,
   0 cycles; the timer scale num/den (0 means 1). rom and fw stay NULL. */
void board_init(board *b, const uint8_t port_in[5], uint32_t cycle_num, uint32_t cycle_den);
void board_set_outputs(board *b, board_frame_fn on_frame, void *user);
void board_set_raw_output(board *b, board_frame_fn on_raw, void *user);
/* Queues serial input (machine_feed_serial): false when it does not fit (the bytes before stay queued). */
bool board_feed_serial(board *b, const uint8_t *bytes, size_t n);
/* An SFR ($DA-$FF) read or write as the machine's bus does it: an unmodelled register (or a read of write-only
   UART_TX) logged; a write that enables the data-memory strobe is BOARD_FAULT_DME; a rising edge of P2.0 counts a DSP
   reset and drops the partial raw frame. */
uint8_t board_read_sfr(board *b, uint8_t addr);
void board_write_sfr(board *b, uint8_t addr, uint8_t v);
/* A byte to the DSP port $8000: counted, grouped into raw frames of 20. */
void board_dsp_write(board *b, uint8_t v);
/* The frame event: the 20 bytes at the BOARD_PRODUCER_PTR pointer to the frame callback, frames_produced counted. */
void board_capture_frame(board *b);
/* A byte as the CPU reads it (machine_peek): RAM, an SFR 0, else the converted image under the current P2 ($FF for
   another bank, for $4000-$7FFF under the lower window (P2 bit 7), or without an image). */
uint8_t board_peek(const board *b, uint16_t addr);
/* P2 selects the main ROM. */
bool board_main_bank(const board *b);
/* prepare: serial delivery at the idle point (one byte, the UART empty, P0.5 at the first idle's level), then the
   IRQ lines from m50734_irq_lines. */
void board_prepare(board *b);
/* After prepare, the machine has nothing left to do: at the idle point, no queued byte, the UART empty, timer 3 off and
   no line raised. */
bool board_idle(const board *b);
/* The vector at addr (BOARD_VEC_FIRST-BOARD_VEC_RESET) as the CPU reads it: from the image while the main ROM is
   selected, else BOARD_EMPTY_VECTOR. */
uint16_t board_vector(const board *b, uint16_t addr);
/* The dispatch table (M5a spec 3.4): true when the C has a function at addr. */
bool board_has_handler(uint16_t addr);
/* Runs the C function of addr on b->fw with A = a and X = x (the window and the bank synced from the P2 pins first,
   as a hybrid thunk does): an interrupt handler, the reset or a bank entry. False, with BOARD_FAULT_VECTOR at addr,
   when the table has none (or b->fw is NULL). */
bool board_call(board *b, uint16_t addr, uint8_t a, uint8_t x);
/* The waits (machine.c), each bounded by end (the cycle count by which it must be over), false on a fault or when end
   comes first:
   - board_wait_interrupt (machine_wait_interrupt): a WIT until one interrupt has been handled; a line that wakes the
     CPU with I set fails it; no byte is delivered.
   - board_wait_input (machine_wait_input): the same at the idle point, where bytes are delivered; whenever
     board_idle holds, idle (NULL: none, a stop) feeds (the wait goes on) or stops (BOARD_INPUT_STOPPED, the board
     left waiting at the idle point).
   - board_wait_timer (machine_run_loop of $B372 / $B37F): BBC bit,ICR2 to itself with I set, pass by pass, until
     the bit reads set; I restored.
   - board_wait_uart_tx (machine_rom_call of $C500): BBC1 UART_STATUS to itself, then its RTS, interrupts live. */
bool board_wait_interrupt(board *b, uint64_t end);
board_input_result board_wait_input(board *b, uint64_t end, board_idle_fn idle, void *user);
bool board_wait_timer(board *b, unsigned bit, uint64_t end);
bool board_wait_uart_tx(board *b, uint64_t end);
/* The CLI's reports (M5b spec 3.2); the board has no CPU, so they name no PC.
   - board_print_fault: one line, the fault with its address where it has one, and the cycles spent:
     BOARD_FAULT_FIRMWARE   "firmware fault at $A893 after N cycles" (the NUL bug: fault_addr)
     BOARD_FAULT_DME        "firmware fault: DME (data-memory strobe) enabled after N cycles"
     BOARD_FAULT_ESCAPED    "firmware fault: code run from a deselected ROM bank after N cycles"
     BOARD_FAULT_VECTOR     "firmware fault: unmapped vector $D300 after N cycles" (fault_addr)
     BOARD_FAULT_INPUT_OVERFLOW "firmware fault: input longer than the serial queue after N cycles"
     BOARD_FAULT_NONE       "no firmware fault after N cycles"
   - board_print_unmodelled: machine_print_unmodelled without the PC: "unmodelled read $00DA" or
     "unmodelled write $00DA" per logged access, then "unmodelled log full: N more accesses not listed" when accesses
     were dropped. */
void board_print_fault(const board *b, FILE *out);
void board_print_unmodelled(const board *b, FILE *out);

#endif
