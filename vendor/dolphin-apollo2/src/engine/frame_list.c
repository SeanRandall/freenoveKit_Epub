#include <stdlib.h>
#include <string.h>
#include "engine/frame_list.h"

void frame_list_add(frame_list *fl, const uint8_t frame[ENGINE_FRAME_BYTES])
{
    if (fl->count == fl->cap) {
        size_t cap = fl->cap ? fl->cap * 2 : 256;
        void *grown = realloc(fl->frames, cap * sizeof fl->frames[0]);
        if (!grown)
            abort();
        fl->frames = grown;
        fl->cap = cap;
    }
    memcpy(fl->frames[fl->count++], frame, ENGINE_FRAME_BYTES);
}

void frame_list_free(frame_list *fl)
{
    free(fl->frames);
    fl->frames = NULL;
    fl->count = fl->cap = 0;
}

char *frame_list_hex(const frame_list *fl)
{
    static const char k_hex[] = "0123456789ABCDEF";
    char *s = malloc(fl->count * (ENGINE_FRAME_BYTES * 2 + 1) + 1);
    char *p = s;
    if (!s)
        abort();
    for (size_t i = 0; i < fl->count; i++) {
        for (int b = 0; b < ENGINE_FRAME_BYTES; b++) {
            *p++ = k_hex[fl->frames[i][b] >> 4];
            *p++ = k_hex[fl->frames[i][b] & 0x0F];
        }
        *p++ = '\n';
    }
    *p = '\0';
    return s;
}
