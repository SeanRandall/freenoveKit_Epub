/*
 * The email context of TruVoice's drivers (REFERENCE 8.3, 9.7): what
 * tts_SaveFile 1005fb50 (and tts_Speak) do with the text when the handle's
 * context is 4 (tts_SetCtx 1), before the engine sees it.  The front end is
 * the C port in src/frontend.
 */
#include <stdlib.h>
#include <string.h>

#include "frontend.h"
#include "truvoice.h"

/* ESC[7F ESC[8F @0x100ff36c and ESC[7N ESC[8N @0x100ff360: the engine's
 * capitals/acronym handling (flag 7) and abbreviation lists (flag 8) are off
 * while it reads the front end's output (REFERENCE 8.5). */
static const char flags_off[] = "\x1b[7F\x1b[8F";
static const char flags_on[] = "\x1b[7N\x1b[8N";

/* Bytes above 0x7f become blanks, except the right quote 0x92, the pound
 * sign 0xa3, the yen sign 0xa5, (c) 0xa9 and (R) 0xae. */
/* @0x1005e830 */
static void mask_high_chars(char *s)
{
    int n = (int)strlen(s), i;

    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c > 0x7f && c != 0x92 && c != 0xa3 && c != 0xa5 && c != 0xa9 && c != 0xae)
            s[i] = ' ';
    }
}

/* The DLL takes the TextProc from its heap without clearing it (operator
 * new), and the C clears it.  FrontEnd_ProcessText sets the fields it reads
 * first; the slots the original reads past the last word (REFERENCE 3.4c)
 * then hold zeros, where the DLL has whatever its heap left there. */
char *tv_email_text(char *full)
{
    uint32_t n = (uint32_t)strlen(full) + 1;
    uint32_t size = n < 4000 ? 12000 : n * 3;
    char *out, *res;
    TextProc *t;

    out = (char *)calloc(size, 1);
    if (out == NULL)
        return NULL;
    t = (TextProc *)calloc(1, sizeof *t);
    if (t == NULL) {
        free(out);
        return NULL;
    }
    mask_high_chars(full);
    /* The C front end keeps its working buffers in static storage, where
     * the DLL's are on the stack or in this TextProc, so two threads must
     * not run it at once (two handles in email context, or tts_Speak and
     * tts_SaveFile together).  It is quick next to synthesis, so it is
     * simply serialised; one thread on its own sees no change. */
    tv_lock(TV_LOCK_FRONTEND);
    fe_FrontEnd_ProcessText(t, full, out, size, 1);
    tv_unlock(TV_LOCK_FRONTEND);
    res = (char *)malloc(strlen(out) + 0x1d);
    if (res != NULL) {
        strcpy(res, flags_off);
        strcat(res, out);
        strcat(res, flags_on);
    }
    free(out);
    free(t);
    return res;
}
