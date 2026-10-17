#ifndef FW_H
#define FW_H
/* Decompiled Apollo 2 firmware on the firmware's own memory image (M4.0 spec section 3).
   Nothing under src/firmware/ includes machine/ or cpu740/ headers: build.bat syntax-checks this directory
   without the src include path. */
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include "fw_ram.h"

#define FW_RAM_SIZE  0x4000
#define FW_TMP_78    0x0078   /* zero-page scratch, unnamed in the annotations: a saved Y, byte counters;
                                 par_scale_duration's saved Y ($C2A0; not written for a duration of 0);
                                 bank_load_ramcode's saved Y, which bank_restore reloads ($A601, $0258);
                                 boot_queue_string's Y around each ser_rx_put ($C620, $C625) */
#define FW_TMP_75    0x0075   /* zero-page scratch, unnamed in the annotations: tn_convert_clause's saved X;
                                 bank_load_ramcode's saved P2, which bank_restore writes back ($A607, $0252);
                                 isr_set_volume's inverted level << 3 ($ACEF);
                                 frm_clamp_mark_space's 2 x b10 ($B936), frm_raise_formants' (b10 - 32) / 4 ($B969)
                                 and frm_transition_flags' count of the rising amplitudes ($BAA7-$BAB7) */
#define FW_TMP_76    0x0076   /* zero-page scratch, unnamed in the annotations: cmd_scan_clause's resume index,
                                 bank_load_ramcode's saved X, which bank_restore reloads ($A603, $0256);
                                 cmd_set_prosody's parameter index; par_scale_voicing's first term at @B2, @B3, @B4,
                                 @B6 and @B7 ($BFE7, $BFF8, $C006/$C00D, $C022, $C030) */
#define FW_TMP_34    0x0034   /* zero-page scratch, unnamed in the annotations: cmd_hex_byte's high digit */
#define FW_TMP_35    0x0035   /* zero-page scratch, unnamed in the annotations: cmd_hex_byte's low digit */
#define FW_TMP_1809  0x1809   /* scratch, unnamed in the annotations: cmd_hex_byte's high nibble */
/* The scratch bytes of pros_scale_target $BDD3, unnamed in the annotations; no other routine uses them (M4.4a spec
   2.6). d = |pros_play_target - $80|. */
#define FW_PROS_SCALE_DEV    0x1811  /* the scaled deviation s added to (target >= $80) or subtracted from
                                        pros_pitch_centre ($BE13, $BE26, $BE31); not written when the limited R is 0
                                        ($BE52), written 0 for R = 0 below cmd_speed_limit */
#define FW_PROS_SCALE_FRAC   0x1812  /* the fraction accumulator of the R & 3 adds (0 at $BDE9); its bit 7 rounds s
                                        ($BE2C ASL) */
#define FW_PROS_DEV_FRAC     0x1813  /* d bits 1-0, in bits 7-6 (0 at $BDE6, rotated in at $BDF0) */
#define FW_PROS_DEV_QUARTER  0x1814  /* d >> 2 ($BDF6) */
#define FW_UTT_CONTENT         0x006B   /* unnamed in the annotations: "the utterance has content", $FF (LDM) at each
                                           word pause and each phoneme triple; boot_loop_play tests it */
#define FW_CLAUSE_CARRY        0x1827   /* unnamed in the annotations: a command character boot_get_clause cut off
                                           at a clause's end ($A739), put first in the next clause ($A6A3) and
                                           cleared; 0 when none. boot_set_defaults clears it ($A782) */
#define FW_PROS_STRESS_TOGGLE  0x1C0D   /* unnamed in the annotations: pros_phrase_gen's "a stress or an accent seen
                                           in this word" (0 at each word start). pros_mark_stress: the syllable's
                                           stress, $FF at its start, at each word end and at '}', 0 at '~', '{' and
                                           'x' ($CBC4, $CC06, $CC1E; on the vowel path $CD74 too), inverted ($CC45)
                                           at the onset of each later syllable of a word, and 0 ($CC5D) at a long
                                           symbol (features bit 1) while pros_accent_state is 0.
                                           pros_mark_phrases writes it (0 at its start and at each space or
                                           punctuation, 1 at '{') and never reads it */
#define FW_PROS_WORD_U         0x1C1F   /* unnamed in the annotations: the U term (pros_params[20]) that the first
                                           symbol of a word adds to pros_term_stress; 0 after it */
/* The state bytes of the stress and phrase passes (pros_mark_stress $CABD, pros_mark_phrases $CEF6 and
   pros_store_phrase_len $D0FD), unnamed in the annotations; no other routine uses them (M4.3b spec 2.6). */
#define FW_PROS_NEXT_SYM           0x1C06   /* pros_mark_stress: the next symbol after pros_pos that has a class
                                               (class 0, the markers, skipped), or a space if one comes first; $0D
                                               when the scan reaches tn_phon_len (after a space too) */
#define FW_PROS_NEXT_FEATURES      0x1C07   /* pros_mark_stress: the features of FW_PROS_NEXT_SYM (0 for a space and at
                                               the end) */
#define FW_PROS_NEXT_PHON          0x1C08   /* pros_mark_stress: the next symbol that has a class, past a space; $0D at
                                               the end. The vowel path replaces it with tn_phonetic[pros_pos + 2]
                                               when it is '1' or 'L' ($CDBF) */
#define FW_PROS_NEXT_PHON_FEATURES 0x1C09   /* pros_mark_stress: the features of FW_PROS_NEXT_PHON (0 at the end) */
#define FW_PROS_SYLLABLES          0x1C0B   /* pros_mark_stress: the syllables of the word so far (a vowel counts when
                                               a consonant or no features follow, $CD65), stored at the word's end
                                               in pros_sym_lengthen and pros_sym_shorten, then 0. pros_mark_phrases:
                                               that count, read back at each space and punctuation ($CF6B), of the
                                               word it walks next (the one before it in the text); 5 or more
                                               shortens each of its phonetic symbols by 4 ($D0B5) */
#define FW_PROS_VOWEL_SEEN         0x1C0C   /* pros_mark_stress: a vowel seen in this word (1 on the vowel path, 0 at
                                               its start ($CAD7) and at each word end) */
#define FW_PROS_TILDE_SHORTEN      0x1C11   /* pros_mark_stress: 1 at its start and at '~', 0 at each word end and
                                               while FW_PROS_ACCENT_PENDING is set ($CC81); while set, each symbol's
                                               shortening count gets 3 ($CC92). A space copies it into
                                               FW_PROS_ACCENT_PENDING ($CEA2) */
#define FW_PROS_WRITE_ONLY_1C12    0x1C12   /* pros_mark_stress, on the vowel path: 1 when FW_PROS_NEXT_PHON is '1' or
                                               'L' (stored at $CDCB), else 0 (stored at $CDD4); never read */
#define FW_PROS_FINAL              0x1C13   /* pros_mark_stress: the vowel's syllable ends a phrase (the class bit 0,
                                               punctuation, of the first non-consonant from pros_pos + 2 on, or from
                                               pros_pos + 3 when FW_PROS_NEXT_PHON is '1' or 'L' ($CDBA-$CDCE), $CDEA).
                                               pros_mark_phrases: walking back through a word's last syllable (1 at
                                               its start and at each space and punctuation, 0 at the consonant
                                               before a vowel or long symbol, $D079, tested at $D06E-$D075); a vowel
                                               outside it is shortened by 3 */
#define FW_PROS_BACKQUOTE          0x1C14   /* pros_mark_stress: a backquote seen ($CC12), so the next '?' becomes '.'
                                               ($CEB3); 0 at its start, at that '?' and at $CEC2 */
#define FW_PROS_PUNCT_NOT_KIND3    0x1C15   /* pros_store_phrase_len: the punctuation it stored at last (the next one
                                               in the text) had no class bit 3 (PHC_KIND_3 is set for . : ; ?), so
                                               this store gets no bit 7. Cleared by pros_mark_stress at its start
                                               ($CAE0), which pros_mark_phrases' entry relies on; then by each
                                               pros_store_phrase_len call ($D11B), which sets it to 1 ($D125) when
                                               pros_class has no bit 3 */
#define FW_PROS_FINAL_LENGTHEN     0x1C16   /* pros_mark_phrases: the phrase-final syllable's lengthening: 8
                                               (PHC_KIND_3) after a punctuation with class bit 3 ($D057); 0 at its
                                               start, at each space and punctuation, and at the consonant before a
                                               vowel or long symbol ($D07C). While set, each phonetic symbol's
                                               lengthening count gets 8 ($D084); while clear, a vowel or long
                                               symbol's shortening gets 7 ($D0A5) */
#define FW_PROS_ACCENT_PENDING     0x1C19   /* pros_mark_stress: lengthen the word's symbols by 4 ($CC84): 1 at its
                                               start and at each word end but a space, 0 at '~', and at a space
                                               FW_PROS_TILDE_SHORTEN (1 after a function word). pros_mark_phrases: an
                                               accent still to place: 1 at its start and at each '~'; the next
                                               stressed vowel or long symbol walked (features bit 7 or 1, $D0CB and
                                               $D0CD) gets the accent $60 and clears it ($D0E1) */
#define FW_PROS_PREV_FEATURES_COPY 0x1C1A   /* pros_mark_stress: pros_prev_features as stored at $CD58, but 0 at its
                                               start and at each word end ($CEDE), where pros_prev_features is kept:
                                               the previous symbol's features within the word ($CD81) */
#define FW_PROS_WRITE_ONLY_1C1C    0x1C1C   /* pros_mark_phrases: 1 at its start ($CF0F) and at each space and
                                               punctuation ($CF79); never read */
#define FW_PROS_NUCLEUS_MASK       0x1C1E   /* pros_mark_phrases: ORed into each phonetic symbol's pros_sym_flags
                                               ($D0E7): 7 at its start and at each '?' '!' ',' ':' '.' ';', 0 after
                                               the first symbol whose flags then have the stress bit $40 ($D0F7).
                                               pros_mark_stress clears it ($CAE3) */

/* Cycle budgets of a ROM call from C (M4.1 spec 3.1): a helper that does not return within FW_ROM_CALL_BUDGET
   is a fault; a call that plays speech (prosody and playback) gets FW_ROM_CALL_SPEECH_BUDGET. */
#define FW_ROM_CALL_BUDGET         10000000ULL
#define FW_ROM_CALL_SPEECH_BUDGET  1000000000ULL

typedef struct { uint8_t a, x, y, p; } fw_regs;

/* How a ROM call from C ended: returned; failed (a fault, or out of budget); or unwound: the ROM reset the
   stack past the call's own frame (bank_enter, M4.2 spec 3), so it can never return. */
typedef enum { FW_CALL_FAILED = 0, FW_CALL_RETURNED, FW_CALL_UNWOUND } fw_call_result;

/* The hardware waits (M4.6a spec 3.3, M4.6b spec 3.2): one kind per ROM wait loop. The hybrid host runs the loop on
   the emulated CPU, or idles it until an interrupt has been handled; a native host waits natively. */
typedef enum fw_wait_kind {
    FW_WAIT_UART_TX = 0,   /* until the UART can transmit: ser_tx_wait, BBC1 UART_STATUS until bit 1 is set */
    FW_WAIT_INTERRUPT,     /* a polling loop on a flag an interrupt sets: until an interrupt has been handled */
    FW_WAIT_TIMER2,        /* isr_timer_start's BBC3 ICR2 (inside SEI): until timer 2's request bit is set */
    FW_WAIT_TIMER3,        /* isr_timer_start's BBC5 ICR2 (inside SEI): until timer 3's request bit is set */
    FW_WAIT_INPUT,         /* isr_idle's WIT (M4.6c2 spec 3.1): until an interrupt has been handled, the machine at the
                              idle point, where serial bytes are delivered; there the host's idle handler may stop the
                              run */
} fw_wait_kind;
/* How a wait ended: FW_WAIT_RETURNED when it did; FW_WAIT_FAILED when it did not (a fault); FW_WAIT_STOPPED when the
   run ends there (M4.6c2 spec 3.1, 3.2): the input wait's idle handler stopped it, or an entry row's phase deadline
   passed. */
typedef enum { FW_WAIT_FAILED = 0, FW_WAIT_RETURNED, FW_WAIT_STOPPED } fw_wait_result;
#define FW_WAIT_UART_TX_LOOP  0xC500   /* ser_tx_wait: the ROM loop of FW_WAIT_UART_TX (BBC1 $E7,$C500 / RTS) */
#define FW_WAIT_TIMER2_LOOP   0xB372   /* isr_timer_start: BBC3 ICR2,$B372, the loop of FW_WAIT_TIMER2 ... */
#define FW_WAIT_TIMER2_EXIT   0xB375   /* ... and the instruction after it (LDM #$28,T3_RELOAD) */
#define FW_WAIT_TIMER3_LOOP   0xB37F   /* isr_timer_start: BBC5 ICR2,$B37F, the loop of FW_WAIT_TIMER3 ... */
#define FW_WAIT_TIMER3_EXIT   0xB382   /* ... and the instruction after it (LDM #$31,T3_RELOAD) */
#define FW_WAIT_INPUT_LOOP    0xC602   /* isr_idle: the WIT of FW_WAIT_INPUT, the firmware's only one */
#define FW_RESET_CPU          0xA625   /* boot_reset: the SEI of the CPU set-up, where fw_reset_cpu faults */
#define FW_DSP_DATA           0x8000   /* DSP_DATA, the DSP port (write only): where fw_dsp_write faults */
#define FW_FRAME_EVENT        0xB306   /* isr_swap_buffers: the ROM's point of the frame event, where fw_frame_done
                                          faults */
#define SFR_FIRST             0xDA     /* the SFRs are $DA-$FF: zero page there is not RAM */
#define SFR_P2                0xF0     /* port P2: bit 0 the DSP reset line, bit 1 the self-test pin, bits 2-6
                                          the bank, bit 7 the window */
#define P2_LOW_WINDOW         0x80     /* P2 bit 7: the lower-ROM window ($4000-$7FFF read the LOW overlay) */
#define P2_BANK_BITS          0x7C     /* P2 bits 2-6: the selected ROM bank (M4.6c1 spec 3.1) */
#define P2_MAIN_BANK          0x70     /* the main ROM's bank bits (boot_init_ports $AC50 LDM #$70,P2) */

/* The machine or native host. In hybrid runs it is the emulator (tests/oracle/engine/thunks.c). */
typedef struct fw_host {
    void *ctx;
    /* Runs the routine at addr, still in ROM, to its RTS, within budget cycles. A, X and Y in and out; the
       routine starts with the machine's current flags, and p returns its exit flags. */
    fw_call_result (*rom_call)(void *ctx, uint16_t addr, fw_regs *regs, uint64_t budget);
    /* P2 bit 7, the lower-ROM window, as SEB7/CLB7 $F0 set and clear it. */
    void (*set_low_window)(void *ctx, bool on);
    /* Reads an SFR ($DA-$FF) as the CPU's read would, side effects included (UART_RX $E5 clears receive full). */
    uint8_t (*read_sfr)(void *ctx, uint8_t addr);
    /* Writes an SFR ($DA-$FF) as the CPU's write would (M4.6a spec 3.1). */
    void (*write_sfr)(void *ctx, uint8_t addr, uint8_t value);
    /* Waits for the hardware (M4.6a spec 3.3, M4.6b spec 3.2, M4.6c2 spec 3.1); at is the ROM loop the wait stands
       for. */
    fw_wait_result (*wait)(void *ctx, fw_wait_kind kind, uint16_t at);
    /* Writes a byte to the DSP port DSP_DATA $8000, as isr_timer_dsp's STA DSP_DATA does (M4.6b spec 3.1). */
    void (*dsp_write)(void *ctx, uint8_t byte);
    /* The frame event: the C timer interrupt is where the ROM's reaches $B306 (isr_swap_buffers), the frame at
       isr_fill_ptr complete (M4.6b spec 3.1). */
    void (*frame_done)(void *ctx);
    /* A ROM byte at addr ($8000-$FFFF) as the CPU reads it while another bank than the main ROM is selected
       (M4.6c1 spec 3.1): an empty socket reads $FF. */
    uint8_t (*read_bank)(void *ctx, uint16_t addr);
    /* Leaves the C through a bank entry (bank_entry_cold $FFD0, bank_entry_warm $FFD4; M4.6c1 spec 3.2): the CPU
       goes on at addr with SP $FF, the I flag set and A and X as given, as bank_enter's LDX #$FF / TXS, SEI and
       JMP leave them. */
    void (*enter)(void *ctx, uint16_t addr, uint8_t a, uint8_t x);
    /* As rom_call, but the hook at addr does not fire on this entry (M4.6c1 spec 3.2): the NUL-bug fallback's
       ROM cmd_run_next. */
    fw_call_result (*rom_call_unhooked)(void *ctx, uint16_t addr, fw_regs *regs, uint64_t budget);
    /* boot_reset's CPU set-up $A625-$A62C (M4.6c2 spec 3.3): SEI, SP = $FF (LDX #$FF / TXS), CLT, CLD, CLC, CLV, so
       that the ROM code run later and the interrupts see the ROM's flags and SP. */
    void (*reset_cpu)(void *ctx);
    /* CLI (enabled) or SEI: the main loop's unpaired ones (M4.6c2 spec 3.3), on which the next wait's interrupts
       depend. */
    void (*set_interrupts)(void *ctx, bool enabled);
} fw_host;

/* Converted ROM data at its original addresses (built by src/firmware/data/fw_data.c). */
typedef struct fw_image {
    uint8_t rom[0x10000];          /* $4000-$FFFF with the window off; index = CPU address */
    uint8_t rom_ok[0x10000 / 8];   /* bit set = converted byte */
    uint8_t low[0x4000];           /* $4000-$7FFF with the window on; index = CPU address - $4000 */
    uint8_t low_ok[0x4000 / 8];
} fw_image;

typedef struct fw {
    uint8_t *ram;                  /* the firmware's RAM $0000-$3FFF (the SFR bytes $DA-$FF are not data) */
    const fw_image *img;
    const fw_host *host;           /* NULL: no ROM to call, no port to drive */
    bool low_window;               /* P2 bit 7: $4000-$7FFF read the LOW overlay */
    uint8_t bank;                  /* P2 bits 2-6 as last written (P2_MAIN_BANK: the main ROM; M4.6c1 spec 3.1) */
    bool fault;                    /* a read outside the converted data, or a failed ROM call */
    uint16_t fault_addr;
    jmp_buf *trap;                 /* when set, a fault longjmps here and the routine is abandoned */
    bool unwound;                  /* a ROM call reset the stack: the C was abandoned (not a fault; M4.2 spec 3) */
    bool stopped;                  /* a wait stopped the run: the C was abandoned (not a fault; M4.6c2 spec 3.1) */
    unsigned tn_flushes;           /* mid-clause flushes by tn_convert_clause (a statistic for the tests) */
} fw;

/* RAM by name; a < FW_RAM_SIZE. */
static inline uint8_t fw_rd8(const fw *f, uint16_t a) { return f->ram[a]; }
static inline void fw_wr8(fw *f, uint16_t a, uint8_t v) { f->ram[a] = v; }
static inline uint16_t fw_rd16(const fw *f, uint16_t a) { return (uint16_t)(f->ram[a] | f->ram[a + 1] << 8); }
static inline void fw_wr16(fw *f, uint16_t a, uint16_t v)
{
    f->ram[a] = (uint8_t)v;
    f->ram[a + 1] = (uint8_t)(v >> 8);
}

void fw_init(fw *f, uint8_t *ram, const fw_image *img, const fw_host *host);
/* A ROM data byte as the CPU reads it at addr (>= $4000) under the current window. A byte outside the
   converted data is a fault: it longjmps to f->trap when set, else reads $FF. While another bank than the main ROM
   is selected (f->bank), $8000-$FFFF are read through the host's read_bank; without a host or read_bank, a fault
   at addr (M4.6c1 spec 3.1). */
uint8_t fw_rom8(fw *f, uint16_t addr);
/* A data byte anywhere: RAM below $4000 (an SFR address is a fault), else fw_rom8. */
uint8_t fw_mem8(fw *f, uint16_t addr);
/* (zp),Y: the byte at the little-endian pointer in zero page zp, plus y. */
uint8_t fw_ind_y(fw *f, uint8_t zp, uint8_t y);
/* STA (zp),Y: writes v at the little-endian pointer in zero page zp, plus y. A target that is not RAM ($4000 and
   above, or an SFR $DA-$FF) is a fault and nothing is written. */
void fw_wr_ind_y(fw *f, uint8_t zp, uint8_t y, uint8_t v);
/* An SFR byte, read through the host (LDA, CMP and the like). Without a host or read_sfr, or for an address below
   $DA, a fault: it longjmps to f->trap when set, else reads $FF. */
uint8_t fw_sfr8(fw *f, uint8_t addr);
/* STA/LDM to an SFR: v written through the host. Without a host or write_sfr, or below $DA, a fault (nothing
   written). A write of P2 ($F0) sets f->low_window from bit 7 of v (M4.6b spec 3.4) and f->bank from bits 2-6
   (M4.6c1 spec 3.1). */
void fw_wr_sfr8(fw *f, uint8_t addr, uint8_t v);
/* SEB bit,addr / CLB bit,addr on an SFR: a read, then a write of the value with the bit set or cleared, as the CPU
   does them (cpu740.c, M_SEB/M_CLB): a port reads its pins, so an input pin's level reaches the latch. Without a
   host, read_sfr or write_sfr, or below $DA, a fault (nothing read or written). On P2 ($F0) f->low_window follows
   bit 7 of the value written (M4.6b spec 3.4) and f->bank its bits 2-6. */
void fw_sfr_seb(fw *f, uint8_t addr, unsigned bit);
void fw_sfr_clb(fw *f, uint8_t addr, unsigned bit);
/* BBS bit,addr / BBC bit,addr on an SFR: the bit of one read (fw_sfr8, its faults included). */
bool fw_sfr_bit(fw *f, uint8_t addr, unsigned bit);
/* Waits for the hardware through the host: the polling loop or wait loop at the ROM address at (JSR ser_tx_wait
   $C500 for FW_WAIT_UART_TX; the polling loop's own address for FW_WAIT_INTERRUPT; $B372 and $B37F for the timer
   waits; isr_idle's WIT $C602 for FW_WAIT_INPUT). Without a host or wait, or when the wait fails, a fault at at.
   When it stops the run, f->stopped is set and the routine is abandoned like an unwound ROM call (a longjmp to
   f->trap when set), but it is not a fault (M4.6c2 spec 3.1). */
void fw_wait(fw *f, fw_wait_kind kind, uint16_t at);
/* STA DSP_DATA: v written to the DSP port through the host. Without a host or dsp_write, a fault at FW_DSP_DATA. */
void fw_dsp_write(fw *f, uint8_t v);
/* The frame event through the host (M4.6b spec 3.1). Without a host or frame_done, a fault at FW_FRAME_EVENT. */
void fw_frame_done(fw *f);
/* A fault at addr where the ROM would never return (a loop with no exit), like a fault of fw_rom8: it sets f->fault
   (keeping the first fault_addr), then longjmps to f->trap when set, else returns. run_thunk relies on the flag being
   set after the longjmp. */
void fw_fault(fw *f, uint16_t addr);
/* SEB7/CLB7 P2: sets the window and tells the host. */
void fw_set_low_window(fw *f, bool on);
/* Calls a routine still in ROM through the host, within budget cycles. True when it returned. Without a host, or
   when the call fails, a fault. When the ROM reset the stack (unwound), f->unwound is set and the routine is
   abandoned like a fault (a longjmp to f->trap), but it is not a fault. */
bool fw_rom_call(fw *f, uint16_t addr, fw_regs *regs, uint64_t budget);
/* fw_rom_call through the host's rom_call_unhooked: the hook at addr does not fire on this entry (M4.6c1 spec 3.2).
   Without a host or rom_call_unhooked, or when the call fails, a fault at addr. */
bool fw_rom_call_unhooked(fw *f, uint16_t addr, fw_regs *regs, uint64_t budget);
/* Leaves the C through the bank entry addr ($FFD0 or $FFD4) with A = a and X = x (M4.6c1 spec 3.2): the host's
   enter, then the routine is abandoned like an unwound ROM call (f->unwound set, a longjmp to f->trap when set; it
   is not a fault). Without a host or enter, a fault at addr. */
void fw_enter(fw *f, uint16_t addr, uint8_t a, uint8_t x);
/* boot_reset's CPU set-up through the host's reset_cpu (M4.6c2 spec 3.3). Without a host or reset_cpu, a fault at
   FW_RESET_CPU $A625. */
void fw_reset_cpu(fw *f);
/* CLI (enabled) or SEI at the ROM address at through the host's set_interrupts (M4.6c2 spec 3.3). Without a host or
   set_interrupts, a fault at at. */
void fw_set_interrupts(fw *f, bool enabled, uint16_t at);

#endif
