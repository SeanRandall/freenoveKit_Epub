/* Converted from the Apollo 2 ROM (M4.5 spec section 3), then reviewed: the frame builder's three small tables.
   Its two formula tables (frm_product_limit, frm_b14_15_lo/hi) are expanded by the encoder (fw_data.c, step 10).
   tests/test_firmware_data.c re-encodes it and compares it byte for byte with the ROM. */
#include "fw_data.h"

/* frm_template ($B783, read as $B782,Y with Y = 20 down to 1 by frm_build_rest_frame): the rest frame before the
   pause phoneme's FN, F1, F2, F3 are put over it. */
const uint8_t fw_frm_template[FRAME_SIZE] = {
    [FRAME_ALF] = 1, [FRAME_A1] = 1, [FRAME_A2] = 1, [FRAME_A3] = 1, [FRAME_AHF] = 1,   /* the silent value */
    [FRAME_FN] = 32, [FRAME_F1] = 32, [FRAME_F2] = 32, [FRAME_F3] = 32,                  /* mid-range */
    [FRAME_V] = 63,                         /* full voicing */
    [FRAME_PITCH] = 32,
    [FRAME_MARK_SPACE] = 32,
    [FRAME_B12] = 0x17,                     /* the constant b12 */
    [FRAME_FLAGS] = 0x20,                   /* bit 5, which the firmware never computes (parent spec 1.4) */
    [FRAME_B14] = 0x19, [FRAME_B15] = 0x24,
    [FRAME_SOURCE] = 0x82,                  /* the default source, frm_source_codes[0] */
    [FRAME_FILTER] = 0x4A,                  /* the male filter, frm_filter_codes[0] */
    [FRAME_END_MARKER] = 0xFF, [FRAME_END_MARKER + 1] = 0xFF,
};

/* frm_source_codes ($BAEC): b16 per frm_source_index (@$ bits 1-2; frm_finish_frame). */
const uint8_t fw_frm_source_codes[4] = {
    0x82,   /* 0: the default source */
    0x58,   /* 1: spike */
    0x9B,   /* 2: cut-down */
    0xBA,   /* 3: reduced HF */
};

/* frm_filter_codes ($BAF0): b17 per frm_filter_index (@$ bit 0; frm_finish_frame). */
const uint8_t fw_frm_filter_codes[2] = {
    0x4A,   /* 0: male */
    0x54,   /* 1: female */
};
