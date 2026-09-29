/* The single language linked into the ESP32 viability build. */
#include "delta_lang.h"

extern delta_language delta_lang_enus;
void delta_lang_bind_enus(void);

const delta_language *const delta_languages[] = {
    &delta_lang_enus,
    0
};

void delta_lang_bind_all(void)
{
    delta_lang_bind_enus();
}
