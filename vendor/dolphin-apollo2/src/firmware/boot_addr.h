#ifndef FW_BOOT_ADDR_H
#define FW_BOOT_ADDR_H
/* ROM code addresses of boot, the main loop and isr_idle (M4.6c2 spec 2.1), checked against build/ghidra740/dump and
   the JSR/JMP operands and vectors in the ROM image. The re-entries and the bank entries are in bank_addr.h:
   ROM_BOOT_REENTRY_COLD $A665 (bank_entry_cold's JMP at $FFD1), ROM_BOOT_REENTRY_WARM $A66B (boot_reset's JMP at
   $A662, bank_entry_warm's JMP at $FFD4, and boot_reentry_cold falls into it), BANK_ENTRY_COLD $FFD0 (bank_enter's
   JMP at $0240) and BANK_ENTRY_WARM $FFD4 (bank_enter's JMP at $0232). */

/* The entry */
#define ROM_BOOT_RESET                  0xA61C   /* vec_reset $FFFE (and bank_entry_reset's JMP at $FFD8, which no
                                                    code reaches) */

/* The main loop's routines that wait */
#define ROM_BOOT_LOOP_TEXT              0xA68D   /* called by boot_reentry_warm $A681, boot_selftest_sound $A7B4 */
#define ROM_BOOT_GET_CLAUSE             0xA697   /* called by boot_loop_text $A68D */
#define ROM_ISR_IDLE                    0xC5FE   /* called by boot_get_clause $A6DF */
#define ROM_BOOT_SELFTEST_SOUND         0xA79F   /* called by boot_reset $A654 */
#define ROM_CMD_SAY_PHONETIC            0xA876   /* called by boot_init_baud $AC96, $ACAC, $ACBF (with X = 1 only:
                                                    unreachable) */

/* The helpers */
#define ROM_BOOT_READ_DIP               0xADE0   /* called by boot_reset $A645, boot_reentry_cold $A665 */
#define ROM_BOOT_INIT_PORTS             0xAC22   /* called by boot_reset $A648 */
#define ROM_BOOT_INIT_SETTINGS          0xADEB   /* called by boot_reset $A64E, boot_reentry_cold $A668 */
#define ROM_BOOT_CLEAR_INPUT_STATE      0xABF0   /* called by boot_init_settings $ADEB */
#define ROM_BOOT_SET_DEFAULTS           0xA742   /* called by boot_init_settings $ADEE */
#define ROM_BOOT_LOAD_PROSODY_DEFAULTS  0xA7F0   /* called by boot_set_defaults $A788 */
#define ROM_BOOT_INIT_BAUD              0xAC7E   /* called by boot_reset $A65C (with X = 0) */
#define ROM_BOOT_INIT_HOST_LINES        0xAC68   /* called by boot_reset $A65F */
#define ROM_BOOT_SELFTEST               0xC605   /* called by boot_selftest_sound $A7A5 */
#define ROM_BOOT_QUEUE_STRING           0xC612   /* called by boot_selftest_sound $A7B1 */
#define ROM_ISR_UNUSED                  0xC3DF   /* vec_int3_step $FFF6, vec_int0 $FFFC (RTI) */

/* Instructions the C reports to the host */
#define BOOT_GET_CLAUSE_LOOP            0xA6C2   /* boot_get_clause's polling loop: its FW_WAIT_INTERRUPT */
#define BOOT_CLI_WARM                   0xA66E   /* boot_reentry_warm's CLI */
#define BOOT_CLI_EMPTY                  0xA6D3   /* boot_get_clause: the CLI of a pass with the input ring empty ... */
#define BOOT_SEI_READ                   0xA6E7   /* ... the SEI before a byte is read ... */
#define BOOT_CLI_READ                   0xA6FF   /* ... and the CLI after it */

#endif
