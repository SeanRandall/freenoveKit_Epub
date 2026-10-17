#include "boot_input.h"
#include "boot.h"
#include "boot_addr.h"
#include "cmd_scan.h"
#include "fw_ops.h"
#include "isr_serial.h"
#include "ser.h"
#include "tn_convert.h"

#define BOOT_CR  0x0D   /* boot_get_clause: ends the clause, not kept */

/* $A6B4-$A6BE, $A6C2-$A6CC: the input ring is empty (ser_rx_write = ser_rx_read: the low bytes, then the pages). */
static bool ring_empty(const fw *f)
{
    return fw_rd8(f, SER_RX_WRITE) == fw_rd8(f, SER_RX_READ) &&
           fw_rd8(f, SER_RX_READ + 1) == fw_rd8(f, SER_RX_WRITE + 1);
}

/* $A6CE-$A6E2: a pass with the input ring empty. The ROM loops at once ($A6DA, $A6E2 BRA $A6C2); where that pass
   changed nothing (ser_tx_service sent nothing: the host line low, or parallel mode; isr_idle in parallel mode, which
   returns at once) the C waits for an interrupt first, which is what can change the state (M4.6c2 spec 2.5). */
static void empty_pass(fw *f, fw_regs *r)
{
    fw_seb(f, SER_RX_IDLE, 0);
    fw_par_ready(f, *r);                                         /* $A6D0 */
    fw_set_interrupts(f, true, BOOT_CLI_EMPTY);                  /* $A6D3 CLI */
    bool changed;
    if (fw_rd8(f, SER_STATUS_DUE) & 1) {                         /* $A6D4 BBC0 ser_status_due */
        changed = ser_tx_service(f, r);                          /* $A6D7 */
    } else {
        fw_par_ready(f, *r);                                     /* $A6DC */
        isr_idle(f, r);                                          /* $A6DF: in serial mode, the wait for input */
        changed = !(fw_rd8(f, BOOT_DIP) & DIP_PARALLEL);
    }
    if (!changed)
        fw_wait(f, FW_WAIT_INTERRUPT, BOOT_GET_CLAUSE_LOOP);
}

/* $A704: a space after the clause, boot_clause_len = X + 1. */
static void end_clause(fw *f, fw_regs *r)
{
    r->a = ' ';
    fw_wr8(f, (uint16_t)(BOOT_CLAUSE + r->x), r->a);
    r->x++;
    fw_wr8(f, BOOT_CLAUSE_LEN, r->x);
}

/* $A6EB-$A73F: the byte at ser_rx_read into boot_clause[X]; true when it ended the clause. */
static bool take_byte(fw *f, fw_regs *r)
{
    r->y = 0;
    r->a = fw_ind_y(f, (uint8_t)SER_RX_READ, r->y);              /* $A6EB LDY #0; LDA (ser_rx_read),Y */
    uint8_t lo = (uint8_t)(fw_rd8(f, SER_RX_READ) + 1);          /* INC ser_rx_read; BNE */
    fw_wr8(f, SER_RX_READ, lo);
    if (lo == 0) {
        r->y = fw_rd8(f, SER_RX_READ + 1);                       /* $A6F3 */
        ser_rx_next_page(f, r);
        fw_wr8(f, SER_RX_READ + 1, r->y);
    }
    fw_seb(f, BOOT_SPEAKING, 0);                                 /* $A6FA */
    fw_par_ready(f, *r);
    fw_set_interrupts(f, true, BOOT_CLI_READ);                   /* $A6FF CLI */
    if (r->a == BOOT_CR) {                                       /* CMP #$0D; BNE */
        end_clause(f, r);
        return true;
    }
    fw_wr8(f, (uint16_t)(BOOT_CLAUSE + r->x), r->a);             /* $A70D STA $1B00,X; INX */
    r->x++;
    bool ends = r->a == '.' || r->a == ',' || r->a == ':' || r->a == ';' || r->a == '!' || r->a == '?' ||
                r->x >= BOOT_CLAUSE_FULL;                        /* CPX #$FA; BCS */
    if (!ends && r->x >= BOOT_CLAUSE_SPACE_END) {                /* CPX #$DC; BCC */
        if (r->a == ' ') {
            ends = true;
        } else if (r->a == fw_rd8(f, CMD_CHAR)) {                /* CMP cmd_char */
            fw_wr8(f, FW_CLAUSE_CARRY, r->a);                    /* $A739: carried into the next clause */
            r->x--;
            ends = true;
        }
    }
    if (ends)
        end_clause(f, r);
    return ends;                                                 /* else $A73F JMP $A6C2 */
}

void isr_idle(fw *f, fw_regs *r)
{
    (void)r;                                                     /* PHA ... PLA: A kept */
    if (!(fw_rd8(f, BOOT_DIP) & DIP_PARALLEL))                   /* $C5FF BBS0 boot_dip,$C603 */
        fw_wait(f, FW_WAIT_INPUT, FW_WAIT_INPUT_LOOP);           /* $C602 WIT */
}

void boot_get_clause(fw *f, fw_regs *r)
{
    for (;;) {                                                   /* $A697, and again after a mute ($A6E8) */
        r->x = 1;
        r->a = ' ';
        fw_wr8(f, BOOT_CLAUSE, r->a);
        r->a = fw_rd8(f, FW_CLAUSE_CARRY);                       /* LDA $1827; BEQ */
        if (r->a) {
            fw_wr8(f, BOOT_CLAUSE + 1, r->a);
            r->x++;
            r->a = 0;
            fw_wr8(f, FW_CLAUSE_CARRY, r->a);
        }
        fw_wr8(f, SER_ABORT, r->a);                              /* $A6AC: A = 0 */
        fw_wr8(f, TN_IN_WORD, r->a);
        fw_wr8(f, FW_UTT_CONTENT, r->a);
        fw_wr8(f, BOOT_SPEAKING, r->a);
        if (ring_empty(f))                                       /* $A6B4-$A6C0 */
            fw_seb(f, SER_RX_IDLE, 0);
        for (;;) {                                               /* $A6C2 */
            while (ring_empty(f)) {
                empty_pass(f, r);
                if (f->fault || f->stopped)
                    return;                                      /* abandoned (without a trap: a unit test) */
            }
            isr_par_hold(f, r);                                  /* $A6E4 */
            fw_set_interrupts(f, false, BOOT_SEI_READ);          /* $A6E7 SEI */
            if (fw_rd8(f, SER_ABORT) & 1)                        /* $A6E8 BBS0 ser_abort,boot_get_clause */
                break;
            if (take_byte(f, r))
                return;
        }
    }
}

void boot_loop_text(fw *f, fw_regs *r)
{
    boot_get_clause(f, r);
    if (f->fault || f->stopped)
        return;
    cmd_scan_clause(f);
    tn_convert_clause(f);
}

void boot_queue_string(fw *f, fw_regs *r)
{
    fw_regs t = *r;                                              /* PHA / TYA / PHA ... PLA / TAY / PLA: A, Y kept */
    t.y = 0;
    isr_par_hold(f, &t);                                         /* $C617, then PHP / SEI ... PLP */
    for (;;) {
        t.a = fw_ind_y(f, (uint8_t)CMD_TEXT_PTR, t.y);           /* $C61C LDA ($46),Y; BEQ */
        if (t.a == 0)
            break;
        fw_wr8(f, FW_TMP_78, t.y);                               /* STY $78 */
        ser_rx_put(f, &t);
        t.y = fw_rd8(f, FW_TMP_78);                              /* LDY $78; INY */
        t.y++;
    }
    isr_par_ready(f, &t);                                        /* $C62B */
    r->x = t.x;
}
