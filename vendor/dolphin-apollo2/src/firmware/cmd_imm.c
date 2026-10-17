#include <stddef.h>
#include "cmd_imm.h"
#include "cmd_match.h"
#include "fw_ops.h"
#include "ser.h"
#include "ser_addr.h"
#include "data/fw_data.h"

/* The zero page cmd_imm_match keeps for the command scan it may interrupt, in push order ($C418-$C42C; pulled in
   reverse at cmd_imm_done $C46C-$C47F): cmd_table_ofs, cmd_table_ptr, cmd_text_ptr, cmd_pat_ptr. */
static const uint8_t k_kept[] = {
    CMD_TABLE_OFS, CMD_TABLE_PTR, CMD_TABLE_PTR + 1, CMD_TEXT_PTR, CMD_TEXT_PTR + 1, CMD_PAT_PTR, CMD_PAT_PTR + 1,
};

void cmd_imm_match(fw *f, fw_regs *r)
{
    uint8_t kept[sizeof k_kept];
    fw_wr_ind_y(f, (uint8_t)SER_CMD_PTR, 0, r->a);               /* PHA; LDY #0; STA (ser_cmd_ptr),Y */
    for (size_t i = 0; i < sizeof k_kept; i++)
        kept[i] = fw_rd8(f, k_kept[i]);
    fw_wr8(f, CMD_TABLE_OFS, fw_rd8(f, SER_CMD_TABLE_OFS));      /* $C42D */
    fw_wr16(f, CMD_TEXT_PTR, SER_CMD_BUF);
    fw_wr16(f, CMD_TABLE_PTR, CMD_IMM_TABLE);
    fw_regs t = *r;
    t.y = cmd_match_table(f);                                    /* X kept */
    if (t.y == 0) {                                              /* $C440 CPY #0; BNE: no match */
        fw_clb(f, SER_CMD_MODE, 0);
        t.a = 0xFF;                                              /* cmd_match_table's A at the end marker */
        ser_rx_flush_cmd(f, &t);
    } else if (fw_ind_y(f, (uint8_t)CMD_PAT_PTR, t.y) != 0) {    /* $C44C LDA (cmd_pat_ptr),Y; BEQ: partial */
        t.y = fw_rd8(f, CMD_TABLE_OFS);
        fw_wr8(f, SER_CMD_TABLE_OFS, t.y);
        uint8_t lo = (uint8_t)(fw_rd8(f, SER_CMD_PTR) + 1);      /* INC ser_cmd_ptr; BNE; INC ser_cmd_ptr+1 */
        fw_wr8(f, SER_CMD_PTR, lo);
        if (lo == 0)
            fw_inc8(f, SER_CMD_PTR + 1);
    } else {                                                     /* $C45C: a full match */
        uint16_t handler = (uint16_t)(fw_ind_y(f, (uint8_t)CMD_PAT_PTR, (uint8_t)(t.y + 1)) |
                                      fw_ind_y(f, (uint8_t)CMD_PAT_PTR, (uint8_t)(t.y + 2)) << 8);
        fw_wr16(f, CMD_IMM_VECTOR, handler);
        t.y = 0;
        t.a = fw_ind_y(f, (uint8_t)SER_CMD_PTR, 0);              /* $C468 LDA (ser_cmd_ptr),Y */
        switch (handler) {                                       /* $C46A JMP (cmd_imm_vector) */
        case ROM_CMD_IMM_INDEX_ADD:
            cmd_imm_index_add(f, &t);
            break;
        case ROM_CMD_IMM_INDEX_QUERY:
            cmd_imm_index_query(f, &t);
            break;
        default:
            fw_fault(f, handler);
            break;
        }
    }
    for (size_t i = sizeof k_kept; i-- > 0;)                     /* cmd_imm_done */
        fw_wr8(f, k_kept[i], kept[i]);
    r->y = t.y;                                                  /* $C481 PLA: A as it came; X kept */
}

void cmd_imm_index_add(fw *f, fw_regs *r)
{
    if (fw_rd8(f, CMD_INDEX_RESET) & 1) {                        /* BBC0 cmd_index_reset */
        fw_wr8(f, CMD_INDEX_COUNT, 0);
        fw_clb(f, CMD_INDEX_RESET, 0);
    }
    fw_inc8(f, CMD_INDEX_COUNT);
    fw_clb(f, SER_CMD_MODE, 0);
    ser_rx_flush_cmd(f, r);
}

void cmd_imm_index_query(fw *f, fw_regs *r)
{
    r->a = 'I';
    ser_tx_put(f, r);
    r->a = fw_rd8(f, CMD_INDEX_COUNT);
    ser_tx_hex(f, r);
    r->a = 'T';                                                  /* text waiting or speech in progress */
    if ((fw_rd8(f, SER_RX_IDLE_SAVED) & 1) && !(fw_rd8(f, BOOT_SPEAKING) & 1))
        r->a = 'M';                                              /* $C4A7: neither */
    ser_tx_put(f, r);
    fw_clb(f, SER_CMD_MODE, 0);
    r->a = fw_rd8(f, SER_RX_IDLE_SAVED);
    fw_wr8(f, SER_RX_IDLE, r->a);
}
