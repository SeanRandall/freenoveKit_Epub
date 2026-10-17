#ifndef FW_PROS_ADDR_H
#define FW_PROS_ADDR_H
/* ROM code addresses of the segment generator (M4.3a spec 2.2) and of the stress and phrase passes before it
   (M4.3b spec 2.2), checked against build/ghidra740/dump and the JSR/JMP operands in the ROM image.
   isr_par_ready, the one interrupt routine they call, is C since M4.6b (ROM_ISR_PAR_READY in isr_addr.h). */
#define ROM_PH_APPEND_PHRASE_PAUSE  0xC9B8   /* exits with JMP ROM_PROS_SEG_TAIL */
#define ROM_PH_APPEND_PAUSE         0xCA15
#define ROM_PROS_APPEND_SEGMENT     0xCA3B
#define ROM_PROS_SEG_TAIL           0xCA6D   /* pros_append_segment's PLA / TAY / PLA / RTS (restores Y and A) */
#define ROM_PROS_RESET_SEG_PTRS     0xCA71   /* also called at playback by par_play_start $BAF5 */
#define ROM_PROS_MAKE_SEGMENTS      0xCA92   /* called by boot_reentry_warm $A684, boot_selftest_sound $A7B7 and
                                                cmd_say_phonetic $A879, and tn_convert_clause: their C calls the C */
#define ROM_PROS_MARK_STRESS        0xCABD   /* called by pros_make_segments $CAB3 */
#define ROM_PROS_MARK_PHRASES_EXIT  0xCEEC   /* pros_mark_phrases' exit block, before its entry (BEQ from $CF26) */
#define ROM_PROS_MARK_PHRASES       0xCEF6   /* called by pros_make_segments $CAB6 */
#define ROM_PROS_STORE_PHRASE_LEN   0xD0FD   /* called by pros_mark_phrases $CF97 and $CEEC */
#define ROM_PROS_PHRASE_GEN         0xD129   /* called by pros_make_segments $CAB9 */
#define ROM_PH_DECODE_SYMBOL        0xD31B   /* a label inside pros_phrase_gen, not a routine */
#define ROM_PROS_DECLINATION        0xD3DD   /* called by pros_mark_phrases $D04D and pros_phrase_gen $D129 */
#define ROM_PROS_DECLINATION_STEP   0xD40D   /* JSR from $D3F2, and BCC/BEQ from $D3E3/$D3E5 (a tail-branch) */
#define ROM_PH_LENGTHEN_COUNT       0xD43E

#endif
