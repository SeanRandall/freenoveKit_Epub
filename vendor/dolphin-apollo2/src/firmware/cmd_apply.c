#include "cmd_apply.h"
#include "cmd_match.h"
#include "fw_ops.h"
#include "par_start.h"
#include "data/fw_data.h"

/* $A7FE / $A814: the count byte is data (converted); the table address is an immediate (LDA #$6A / #$D4,
   LDA #$E3 / #$E5), so the table itself (M4.4) is never read here */
static void select_voice(fw *f, uint8_t speaker, uint16_t count_addr, uint16_t table)
{
    fw_wr8(f, CMD_SPEAKER, speaker);
    fw_wr8(f, PAR_COUNT, fw_rom8(f, count_addr));
    fw_wr16(f, PAR_TABLE_PTR, table);
    fw_wr16(f, PAR_SPARE, 0);                                   /* the shared tail $A828 */
}

void cmd_select_voice_a(fw *f) { select_voice(f, 0, PAR_VOICE_A_COUNT, PAR_VOICE_A); }
void cmd_select_voice_b(fw *f) { select_voice(f, 1, PAR_VOICE_B_COUNT, PAR_VOICE_B); }

void cmd_apply_pitch(fw *f, uint8_t *x, uint8_t *y)
{
    fw_wr8(f, CMD_SOURCE_PROSODY, (uint8_t)(util_shl4(fw_rd8(f, CMD_SOURCE)) | fw_rd8(f, CMD_PROSODY)));
    uint8_t a = util_shl4(fw_rd8(f, CMD_PITCH));
    fw_wr8(f, PROS_PITCH_CENTRE, a);
    fw_regs r = { a, *x, *y, 0 };
    par_setup_voice(f, &r);                     /* $A845: the C (M4.4a); it leaves X = 9 and Y = $0B */
    *x = r.x;
    *y = r.y;
}

void cmd_apply_speed(fw *f)
{
    uint8_t i = (uint8_t)(fw_rd8(f, CMD_SPEED) - 1);                     /* DEC A / TAY */
    fw_wr8(f, CMD_SPEED_LIMIT, fw_rom8(f, (uint16_t)(CMD_SPEED_LIMITS + i)));
    i = (uint8_t)(i << 1);                                              /* ASL A / TAY */
    fw_wr8(f, CMD_SPEED_MAP, fw_rom8(f, (uint16_t)(CMD_SPEED_MAPS + i)));
    fw_wr8(f, CMD_SPEED_MAP + 1, fw_rom8(f, (uint16_t)(CMD_SPEED_MAPS + 1 + i)));
}

uint8_t cmd_apply_speaker(fw *f, uint8_t *x, uint8_t *y)
{
    uint8_t a = fw_rd8(f, CMD_SPEAKER);
    if (a == 0)
        cmd_select_voice_a(f);
    else
        cmd_select_voice_b(f);
    cmd_apply_pitch(f, x, y);
    cmd_apply_speed(f);
    return a;
}

void cmd_clear_formant(fw *f)
{
    for (uint8_t y = 11; y-- != 0;)                                     /* DEY / STA $2014,Y / BNE */
        fw_wr8(f, (uint16_t)(CMD_FORMANT_OFS + y), 0);
}
