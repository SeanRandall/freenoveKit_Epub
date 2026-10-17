#ifndef FW_CMD_APPLY_H
#define FW_CMD_APPLY_H
/* The settings helpers of the command handlers (ROM $A78C-$A867), decompiled (M4.2 spec 2.2). */
#include "fw.h"

/* $A78C: the voice tables of cmd_speaker (0: voice A, else voice B), then cmd_apply_pitch and cmd_apply_speed.
   *x and *y go on to par_setup_voice and come back as it leaves them (X = 9, Y = $0B). Returns A = the
   cmd_speaker it read; every callee preserves A. */
uint8_t cmd_apply_speaker(fw *f, uint8_t *x, uint8_t *y);
/* $A7FE / $A814, with their shared tail $A828: cmd_speaker = 0 / 1, par_count = the voice table's count byte,
   par_table_ptr = the table after it (par_voice_a / par_voice_b), par_spare = 0. */
void cmd_select_voice_a(fw *f);
void cmd_select_voice_b(fw *f);
/* $A832: cmd_source_prosody = cmd_source * 16 | cmd_prosody, pros_pitch_centre = cmd_pitch * 16, then the C
   par_setup_voice (M4.4a) rebuilds the voice set-up, with A = pros_pitch_centre, X = *x and Y = *y (it reads
   none of them). *x and *y return what it leaves; cmd_apply_pitch restores only A. */
void cmd_apply_pitch(fw *f, uint8_t *x, uint8_t *y);
/* $A84A: cmd_speed_limit = cmd_speed_limits[cmd_speed - 1], cmd_speed_map = cmd_speed_maps[cmd_speed - 1]. */
void cmd_apply_speed(fw *f);
/* $A7DF: $2014-$201E (cmd_formant_ofs and par_ofs) = 0. */
void cmd_clear_formant(fw *f);

#endif
