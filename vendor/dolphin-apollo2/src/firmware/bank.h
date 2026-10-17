#ifndef FW_BANK_H
#define FW_BANK_H
/* The bank code (ROM $A600 and the RAM copy $0210-$0312), decompiled (M4.6c1 spec 2.1). The RAM-code routines read
   the bank tables from the RAM copy ($0200,X and $0208,X), every byte of the selected bank through fw_rom8 (so under
   another bank through the host's read_bank) and P2 through the SFR helpers, in the ROM's order. Each takes A, X and
   Y in r and leaves them as the ROM does; the PHP / SEI ... PLP pairs are invisible to the C (nothing else runs while
   it runs), and no caller reads a flag. */
#include "fw.h"

/* $A600: $78 = Y, $76 = X, $75 = P2 (one read); bank_ramcode_image $ADFE-$AFFC copied to $0200-$03FE in 256 passes
   of $ADFE,X -> $0200,X and $AEFD,X -> $02FF,X ($02FF is written twice, with the same byte). A and Y kept, X = 0. */
void bank_load_ramcode(fw *f, fw_regs *r);
/* $0210: A = the bank. bank_select, bank_probe; an absent bank: bank_restore (P2 = $75, X = $76, Y = $78, A kept).
   Else the bank entry through fw_enter: $FFD4 (warm) with A = the bank's $FFE0 and X = $FF when its $FFE1 and $FFE0
   equal bank_ver_minor and bank_ver_major (read in that order, $FFE0 only when $FFE1 is equal); else $01-$64 cleared
   and $FFD0 (cold) with A = 0 and X = 0. fw_enter does not return while a trap is set. */
void bank_enter(fw *f, fw_regs *r);
/* $0241: for A = 7 down to 1: bank_select, bank_probe, bank_read_id; then bank_restore: P2 = $75 (one write), X = $76,
   Y = $78. A kept. */
void bank_scan(fw *f, fw_regs *r);
/* $025E: A = the bank: X = bank_p2_table[A] ($0200,X, an 8-bit index: 8-15 read bank_p2_table_alt), or
   bank_p2_table_alt[A] ($0208,X) while boot_dip bit 1 is set; $77 = P2 & $83 (one read); P2 = X | $77 (one write).
   A and Y kept. */
void bank_select(fw *f, fw_regs *r);
/* $0277: bank_absent bit 0 set, then cleared when the selected bank reads BANK_SIGNATURE ($78) at $FFD0. A, X and Y
   kept. */
void bank_probe(fw *f, fw_regs *r);
/* $0285: A = the bank. X = 14 x (A - 1) by an 8-bit add loop (A = 0 runs it 255 times); while bank_absent bit 0 is
   set 14 spaces to bank_id_table + X, Y = 14; else bank_put_id_char of the 6 bytes at $FFE2,Y, then $9800-$9803 and
   $FFEA-$FFED, Y = 6. A kept; X after the last byte written (8 bits). */
void bank_read_id(fw *f, fw_regs *r);
/* $02E1: bank_safe_char, then bank_id_table[X] = A, X + 1 (8 bits). Y kept. */
void bank_put_id_char(fw *f, fw_regs *r);
/* $02E9: A below $20 becomes '*'. X and Y kept. */
void bank_safe_char(fw *f, fw_regs *r);
/* $02F0: P2 & $7C (one read of P2); bank_current = the first X in 1-7 whose table entry equals it (the table chosen by
   boot_dip bit 1 at each pass); unchanged when none does. A, X and Y kept. */
void bank_find_current(fw *f, fw_regs *r);

#endif
