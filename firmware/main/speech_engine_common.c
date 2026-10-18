#include "speech_engine.h"

/* Conservative defaults for adapters which have not supplied measured
   playback characteristics.  A strong adapter implementation overrides this
   weak definition without exposing an engine identity to the reader. */
__attribute__((weak))
void speech_engine_playback_hints(const speech_engine_t *engine,
                                  speech_engine_playback_hints_t *hints)
{
    (void)engine;
    if (!hints) return;
    hints->initial_buffer_ms = 4000;
    hints->navigation_buffer_ms = 1200;
}
