#include <setjmp.h>
#include <string.h>
#include "engine/native_engine.h"

#ifdef _MSC_VER
#pragma warning(disable : 4611)   /* setjmp with no C++ objects to destroy: the trap of fw faults */
#endif

void native_default_config(native_config *cfg)
{
    /* Switches off (serial mode); P2 pulled to $70 so the main ROM is selected at reset. */
    static const uint8_t k_default[5] = { 0x00, 0x00, 0x70, 0x00, 0x00 };
    memcpy(cfg->port_in, k_default, sizeof k_default);
    cfg->cycle_num = cfg->cycle_den = 1;
}

/* One run: its engine, its frame sinks, its input and watch, the idle handler's count of idles and whether it ended
   the run. */
typedef struct {
    native_engine *e;
    frame_list *produced, *raw;
    bool produced_on, raw_on;
    const engine_input *in;
    const native_watch *watch;
    unsigned idles;
    bool stopped;
} native_run;

_Static_assert(BOARD_FRAME_BYTES == ENGINE_FRAME_BYTES, "a board frame is an engine frame");

/* The board's frame callback: the watch, then the produced-frame sink. */
static void on_produced(void *user, const uint8_t frame[BOARD_FRAME_BYTES])
{
    native_run *r = user;
    if (r->watch && r->watch->on_frame)
        r->watch->on_frame(r->watch->user, &r->e->b);
    if (r->produced_on && r->watch && r->watch->on_output
        && !r->watch->on_output(r->watch->user, frame)) {
        r->stopped = true;
        r->e->h.f.stopped = true;
        return;
    }
    if (r->produced_on && r->produced)
        frame_list_add(r->produced, frame);
}

static void on_raw(void *user, const uint8_t frame[BOARD_FRAME_BYTES])
{
    native_run *r = user;
    if (r->raw_on)
        frame_list_add(r->raw, frame);
}

/* The ROM engine's idle handler (rom_engine.c on_idle): the watch first; at the first idle (the end of boot) the
   counters reset, the sinks on, the input fed and the speaking phase's deadline set; at the next the run stops. */
static board_idle_action on_idle(board *b, void *user)
{
    native_run *r = user;
    if (r->watch && r->watch->on_idle)
        r->watch->on_idle(r->watch->user, b);
    if (r->idles++ > 0) {
        r->stopped = true;
        return BOARD_STOP;
    }
    b->frames_produced = 0;
    b->raw_frames = 0;
    b->dsp_resets = 0;
    b->raw_discarded = 0;
    r->produced_on = true;
    r->raw_on = true;
    if (!board_feed_serial(b, r->in->bytes, r->in->n) ||
        (r->in->cr && !board_feed_serial(b, (const uint8_t *)"\r", 1))) {
        b->fault = BOARD_FAULT_INPUT_OVERFLOW;   /* input longer than the serial queue */
        return BOARD_STOP;
    }
    r->e->h.deadline = b->cycles + NATIVE_SPEAK_CYCLES;
    return BOARD_FEED;
}

/* The entries (the hybrid's entry rows and run_frame): the reset vector's C first. A stopped wait ends the run; a bank
   entry (fw_enter) runs the entry's C afresh with the recorded A and X, at once when it is the entry that is running
   (a warm @=1 under bank_entry_warm), else as machine_run reaches the hooked address: not after a fault, not once the
   phase deadline has passed, and only while P2 selects the main ROM. An entry that returns, or any other end, is a
   firmware fault (at the C's fault address, or the entry's). */
static void run_entries(native_engine *e)
{
    board *b = &e->b;
    fw *f = &e->h.f;
    jmp_buf trap;
    uint16_t entry = board_vector(b, BOARD_VEC_RESET);
    uint8_t a = 0, x = 0;
    f->trap = &trap;
    for (;;) {
        e->entries++;
        e->h.entered = false;
        if (setjmp(trap) == 0) {
            if (!board_call(b, entry, a, x))
                break;   /* no C at the entry address: BOARD_FAULT_VECTOR */
        }
        if (f->stopped)
            break;
        if (f->unwound && e->h.entered) {
            f->unwound = false;
            e->unwinds++;
            if (e->h.entry != entry) {
                if (b->fault != BOARD_FAULT_NONE || b->cycles >= e->h.deadline)
                    break;
                if (!board_main_bank(b)) {
                    b->fault = BOARD_FAULT_ESCAPED;
                    break;
                }
            }
            entry = e->h.entry;
            a = e->h.entry_a;
            x = e->h.entry_x;
            continue;
        }
        if (b->fault == BOARD_FAULT_NONE) {
            b->fault = BOARD_FAULT_FIRMWARE;
            b->fault_addr = f->fault ? f->fault_addr : entry;
        }
        break;
    }
    f->trap = NULL;
}

engine_status native_engine_run_input(native_engine *e, const native_config *cfg, const engine_input *in,
                                      bool include_boot, frame_list *produced, frame_list *raw,
                                      const native_watch *watch)
{
    native_config def;
    if (!cfg) {
        native_default_config(&def);
        cfg = &def;
    }
    memset(e, 0, sizeof *e);
    board_init(&e->b, cfg->port_in, cfg->cycle_num, cfg->cycle_den);
    if (!native_host_init(&e->h, &e->b)) {
        e->b.fault = BOARD_FAULT_FIRMWARE;   /* the data image does not build */
        return ENGINE_FAULT;
    }
    native_run r = { e, produced, raw, include_boot, false, in, watch, 0, false };
    board_set_outputs(&e->b, on_produced, &r);
    if (raw)
        board_set_raw_output(&e->b, on_raw, &r);
    e->h.idle = on_idle;
    e->h.idle_user = &r;
    e->h.deadline = e->b.cycles + NATIVE_BOOT_CYCLES;
    run_entries(e);
    board_set_outputs(&e->b, NULL, NULL);   /* the sinks and the handler live on this stack frame */
    board_set_raw_output(&e->b, NULL, NULL);
    e->h.idle = NULL;
    e->h.idle_user = NULL;
    if (e->b.fault != BOARD_FAULT_NONE)
        return ENGINE_FAULT;
    return r.stopped ? ENGINE_IDLE : ENGINE_CYCLE_LIMIT;
}

engine_status native_engine_speak(native_engine *e, const native_config *cfg, const char *text, bool include_boot,
                                  frame_list *produced, frame_list *raw)
{
    engine_input in = { (const uint8_t *)text, strlen(text), true };
    return native_engine_run_input(e, cfg, &in, include_boot, produced, raw, NULL);
}
