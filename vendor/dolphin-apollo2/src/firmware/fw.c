#include "fw.h"
#include <stddef.h>

void fw_init(fw *f, uint8_t *ram, const fw_image *img, const fw_host *host)
{
    f->ram = ram;
    f->img = img;
    f->host = host;
    f->low_window = false;
    f->bank = P2_MAIN_BANK;
    f->fault = false;
    f->fault_addr = 0;
    f->trap = NULL;
    f->unwound = false;
    f->stopped = false;
    f->tn_flushes = 0;
}

static void fault(fw *f, uint16_t addr)
{
    if (!f->fault) {
        f->fault = true;
        f->fault_addr = addr;
    }
    if (f->trap)
        longjmp(*f->trap, 1);
}

static bool converted(const uint8_t *ok, uint32_t i) { return (ok[i >> 3] >> (i & 7)) & 1; }

/* An SFR address, $DA-$FF: a 16-bit addr above $FF is none. */
static bool is_sfr(uint16_t addr) { return addr >= SFR_FIRST && addr <= 0xFF; }

uint8_t fw_rom8(fw *f, uint16_t addr)
{
    const fw_image *img = f->img;
    if (addr >= 0x8000 && f->bank != P2_MAIN_BANK) {   /* another bank's socket (M4.6c1 spec 3.1) */
        if (f->host && f->host->read_bank)
            return f->host->read_bank(f->host->ctx, addr);
        fault(f, addr);
        return 0xFF;
    }
    if (img && addr >= 0x4000) {
        if (f->low_window && addr < 0x8000) {
            if (converted(img->low_ok, addr - 0x4000u))
                return img->low[addr - 0x4000u];
        } else if (converted(img->rom_ok, addr)) {
            return img->rom[addr];
        }
    }
    fault(f, addr);
    return 0xFF;
}

uint8_t fw_mem8(fw *f, uint16_t addr)
{
    if (addr >= 0x4000)
        return fw_rom8(f, addr);
    if (is_sfr(addr)) {
        fault(f, addr);
        return 0xFF;
    }
    return f->ram[addr];
}

uint8_t fw_ind_y(fw *f, uint8_t zp, uint8_t y)
{
    return fw_mem8(f, (uint16_t)(fw_rd16(f, zp) + y));
}

void fw_wr_ind_y(fw *f, uint8_t zp, uint8_t y, uint8_t v)
{
    uint16_t addr = (uint16_t)(fw_rd16(f, zp) + y);
    if (addr >= FW_RAM_SIZE || is_sfr(addr)) {
        fault(f, addr);
        return;
    }
    f->ram[addr] = v;
}

uint8_t fw_sfr8(fw *f, uint8_t addr)
{
    if (is_sfr(addr) && f->host && f->host->read_sfr)
        return f->host->read_sfr(f->host->ctx, addr);
    fault(f, addr);
    return 0xFF;
}

/* A value written to P2: the window cache follows its bit 7 (M4.6b spec 3.4), the selected bank its bits 2-6
   (M4.6c1 spec 3.1). */
static void p2_written(fw *f, uint8_t addr, uint8_t v)
{
    if (addr == SFR_P2) {
        f->low_window = (v & P2_LOW_WINDOW) != 0;
        f->bank = v & P2_BANK_BITS;
    }
}

void fw_wr_sfr8(fw *f, uint8_t addr, uint8_t v)
{
    if (is_sfr(addr) && f->host && f->host->write_sfr) {
        f->host->write_sfr(f->host->ctx, addr, v);
        p2_written(f, addr, v);
        return;
    }
    fault(f, addr);
}

/* SEB/CLB: one read, then one write of the changed value. */
static void sfr_rmw(fw *f, uint8_t addr, uint8_t set, uint8_t clear)
{
    if (is_sfr(addr) && f->host && f->host->read_sfr && f->host->write_sfr) {
        uint8_t v = (uint8_t)((f->host->read_sfr(f->host->ctx, addr) | set) & ~clear);
        f->host->write_sfr(f->host->ctx, addr, v);
        p2_written(f, addr, v);
        return;
    }
    fault(f, addr);
}

void fw_sfr_seb(fw *f, uint8_t addr, unsigned bit) { sfr_rmw(f, addr, (uint8_t)(1u << bit), 0); }
void fw_sfr_clb(fw *f, uint8_t addr, unsigned bit) { sfr_rmw(f, addr, 0, (uint8_t)(1u << bit)); }
bool fw_sfr_bit(fw *f, uint8_t addr, unsigned bit) { return ((fw_sfr8(f, addr) >> bit) & 1) != 0; }

void fw_wait(fw *f, fw_wait_kind kind, uint16_t at)
{
    fw_wait_result r = f->host && f->host->wait ? f->host->wait(f->host->ctx, kind, at) : FW_WAIT_FAILED;
    if (r == FW_WAIT_RETURNED)
        return;
    if (r == FW_WAIT_STOPPED) {
        f->stopped = true;   /* the run ends here: the C is abandoned, as at an unwound ROM call */
        if (f->trap)
            longjmp(*f->trap, 1);
        return;
    }
    fault(f, at);
}

void fw_dsp_write(fw *f, uint8_t v)
{
    if (f->host && f->host->dsp_write) {
        f->host->dsp_write(f->host->ctx, v);
        return;
    }
    fault(f, FW_DSP_DATA);
}

void fw_frame_done(fw *f)
{
    if (f->host && f->host->frame_done) {
        f->host->frame_done(f->host->ctx);
        return;
    }
    fault(f, FW_FRAME_EVENT);
}

void fw_fault(fw *f, uint16_t addr) { fault(f, addr); }

void fw_set_low_window(fw *f, bool on)
{
    f->low_window = on;
    if (f->host && f->host->set_low_window)
        f->host->set_low_window(f->host->ctx, on);
}

typedef fw_call_result (*rom_call_fn)(void *ctx, uint16_t addr, fw_regs *regs, uint64_t budget);

/* fw_rom_call through the host call call (rom_call or rom_call_unhooked; NULL: none). */
static bool call_rom(fw *f, rom_call_fn call, uint16_t addr, fw_regs *regs, uint64_t budget)
{
    fw_call_result r = call ? call(f->host->ctx, addr, regs, budget) : FW_CALL_FAILED;
    if (r == FW_CALL_RETURNED)
        return true;
    if (r == FW_CALL_UNWOUND) {
        f->unwound = true;   /* the ROM abandoned this call chain, and so does the C */
        if (f->trap)
            longjmp(*f->trap, 1);
        return false;
    }
    fault(f, addr);
    return false;
}

bool fw_rom_call(fw *f, uint16_t addr, fw_regs *regs, uint64_t budget)
{
    return call_rom(f, f->host ? f->host->rom_call : NULL, addr, regs, budget);
}

bool fw_rom_call_unhooked(fw *f, uint16_t addr, fw_regs *regs, uint64_t budget)
{
    return call_rom(f, f->host ? f->host->rom_call_unhooked : NULL, addr, regs, budget);
}

void fw_enter(fw *f, uint16_t addr, uint8_t a, uint8_t x)
{
    if (!f->host || !f->host->enter) {
        fault(f, addr);
        return;
    }
    f->host->enter(f->host->ctx, addr, a, x);
    f->unwound = true;   /* the C is abandoned as at an unwound ROM call (M4.2 spec 3) */
    if (f->trap)
        longjmp(*f->trap, 1);
}

void fw_reset_cpu(fw *f)
{
    if (f->host && f->host->reset_cpu) {
        f->host->reset_cpu(f->host->ctx);
        return;
    }
    fault(f, FW_RESET_CPU);
}

void fw_set_interrupts(fw *f, bool enabled, uint16_t at)
{
    if (f->host && f->host->set_interrupts) {
        f->host->set_interrupts(f->host->ctx, enabled);
        return;
    }
    fault(f, at);
}
