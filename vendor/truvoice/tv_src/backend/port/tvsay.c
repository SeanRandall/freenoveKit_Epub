/*
 * Command line for the C back end, the counterpart of tools/probe/tvwav.exe
 * (which runs TV_ENG32.DLL itself), with the same arguments minus the DLL:
 *
 *   tvsay [-0] [-t] "<text>|@file" out.wav [ctx [options]]
 *
 * In the text, \e is ESC and \n a line break.  ctx 1 is the email context
 * (tts_SetCtx 1: the front end, src/frontend, reads the text first).
 * options are tts_Open's, in hex: low word 1 for 8000 Hz, high word the
 * format (0 16-bit, 1 8-bit, 2 mu-law, 3 A-law).
 *   -0  the front end keeps the defects of the original (fe_fixes = 0), so
 *       the output is TV_ENG32's; by default they are fixed
 *   -t  write the text the engine gets (the prefix, through the front end
 *       in the email context) to out.wav instead of speaking it
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "truvoice.h"

extern int fe_fixes;    /* src/frontend/fe_crt.c */
extern void *(*fe_malloc_hook)(size_t);

/* With -0 the front end writes past the end of some word strings, as the
 * original does (REFERENCE 3.4c); its blocks get 256 spare bytes, as in
 * tools/fediff, so that this does not corrupt the heap. */
static void *padded_malloc(size_t n)
{
    return malloc(n + 256);
}

static char *load_text(const char *arg)
{
    char *s, *d, *t;

    if (arg[0] == '@') {
        FILE *f = fopen(arg + 1, "rb");
        long n;
        if (f == NULL)
            return NULL;
        fseek(f, 0, SEEK_END);
        n = ftell(f);
        fseek(f, 0, SEEK_SET);
        s = (char *)malloc((size_t)n + 1);
        n = (long)fread(s, 1, (size_t)n, f);
        s[n] = 0;
        fclose(f);
        return s;
    }
    t = d = (char *)malloc(strlen(arg) + 1);
    for (s = (char *)arg; *s; s++) {
        if (s[0] == '\\' && s[1] == 'e') {
            *d++ = 27;
            s++;
        } else if (s[0] == '\\' && s[1] == 'n') {
            *d++ = '\n';
            s++;
        } else {
            *d++ = *s;
        }
    }
    *d = 0;
    return t;
}

int main(int argc, char **argv)
{
    TvSettings s;
    FILE *f;
    char *text;
    int r, dump = 0;

    for (; argc > 1 && argv[1][0] == '-' && argv[1][1] != 0; argc--, argv++) {
        if (strcmp(argv[1], "-0") == 0) {
            fe_fixes = 0;
            fe_malloc_hook = padded_malloc;
        }
        else if (strcmp(argv[1], "-t") == 0)
            dump = 1;
        else
            break;
    }
    if (argc < 3) {
        fprintf(stderr, "usage: tvsay [-0] [-t] text|@file out.wav [ctx [options]]\n");
        return 1;
    }
    text = load_text(argv[1]);
    if (text == NULL) {
        fprintf(stderr, "cannot read %s\n", argv[1] + 1);
        return 1;
    }
    tv_settings_default(&s, argc > 4 ? (uint32_t)strtoul(argv[4], NULL, 16) : 0);
    if (argc > 3 && atoi(argv[3]) == 1)
        s.ctx = 4;                      /* tts_SetCtx(h, 1) */
    f = fopen(argv[2], "wb+");
    if (f == NULL) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 1;
    }
    if (dump) {
        char *full = (char *)malloc(strlen(text) + 0x28), *e;
        tv_build_prefix(text, full, &s);
        e = s.ctx == 4 ? tv_email_text(full) : full;
        fputs(e, f);
        fclose(f);
        return 0;
    }
    {
        TvOut o;
        tv_out_file(&o, f);
        r = tv_save_file(&s, &o, text, TV_FILE_WAVE);
    }
    fclose(f);
    if (r != 0) {
        fprintf(stderr, "tv_save_file %d\n", r);
        return 3;
    }
    return 0;
}
