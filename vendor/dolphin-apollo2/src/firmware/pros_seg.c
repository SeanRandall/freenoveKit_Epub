#include "pros_seg.h"
#include "fw_ops.h"

/* STA (ptr),Y: the 16-bit pointer plus y. The pointers start at the arrays ($0200, $0600, $0A00, $0E00) and only
   move up one page per 256 segments; a clause makes far fewer than 1,280 segments (at most 3 per phonetic symbol
   plus the pauses), so the address stays in RAM below $1300. */
static void seg_store(fw *f, uint16_t ptr, uint8_t y, uint8_t v) { fw_wr8(f, (uint16_t)(fw_rd16(f, ptr) + y), v); }

/* INC pros_seg_count / BNE; on the wrap to 0: INC pros_seg_count+1, then the pointers' high bytes in the ROM's
   order, phoneme, pitch, duration, flags ($CA02-$CA11, $CA2A-$CA39, $CA5D-$CA6C are the same code). */
static void seg_advance(fw *f)
{
    fw_inc8(f, PROS_SEG_COUNT);
    if (fw_rd8(f, PROS_SEG_COUNT) != 0)
        return;
    fw_inc8(f, PROS_SEG_COUNT + 1);
    fw_inc8(f, PROS_PHONEME_PTR + 1);
    fw_inc8(f, PROS_PITCH_PTR + 1);
    fw_inc8(f, PROS_DURATION_PTR + 1);
    fw_inc8(f, PROS_FLAGS_PTR + 1);
}

/* The pause segment of both pause routines ($C9ED-$CA11 is the same code as $CA15-$CA39): duration d, phoneme
   $3B (stored to ph_phoneme too), pitch pros_last_target, flags $80. Returns Y, the old index low byte. */
static uint8_t append_pause(fw *f, uint8_t d)
{
    uint8_t y = fw_rd8(f, PROS_SEG_COUNT);
    seg_store(f, PROS_DURATION_PTR, y, d);
    seg_store(f, PROS_PHONEME_PTR, y, PH_PAUSE_PHONEME);
    fw_wr8(f, PH_PHONEME, PH_PAUSE_PHONEME);
    seg_store(f, PROS_PITCH_PTR, y, fw_rd8(f, PROS_LAST_TARGET));
    seg_store(f, PROS_FLAGS_PTR, y, PROS_SEG_FLAGS_GEN);
    seg_advance(f);
    return y;
}

void pros_reset_seg_ptrs(fw *f)
{
    fw_wr16(f, PROS_PHONEME_PTR, PROS_SEG_PHONEME);
    fw_wr16(f, PROS_DURATION_PTR, PROS_SEG_DURATION);
    fw_wr16(f, PROS_PITCH_PTR, PROS_SEG_PITCH);
    fw_wr16(f, PROS_FLAGS_PTR, PROS_SEG_FLAGS);
}

void pros_append_segment(fw *f, uint8_t a, uint8_t x, uint8_t y)
{
    fw_par_ready(f, (fw_regs){ a, x, y, 0 });
    uint8_t i = fw_rd8(f, PROS_SEG_COUNT);
    seg_store(f, PROS_PHONEME_PTR, i, fw_rd8(f, PH_PHONEME));
    seg_store(f, PROS_DURATION_PTR, i, fw_rd8(f, PH_DURATION));
    uint8_t target = (uint8_t)(fw_rd8(f, PROS_TARGET) + fw_rd8(f, CMD_PITCH_STEP));   /* CLC; ADC: 8 bits */
    fw_wr8(f, PROS_LAST_TARGET, target);
    seg_store(f, PROS_PITCH_PTR, i, target);
    seg_store(f, PROS_FLAGS_PTR, i, PROS_SEG_FLAGS_GEN);
    seg_advance(f);
}

uint8_t ph_append_pause(fw *f, uint8_t a) { return append_pause(f, a); }

void ph_append_phrase_pause(fw *f)
{
    uint8_t d = fw_rd8(f, CMD_PHRASE_PAUSE);
    switch (fw_rd8(f, PROS_SYMBOL)) {
    case '.': case '?': case '!':
        break;                                  /* $C9E7: PLA, in full (no AND) */
    case ',': case ';': case ':':
        d = (uint8_t)((d >> 1) & 0x0F);         /* $C9E1: LSR; AND #$0F */
        break;
    default:
        d = (uint8_t)((d >> 2) & 0x0F);         /* $C9D9: LSR; LSR; AND #$0F */
        break;
    }
    if (d == 0)
        d++;                                    /* $C9EC INC A */
    append_pause(f, d);
}
