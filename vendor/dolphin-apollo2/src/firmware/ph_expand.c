#include "ph_expand.h"
#include "data/fw_data.h"

#define PH_LENGTHEN_DIRECT  31   /* CMP #$1F: half counts from here on take a >> 1 instead of the table */

uint8_t ph_lengthen_count(fw *f, uint8_t a)
{
    if (a >= PH_LENGTHEN_DIRECT)
        return (uint8_t)(a >> 1);                              /* $D442: LSR A; TAY */
    return fw_rom8(f, (uint16_t)(PH_LENGTHEN_STEPS + a));      /* $D445: TAY; LDA $D44A,Y; TAY */
}
