/* Converted from the Apollo 2 ROM by tools/extract_tables.py (M4.1 spec section 4), then reviewed.
   tests/test_firmware_data.c re-encodes it and compares it byte for byte with the ROM. */
#include <stddef.h>
#include "fw_data.h"

/* name, address, annotated size, counted text, tail (bytes after the count) */
const fw_phonetic_text fw_phonetic_texts[] = {
    { "cmd_version_engine_text", 0x9804, 32, "      TU    PyNf  ZERO  WQN   .", "" },
    { "boot_baud_text_19200", 0xACC7, 3, " ", "\015" },
    { "boot_baud_text_300", 0xACCA, 14, "tRE HuNDRxD ", "\015" },
    { "boot_baud_text_1200", 0xACD8, 16, "TWeLV HuNDRxD \015", "" },
    { "cmd_version_lang_text", 0xF75B, 32, "      Fo|   PyNf  ZERO  Fo|   .", "" },
};
const size_t fw_phonetic_text_count = sizeof fw_phonetic_texts / sizeof fw_phonetic_texts[0];
