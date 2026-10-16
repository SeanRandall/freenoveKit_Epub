/* Mutable globals which lived in TV_ENG32.DLL's writable data image.
 * Desktop reconstruction builds can leave them in the extracted PE section;
 * embedded builds keep the much larger table image in flash and provide the
 * handful of values the engine actually modifies in ordinary RAM. */
#include <stdint.h>

#include "engine.h"

int32_t g_lexicon_dirty;
int32_t g_s2_count;
uint8_t g_s2_voiced;
Node *g_s2_last;
uint8_t g_s2_value;
int32_t g_s2_run;
int32_t g_s2_state;
uint8_t g_s2_hold;
int32_t g_s2_b15;
int32_t g_s2_saved;
Node *g_s2_node;
int32_t g_s2_total;
uint32_t g_s2_rnd;
