#ifndef FW_SER_ADDR_H
#define FW_SER_ADDR_H
/* ROM code addresses of the serial code (M4.6a spec 2.1), checked against build/ghidra740/dump and the JSR/JMP
   operands and vectors in the ROM image. A caller in parentheses was ROM until M4.6c2: it is C now and calls the C. */
#define ROM_ISR_INT1             0xC2B4   /* INT1: entered through vec_int1_serial $FFFA */
#define ROM_SER_RX_BYTE          0xC37E   /* called by isr_int1 $C32E, $C356, $C36F */
#define ROM_SER_RX_PUT           0xC3CC   /* called by ser_rx_byte $C38D, ser_rx_flush_cmd $C3FF, $C409 (and
                                             boot_queue_string $C622) */
#define ROM_ISR_INT4_HOST_LINE   0xC3E0   /* INT4: entered through vec_int4_timerx $FFF4 */
#define ROM_ISR_HOST_LINE_HIGH   0xC3EB   /* called by isr_int4_host_line $C3E7 (and boot_init_host_lines $AC74) */
#define ROM_ISR_HOST_LINE_LOW    0xC3F4   /* called by isr_int4_host_line $C3E3 (and boot_init_host_lines $AC7A) */
#define ROM_SER_RX_FLUSH_CMD     0xC3FA   /* called by cmd_imm_match $C446, cmd_imm_index_add $C48F */
#define ROM_CMD_IMM_MATCH        0xC413   /* called by ser_rx_byte $C3A8 */
/* Immediate command handlers: cmd_imm_match's JMP (cmd_imm_vector) at $C46A; each ends with JMP cmd_imm_done $C46C */
#define ROM_CMD_IMM_INDEX_ADD    0xC483   /* "I+" */
#define ROM_CMD_IMM_INDEX_QUERY  0xC495   /* "I?" */
#define ROM_SER_TX_GET           0xC4B5   /* called by isr_int1 $C30A, ser_tx_service $C58A */
#define ROM_SER_TX_PUT           0xC4DD   /* queue A as a reply byte; A, X, Y preserved. Called by
                                             cmd_imm_index_query $C497, $C4A9, ser_tx_hex $C578, $C57C, ser_tx_records
                                             $C5AB, $C5BE, $C5CD, cmd_echo $AB99 and cmd_reply_setting $C643
                                             (both C: they call the C ser_tx_put) */
#define ROM_SER_TX_SEND          0xC504   /* called by ser_tx_service $C58D */
#define ROM_SER_HEX_DIGITS       0xC53A   /* called by ser_tx_hex $C574 */
#define ROM_SER_HEX_DIGIT        0xC55A   /* called by ser_hex_digits $C542, $C54F */
#define ROM_SER_TX_HEX           0xC571   /* queue A as two ASCII hex digits, low nibble first; A preserved. Called by
                                             cmd_imm_index_query $C49C, ser_tx_records $C5B0, $C5B5, and
                                             cmd_reply_setting $C650 (C: calls the C ser_tx_hex) */
#define ROM_SER_TX_SERVICE       0xC581   /* called by par_play $B89C (C: calls the C ser_tx_service)
                                             (and boot_get_clause $A6D7) */
#define ROM_SER_TX_RECORDS       0xC5A4   /* queue the record reply ser_reply_id/_count/_size/_ptr describe; called by
                                             cmd_list_languages $ABCD (C: calls the C ser_tx_records) */
#define ROM_SER_RX_NEXT_PAGE     0xC5EC   /* called by ser_rx_put $C3D9 (and boot_get_clause $A6F5) */
#define ROM_SER_TX_NEXT_PAGE     0xC5F5   /* called by ser_tx_get $C4C6, ser_tx_put $C4F0 */

#endif
