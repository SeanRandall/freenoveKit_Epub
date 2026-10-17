#ifndef FW_BANK_ADDR_H
#define FW_BANK_ADDR_H
/* Addresses of the bank code (M4.6c1 spec 2.1, 3.5), checked against build/ghidra740/dump and the JSR/JMP operands
   in the ROM image. bank_load_ramcode copies bank_ramcode_image $ADFE-$AFFC to RAM $0200-$03FE: the two bank tables
   and the routines below run from there (RAM address + BANK_RAMCODE_OFFSET = ROM address). A caller in parentheses
   was ROM until M4.6c2: it is C now and calls the C. */

/* The RAM copy */
#define BANK_RAMCODE              0x0200   /* the copy's first byte, bank_p2_table */
#define BANK_P2_TABLE             0x0200   /* byte[8]: P2 per bank 0-7 while DIP bit 1 is clear (entry 0 unused) */
#define BANK_P2_TABLE_ALT         0x0208   /* byte[8]: the same while DIP bit 1 is set */
#define BANK_ENTER                0x0210   /* called by bank_cmd_select $ABB5 */
#define BANK_SCAN                 0x0241   /* called by (boot_reentry_warm $A67B) */
#define BANK_RESTORE              0x0252   /* bank_scan's tail, also bank_enter's exit ($021C JMP) */
#define BANK_SELECT               0x025E   /* called by bank_enter $0213, bank_scan $0246 */
#define BANK_PROBE                0x0277   /* called by bank_enter $0216, bank_scan $0249 */
#define BANK_READ_ID              0x0285   /* called by bank_scan $024C */
#define BANK_PUT_ID_CHAR          0x02E1   /* called by bank_read_id $029A, $02A5 ... $02CF (nine calls) */
#define BANK_SAFE_CHAR            0x02E9   /* called by bank_put_id_char $02E1 */
#define BANK_FIND_CURRENT         0x02F0   /* called by bank_cmd_select $ABB8 (and boot_reentry_warm $A67E) */
#define BANK_RAMCODE_LTS          0x0313   /* the copy of the letter-to-sound code $AF11-$AFFC (never run in RAM) */
#define BANK_RAMCODE_LAST         0x03FE   /* the copy's last byte */

/* The ROM image of the copy */
#define BANK_RAMCODE_IMAGE        0xADFE   /* bank_ramcode_image: $ADFE-$AFFC */
#define BANK_RAMCODE_IMAGE_END    0xAFFC
#define BANK_RAMCODE_OFFSET       0xABFE   /* ROM address - RAM address of the copied bytes */

/* Main-ROM routines */
#define ROM_BANK_LOAD_RAMCODE     0xA600   /* called by bank_cmd_select $ABB2 (and boot_reentry_warm $A678) */
#define ROM_BOOT_REENTRY_COLD     0xA665   /* bank_entry_cold's JMP target (boot_reset's tail: boot_read_dip, ...) */
#define ROM_BOOT_REENTRY_WARM     0xA66B   /* bank_entry_warm's JMP target */

/* A bank's header (every bank has one at the same addresses; the main ROM's is converted data) */
#define BANK_ENTRY_COLD           0xFFD0   /* SEI / JMP: entered by bank_enter's JMP $FFD0 */
#define BANK_ENTRY_WARM           0xFFD4   /* JMP: entered by bank_enter's JMP $FFD4 */
#define BANK_SIGNATURE            0x78     /* bank_probe: a present bank's byte at BANK_ENTRY_COLD (the SEI opcode) */
#define BANK_HDR_VERSION          0xFFE0   /* byte[2]: the engine version major, minor */
#define BANK_HDR_LANG_CODE        0xFFE2   /* char[6]: ID part 1 */
#define BANK_HDR_LANG_VER_TEXT    0xFFEA   /* char[4]: ID part 3 */
#define BANK_HDR_ENGINE_VER_TEXT  0x9800   /* char[4]: ID part 2 */
#define BANK_ID_CHARS             14       /* bank_read_id: 6 + 4 + 4 characters per bank in bank_id_table */
#define BANK_LANG_CODE_CHARS      6        /* bank_read_id: the characters of bank_hdr_lang_code */
#define BANK_VER_TEXT_CHARS       4        /* bank_read_id: the characters of bank_hdr_engine_ver_text, and of
                                              bank_hdr_lang_ver_text */
#define BANK_COUNT                7        /* bank_scan: banks 7 down to 1; bank_find_current: 1 to 7 */
#define BANK_STACK_TOP            0xFF     /* bank_enter's stack reset (LDX #$FF / TXS) */
#define BANK_ZP_CLEAR_LAST        0x64     /* bank_enter's cold path clears $01-$64 */

/* P2 bank selections (bits 2-6), the values of bank_p2_table per bank number: P2_MAIN_BANK (fw.h) is banks 0 and 1 */
#define P2_BANK_2                 0x50
#define P2_BANK_3                 0x30
#define P2_BANK_4                 0x10
#define P2_BANK_5                 0x78
#define P2_BANK_6                 0x74
#define P2_BANK_7                 0x7C
#define P2_KEEP                   0x83     /* bank_select keeps P2 bits 0, 1 and 7 (AND #$83) */
#define DIP_ALT_BANKS             0x02     /* boot_dip bit 1: bank_select and bank_find_current read the alt table */

#endif
