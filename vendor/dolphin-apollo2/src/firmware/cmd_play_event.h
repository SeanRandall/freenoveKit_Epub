#ifndef FW_CMD_PLAY_EVENT_H
#define FW_CMD_PLAY_EVENT_H
/* The command events at playback (ROM $BB74-$BB9F and $BBA0-$BCF2), decompiled (M4.4b spec 2.2, 2.6): the
   dispatcher the frame loop par_play calls when playback reaches an event's position, and the five handlers the
   producers queue (cmd_event.c: @I+, @T, @B, @A). Every routine takes the registers the ROM has at its entry in r
   (r->p is not read: no routine here reads an entry flag) and returns in r the A, X and Y the ROM leaves at its
   RTS; a register the routine does not touch is left as it came in. */
#include "fw.h"

#define CMD_SOUND_ACTIVE_ON       0x01   /* cmd_sound_active bit 0: an effect plays (SEB0 $BC7D, BBS0 $BC80, CLB0
                                            $BC84) */
#define CMD_SOUND_PITCH_CENTRE    0x80   /* $BC43: pros_pitch_centre while an effect plays */
#define CMD_SOUND_SOURCE_PROSODY  3      /* $BC48: cmd_source_prosody while an effect plays (the R that
                                            pros_scale_target reads, AND #7, is 3) */
#define CMD_SOUND_VOICING         8      /* $BC4D: cmd_voicing while an effect plays (@B8: par_scale_voicing passes
                                            P10 unscaled) */

/* $BB74: runs the event cmd_ev_next. A = cmd_ev_next, then cmd_ev_next + 1; X = A << 1 (8 bits).
   cmd_ev_vector = cmd_ev_handlers[X] (the word at $1306 + X); after INX / INX, which wrap X at 8 bits, cmd_ev_pos =
   the word at $1206 + X, the next event's position (cmd_ev_positions[0] when cmd_ev_next was 127). Then X =
   cmd_ev_next - 1 and A = cmd_ev_args[X] (the byte at $1406 + X), and JMP (cmd_ev_vector): the C handler of the
   five cmd_ev_* addresses with that A and X and the entry Y; any other address through fw_rom_call with the same
   registers. r in: Y (passed to the handler). r out: what the handler leaves. */
void cmd_run_event(fw *f, fw_regs *r);
/* $BBA0, the @I+ event: cmd_index_count -= 1 unless it is 0 (TST / BEQ). A, X and Y untouched. */
void cmd_ev_index(fw *f, fw_regs *r);
/* $BBA7, the first @T event, A = the effect 0-16: X = util_shl3(A). The four playback pointers ($22-$29) to
   cmd_sound_save; the effect's four from cmd_sound_table + X (fw_rom8). par_seg_index to cmd_sound_seg_index;
   pros_seg_count, cmd_voicing, pros_pitch_centre, cmd_source_prosody, pros_play_target to cmd_sound_state[0..5],
   par_next_phoneme to [7], then par_next_rank to [6]. par_next_phoneme = the effect's first phoneme,
   (pros_phoneme_ptr),0; par_next_rank = its rank, (par_rank_ptr),phoneme. pros_pitch_centre = $80,
   cmd_source_prosody = 3, cmd_voicing = 8, par_seg_index = 0, pros_seg_count = $00FF. Y = the index of the first
   CMD_SOUND_END in the effect's phonemes (INY first, from $FF); the word at $1206 + (cmd_ev_next << 1, 8 bits), the
   end event's position, and cmd_ev_pos = Y, high bytes 0; cmd_sound_active bit 0 set. No end within 256 bytes: the
   ROM loops forever, the C faults at ROM_CMD_EV_SOUND_START. r in: A. r out: A = 0, X = cmd_ev_next << 1 (8 bits),
   Y = the end's index. */
void cmd_ev_sound_start(fw *f, fw_regs *r);
/* $BC80, the second @T event (and par_play's last call): when cmd_sound_active bit 0 is set, clears it and
   restores what cmd_ev_sound_start saved, in the ROM's order (cmd_sound_state[7] to par_next_phoneme before [6] to
   par_next_rank). r out: A = cmd_sound_state[6] when it restored, else as it came in; X and Y untouched. */
void cmd_ev_sound_end(fw *f, fw_regs *r);
/* $BCEA, the @B event: cmd_voicing = A. A, X and Y untouched. */
void cmd_ev_set_voicing(fw *f, fw_regs *r);
/* $BCED, the @A event: cmd_volume = A, then isr_set_volume (C since M4.6b) with the entry registers. r out: what
   isr_set_volume leaves (A = the new P3 value, X and Y untouched). */
void cmd_ev_set_volume(fw *f, fw_regs *r);

#endif
