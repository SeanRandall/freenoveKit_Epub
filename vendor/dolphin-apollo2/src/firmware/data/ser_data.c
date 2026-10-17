/* Converted from the Apollo 2 ROM (M4.6a spec 4.6), then reviewed: the serial code's one table.
   tests/test_firmware_data.c re-encodes it and compares it byte for byte with the ROM. */
#include "fw_data.h"

/* ser_hex_chars ($C561, read as $C561,Y by ser_hex_digit): the hex digit of each nibble. The ROM has the 16
   characters only; the encoder (fw_data.c, step 11) leaves out C's terminating 0. */
const char fw_ser_hex_chars[17] = "0123456789ABCDEF";
