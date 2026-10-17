#ifndef FW_OPS_H
#define FW_OPS_H

/* Small steps the ported routines share (M4.4a spec 2.6), and the two shift routines (M4.4b spec 2.2). */

#include "fw.h"
#include "isr_serial.h"

/* JSR isr_par_ready with the registers the ROM has at the JSR (it preserves A, X and Y): the C (M4.6b). */
static inline void fw_par_ready(fw *f, fw_regs r) { isr_par_ready(f, &r); }

/* ADC: a + m + the carry in *c; the carry out back in *c. */
static inline uint8_t fw_adc(uint8_t a, uint8_t m, uint8_t *c)
{
    unsigned s = (unsigned)a + m + *c;
    *c = (uint8_t)(s >> 8);
    return (uint8_t)s;
}

/* INC addr: mod 256. */
static inline void fw_inc8(fw *f, uint16_t addr) { fw_wr8(f, addr, (uint8_t)(fw_rd8(f, addr) + 1)); }

/* SEB bit,zp / CLB bit,zp on a RAM byte (M4.6a): RAM only, with no SFR check (fw_rd8/fw_wr8); an SFR goes through
   fw_sfr_seb/fw_sfr_clb (fw.h). */
static inline void fw_seb(fw *f, uint16_t addr, unsigned bit)
{
    fw_wr8(f, addr, (uint8_t)(fw_rd8(f, addr) | 1u << bit));
}
static inline void fw_clb(fw *f, uint16_t addr, unsigned bit)
{
    fw_wr8(f, addr, (uint8_t)(fw_rd8(f, addr) & ~(1u << bit)));
}

/* LDA arr,X; CLC; ADC #n; STA arr,X: an 8-bit add into an array byte. */
static inline void fw_add8(fw *f, uint16_t arr, uint8_t x, uint8_t n)
{
    fw_wr8(f, (uint16_t)(arr + x), (uint8_t)(fw_rd8(f, (uint16_t)(arr + x)) + n));
}

/* util_shl3 $C657: ASL A three times, A * 8 (8-bit). */
static inline uint8_t util_shl3(uint8_t a) { return (uint8_t)(a << 3); }
/* util_shl4 $C656: one ASL, then falls into util_shl3: A * 16 (8-bit). No caller reads the carry (M4.4b spec 2.6). */
static inline uint8_t util_shl4(uint8_t a) { return util_shl3((uint8_t)(a << 1)); }

#endif
