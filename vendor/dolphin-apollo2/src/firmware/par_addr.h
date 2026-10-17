#ifndef FW_PAR_ADDR_H
#define FW_PAR_ADDR_H
/* ROM code addresses of the playback set-up (M4.4a spec 2.2), the frame loop (M4.4b spec 2.2) and the frame
   builder (M4.5 spec 2.2), checked against build/ghidra740/dump and the JSR/JMP operands in the ROM image (the
   interrupt routines they call are in isr_addr.h, M4.6b). A caller in parentheses was ROM until M4.6c2: it is C now
   and calls the C. */
#define ROM_BOOT_LOOP_PLAY         0xA7D4   /* called by tn_convert_clause $B204 (and boot_reentry_warm $A687,
                                               boot_selftest_sound $A7BA, cmd_say_phonetic $A87C) */
#define ROM_PAR_SETUP_VOICE        0xB3A4   /* called by cmd_apply_pitch $A845; the C cmd_apply_pitch calls the C */
#define ROM_FRM_BUILD_REST_FRAME   0xB41D   /* called by par_setup_voice $B404 */
#define ROM_FRM_COPY_REST_FRAME    0xB445   /* called by par_setup_voice $B407, $B416, par_play $B8DA */
#define ROM_PAR_PLAY               0xB877   /* called by boot_loop_play $B2C7; re-entered by its own JMPs $B892,
                                               $B8AE, $B914 */
#define ROM_FRM_SNAPSHOT           0xB917   /* called by par_play $B8DD, frm_finish_frame $B9BA */
#define ROM_FRM_CLAMP_MARK_SPACE   0xB930   /* called by par_setup_voice $B40A, $B419, par_play $B8E0,
                                               frm_finish_frame $B9BD */
#define ROM_FRM_RAISE_FORMANTS     0xB95D   /* called by frm_finish_frame $B9C0 */
#define ROM_PROS_MICROPROSODY      0xB983   /* called by par_read_segment $BD67 */
#define ROM_FRM_FINISH_FRAME       0xB9B5   /* called by par_play $B8C3, $B90E */
#define ROM_FRM_TRANSITION_FLAGS   0xBA3F   /* called by par_play $B8E3, frm_finish_frame $BA36 */
#define ROM_PAR_PLAY_START         0xBAF2   /* called by boot_play_body $B2B6 */
#define ROM_PAR_RESET_TRACKS       0xBB35   /* called by par_play_start $BAF2 */
#define ROM_CMD_RUN_EVENT          0xBB74   /* called by par_play $B88F */
#define ROM_PAR_NEXT_SEGMENT       0xBCF3   /* called by par_play $B900 */
#define ROM_PAR_NEXT_ROW           0xBD07   /* called by par_setup_voice $B3BE, $B3C7; par_boundary_cur_dom $BF09,
                                               $BF14; par_boundary_next_dom $BF4E, $BF59; par_read_dominance $BF7B,
                                               $BF8D, $BFBF */
#define ROM_PAR_NEXT_PARAM         0xBD14   /* called by frm_build_rest_frame $B431, par_read_segment $BD9D,
                                               par_boundary_jump $BEAC */
#define ROM_PAR_CLAMP              0xBD24   /* called by frm_raise_formants $B970, $B97D; par_read_segment $BD8F;
                                               par_read_dominance $BF75; par_boundary_value $C0BF */
#define ROM_PAR_READ_SEGMENT       0xBD33   /* called by par_next_segment $BCFD */
#define ROM_PROS_SCALE_TARGET      0xBDD3   /* called by par_play_start $BAFE, par_read_segment $BD64 */
#define ROM_PAR_BOUNDARY_PASS      0xBE6A   /* called by par_next_segment $BD00 */
#define ROM_PAR_BOUNDARY_JUMP      0xBE94   /* called by par_boundary_pass $BE7F */
#define ROM_PAR_STORE_BOUNDARY     0xBEB5   /* called by par_boundary_jump $BEA9, par_boundary_value $C0AE */
#define ROM_PAR_BOUNDARY_CUR_DOM   0xBEE2   /* called by par_boundary_pass $BE90 */
#define ROM_PAR_BOUNDARY_NEXT_DOM  0xBF27   /* called by par_boundary_pass $BE8C */
#define ROM_PAR_READ_DOMINANCE     0xBF6C   /* called by par_boundary_cur_dom $BEFE, par_boundary_next_dom $BF43 */
#define ROM_PAR_SCALE_VOICING      0xBFC3   /* called by par_read_segment $BD96, par_read_dominance $BF87 */
#define ROM_PAR_BOUNDARY_VALUE     0xC039   /* called by par_boundary_cur_dom $BF19, par_boundary_next_dom $BF5E */
#define ROM_PAR_PLAN_TRACKS        0xC0D2   /* called by par_next_segment $BD03 */
#define ROM_PAR_STEP_TRACKS        0xC0FF   /* called by par_play $B8C0, $B90B */
#define ROM_PAR_TRACK_SEQUENTIAL   0xC14C   /* called by par_step_tracks $C10C */
#define ROM_UTIL_INTERP_STEP       0xC1A1   /* called by par_step_tracks $C124; par_track_sequential $C163, $C195;
                                               par_track_overlap $C210; par_track_start $C265; par_track_end $C28E */
#define ROM_PAR_TRACK_OVERLAP      0xC1F2   /* called by par_step_tracks $C106 */
#define ROM_PAR_TRACK_START        0xC24A   /* called by par_track_overlap $C1F2 */
#define ROM_PAR_TRACK_END          0xC26C   /* called by par_track_overlap $C1F8 */
#define ROM_PAR_SCALE_DURATION     0xC29C   /* called by par_boundary_cur_dom $BF03, $BF0E; par_boundary_next_dom
                                               $BF48, $BF53 */
#define ROM_UTIL_SHL4              0xC656   /* called by cmd_apply_pitch $A835, $A83F; cmd_set_formant $A952;
                                               cmd_set_mark_space $A9AD; cmd_hex_byte $B455 (all C: util_shl4 in
                                               fw_ops.h) */
#define ROM_UTIL_SHL3              0xC657   /* called by cmd_ev_sound_start $BBA7, isr_set_volume $ACEC (both C:
                                               util_shl3 in fw_ops.h); also entered from util_shl4 $C656 by falling
                                               through */

#endif
