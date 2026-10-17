#ifndef FW_FRM_BUILD_H
#define FW_FRM_BUILD_H
/* The frame builder (ROM $B41D-$B44F, $B917-$B982 and $B9B5-$BAEB), decompiled (M4.5 spec 2.2, 2.6): the rest
   frame that par_setup_voice builds and copies into both frame buffers, and frm_finish_frame, which finishes each
   producer frame (the frame at isr_fill_ptr) after the tracks: the snapshot of its parameter bytes, the mark-space
   clamp of b11, the formant raise at high pitch codes, the source and filter codes (b16, b17), the b14-b15 word and
   the transition flags (b13). Each routine takes the registers at entry in r and leaves in r the A, X and Y that the
   ROM leaves at its RTS. r->p is not used: none reads an entry flag, and no caller reads a flag after any of them.
   The producer frame is read with fw_ind_y and written with fw_wr_ind_y on isr_fill_ptr; ROM tables are read with
   fw_rom8 at their ROM addresses. */
#include "fw.h"

#define FRM_PAUSE_PHONEME     0x3B   /* $B428 LDY #$3B: the pause phoneme, whose FN, F1, F2, F3 the rest frame takes */
#define FRM_MARK_SPACE_CAP    0xA0   /* $B93C: b11 is at most 160 - 2 x b10 (unsigned) ... */
#define FRM_MARK_SPACE_BASE   0x2F   /* $B947: ... at least 47 - 2 x b10 unless that has bit 7 set ... */
#define FRM_MARK_SPACE_FLOOR  0x0B   /* $B954: ... and at least 11 */
#define FRM_RAISE_PITCH       0x20   /* $B962: pitch codes b10 from 32 up raise F1 and F2 */
#define FRM_LEVEL_MARGIN      4      /* $B9E3 SBC #4: d = ALF - A1 - 4 */
#define FRM_F1_BASE           0x21   /* $B9F2 SBC #$21: e = F1 - 33 */
#define FRM_PRODUCT_SATURATED 0xFF   /* $BA1A LDA #$FF: the product index when frm_product_limit[e] - d is negative */
/* The bits of frm_flags and b13 (frm_transition_flags), from d = frm_snap_prev - frm_snap per byte */
#define FRM_FLAG_JUMP         0x80   /* a frequency moved by 16 or more, or an amplitude by 12 or more */
#define FRM_FLAG_FALL         0x40   /* an amplitude fell by 28 or more, or by 19 or more to the silent value 1 */
#define FRM_FLAG_RISES        0x10   /* two or more amplitudes rose by 12 or more */
#define FRM_FLAG_PITCH        0x08   /* the pitch code b10 moved by 7 or more */

/* $B41D: frm_rest_frame ($2000) = frm_template, copied 1-based for Y = 20 down to 1; then its FN, F1, F2, F3 (b1, b3,
   b5, b7, X = 1, 3, 5, 7) = row FRM_PAUSE_PHONEME of the target rows at par_row_ptr, moved on one parameter block
   by par_next_param after each; then par_row_ptr = par_params_ptr. par_setup_voice leaves par_row_ptr at
   par_params_ptr (P1's target row) before the call. r in: not read (Y and X are loaded first). r out: A =
   par_params_ptr's high byte, X = 9 (the INX / INX loop ends at CPX #8), Y = FRM_PAUSE_PHONEME. */
void frm_build_rest_frame(fw *f, fw_regs *r);

/* $B445: the 20 bytes of frm_rest_frame into the producer frame, Y = 19 down to 0 (DEY / BPL). r in: X passes
   through. r out: A = frm_rest_frame[0], X unchanged, Y = $FF. */
void frm_copy_rest_frame(fw *f, fw_regs *r);

/* $B917: frm_snap_prev = frm_snap (1-based, Y = 10 down to 1); then frm_snap[X - 1] = the producer frame's byte
   frm_snapshot_map[X - 1], X = 10 down to 1 (b10, AHF, A3, A2, A1, ALF, FN, F1, F2, F3 in frm_snap[0..9]).
   r in: not read. r out: A = the frame byte of the last entry read (frm_snapshot_map[0], the pitch code b10), X = 0,
   Y = that map entry. */
void frm_snapshot(fw *f, fw_regs *r);

/* $B930: with c = b10 (the pitch code), FW_TMP_75 = 2c (8 bits, ASL). b11 = @M (cmd_mark_space); then b11 =
   160 - 2c when b11 is above it (CMP / BCS, unsigned; 160 - 2c wraps in 8 bits for c > 80); then b11 = 47 - 2c
   when that 8-bit difference has bit 7 clear (BMI) and is not below b11 (CMP / BCC); then b11 = 11 when b11 is not
   above 11. Each write goes to the frame (up to four). r in: X passes through. r out: A = $0B, X unchanged,
   Y = FRAME_MARK_SPACE (11). */
void frm_clamp_mark_space(fw *f, fw_regs *r);

/* $B95D: d = b10 - 32 (8 bits). With d's bit 7 set (BPL not taken) it returns at once: A = d, Y = FRAME_PITCH (10).
   Otherwise FW_TMP_75 = d >> 2; F1 (b3) = par_clamp(F1 + (d >> 2)), then F2 (b5) = par_clamp(F2 + (d >> 3)), each
   sum 8 bits (CLC / ADC). r in: X passes through (par_clamp keeps it). r out: A = d and Y = 10 (the early return),
   else A = the new F2 and Y = FRAME_F2 (5); X unchanged. */
void frm_raise_formants(fw *f, fw_regs *r);

/* $BA3F: b13 = frm_flags. With d = frm_snap_prev[i] - frm_snap[i] (SEC / SBC, 8 bits), each test is the N flag of
   an 8-bit CMP (bit 7 of d - k, not a true signed order). frm_flags = 0, then each bit OR-ed in by its own
   read-modify-write:
   - FRM_FLAG_JUMP when, for F3, F2, F1, FN (i = 9 down to 6), d - 16 has bit 7 clear or d - $F1 has it set; else
     when, for ALF, A1, A2, A3, AHF (i = 5 down to 1), d - 12 has it clear or d - $F5 has it set;
   - FRM_FLAG_FALL when, for i = 5 down to 1, d - 28 has bit 7 clear, or d - 19 when frm_snap[i] is 1 (the silent
     value);
   - FRM_FLAG_RISES when FW_TMP_75, cleared (LDM) and then counting the i = 5 down to 1 whose frm_snap[i] -
     frm_snap_prev[i] - 12 has bit 7 clear, minus 2 has bit 7 clear (two or more);
   - FRM_FLAG_PITCH when, for the pitch code (i = 0), d - 7 has bit 7 clear or d - $FA has it set.
   The scans for FRM_FLAG_JUMP and FRM_FLAG_FALL stop at their first hit. r in: X passes through. r out: A =
   frm_flags, X unchanged, Y = FRAME_FLAGS (13). */
void frm_transition_flags(fw *f, fw_regs *r);

/* $B9B5: finishes the producer frame after par_step_tracks: frm_snapshot, frm_clamp_mark_space and
   frm_raise_formants; b16 = frm_source_codes[frm_source_index] and b17 = frm_filter_codes[frm_filter_index]; the
   b14-b15 word from frm_b14_15_lo/hi at a product index, or 0 (frm_b14 = 0) unless d = ALF - A1 - 4 and e = F1 - 33
   (frm_snap[5] - frm_snap[4] - 4 and frm_snap[7] - 33, 8 bits) are both not 0 with bit 7 clear (BEQ, BMI); the index
   is FRM_PRODUCT_SATURATED when frm_product_limit[e] - d has bit 7 set (so always for e = 1), else d * e by an 8-bit
   shift-and-add multiply that loses its carries and leaves frm_level_diff (d) and frm_f1_excess (e) at 0; b15 is
   written before b14, which goes through frm_b14; then frm_transition_flags. r: A, X and Y are preserved (PHA ...
   PLA; the pushes go to the dead stack, which is not reproduced). */
void frm_finish_frame(fw *f, fw_regs *r);

#endif
