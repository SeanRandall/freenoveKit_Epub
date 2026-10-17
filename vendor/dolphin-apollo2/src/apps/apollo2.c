/* apollo2: text -> speech through the Apollo 2 firmware, run as native C on a model of the board (no ROM, no emulated
   CPU), and a Holmes-style parallel-formant synthesizer at 10 kHz (spec R1, R2). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio/playback.h"
#include "audio/wav.h"
#include "render/render_native.h"

#define APOLLO2_MAX_TEXT 1500   /* keeps the speech inside the native engine's speaking budget, NATIVE_SPEAK_CYCLES
                                   (about 200 s) */

static native_engine g_engine;

static int usage(void)
{
    fprintf(stderr,
            "usage: apollo2 [options] TEXT\n"
            "       apollo2 [options] --text-file FILE\n"
            "  -o FILE            write a WAV file (10 kHz, mono, 16-bit) instead of playing live\n"
            "  --voice a|b        speaker table: a = male (default), b = non-male (@K1)\n"
            "  --hyp LIST         synthesizer hypothesis switches, comma-separated: b11,b13\n"
            "  --stream S         frame stream fed to the synthesizer: produced (default) or raw\n"
            "  --prosody N        intonation strength 0-7, sent as @Rn (default 4 = the firmware's\n"
            "                     unscaled contour; the device's own default is 3)\n"
            "  --text-file FILE   read the text from FILE (first line; device control codes allowed)\n"
            "  --audio-check      report whether live 10 kHz playback is available, then exit\n"
            "  --                 end of the options: the next argument is TEXT even if it starts with -\n"
            "TEXT may contain the device's @ commands (the device manual, section 6); at most %d characters.\n"
            "exit codes: 0 ok, 1 usage/file error, 2 firmware fault, 3 cycle limit, 4 audio error, 5 unmodelled access logged\n",
            APOLLO2_MAX_TEXT);
    return 1;
}

/* The whole first line of a file (up to CR, LF or the end), NUL-terminated in a malloc'ed buffer, with its length in
   *len (a NUL byte in the line makes strlen shorter than *len); NULL if unreadable. Caller frees. */
static char *read_text_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    size_t cap = 256, n = 0;
    char *buf = malloc(cap);
    if (!buf)
        abort();
    for (int c; (c = getc(f)) != EOF && c != '\r' && c != '\n';) {
        if (n + 1 == cap && !(buf = realloc(buf, cap *= 2)))
            abort();
        buf[n++] = (char)c;
    }
    fclose(f);
    buf[n] = '\0';
    *len = n;
    return buf;
}

static bool parse_hyp(const char *list, synth_options *o)
{
    char buf[64];
    if (strlen(list) >= sizeof buf)
        return false;
    strcpy(buf, list);
    for (char *t = strtok(buf, ","); t; t = strtok(NULL, ",")) {
        if (!strcmp(t, "b11"))
            o->b11_pulse_width = true;
        else if (!strcmp(t, "b13"))
            o->b13_reset = true;
        else
            return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    const char *out_path = NULL, *text = NULL, *text_file = NULL;
    char *file_text = NULL;
    size_t text_len;
    bool options_done = false;   /* after "--", every argument is TEXT */
    render_options opt;
    render_default_options(&opt);

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *val = i + 1 < argc ? argv[i + 1] : NULL;
        if (options_done) {
            if (text)
                return usage();
            text = a;
        } else if (!strcmp(a, "--")) {
            options_done = true;
        } else if (!strcmp(a, "--audio-check")) {
            bool ok = playback_supported(SYNTH_RATE);
            printf("live playback at %d Hz: %s\n", SYNTH_RATE, ok ? "available" : "NOT available");
            return ok ? 0 : 4;
        } else if (!strcmp(a, "-o") && val) {
            out_path = val;
            i++;
        } else if (!strcmp(a, "--voice") && val && (!strcmp(val, "a") || !strcmp(val, "b"))) {
            opt.voice = val[0];
            i++;
        } else if (!strcmp(a, "--hyp") && val) {
            if (!parse_hyp(val, &opt.synth))
                return usage();
            i++;
        } else if (!strcmp(a, "--stream") && val && (!strcmp(val, "produced") || !strcmp(val, "raw"))) {
            opt.stream = !strcmp(val, "raw") ? RENDER_STREAM_RAW : RENDER_STREAM_PRODUCED;
            i++;
        } else if (!strcmp(a, "--prosody") && val && val[0] >= '0' && val[0] <= '7' && val[1] == '\0') {
            opt.prosody = val[0] - '0';
            i++;
        } else if (!strcmp(a, "--text-file") && val) {
            text_file = val;
            i++;
        } else if (a[0] == '-' && a[1] != '\0') {
            return usage();   /* an unknown option, the M5a CLI's --engine, --rom and --rom-any included */
        } else if (!text) {
            text = a;
        } else {
            return usage();
        }
    }
    if ((text == NULL) == (text_file == NULL))
        return usage();
    if (text_file) {
        if (!(text = file_text = read_text_file(text_file, &text_len))) {
            fprintf(stderr, "cannot read %s\n", text_file);
            return 1;
        }
        if (strlen(text) != text_len) {   /* refused rather than cut off there without a word */
            fprintf(stderr, "the text contains a NUL byte (the first line of %s)\n", text_file);
            return 1;
        }
    } else {
        text_len = strlen(text);
    }
    if (text_len == 0) {
        fprintf(stderr, "no text to speak (TEXT or the first line of the file is empty)\n");
        return 1;
    }
    if (text_len > APOLLO2_MAX_TEXT) {
        fprintf(stderr, "text too long: %zu characters (at most %d)\n", text_len, APOLLO2_MAX_TEXT);
        return 1;
    }

    render_result r;
    engine_status st = render_text_native(&g_engine, text, &opt, &r);
    if (st == ENGINE_FAULT) {   /* no samples: nothing is written or played */
        board_print_fault(&g_engine.b, stderr);
        board_print_unmodelled(&g_engine.b, stderr);
        return 2;
    }
    if (st == ENGINE_CYCLE_LIMIT) {
        fprintf(stderr, "cycle limit reached before the firmware went idle\n");
        return 3;
    }
    if (r.clipped)
        fprintf(stderr, "warning: %llu samples clipped\n", (unsigned long long)r.clipped);
    int rc = 0;
    if (g_engine.b.unmodelled_count || g_engine.b.unmodelled_dropped) {   /* spec section 4: never accept silently */
        fprintf(stderr, "the firmware made accesses the board model does not cover:\n");
        board_print_unmodelled(&g_engine.b, stderr);
        rc = 5;
    }
    /* A failed write (1) or playback (4) replaces a 5: no output reached the user, which matters more; both messages
       are printed. */
    if (out_path) {
        if (!wav_write(out_path, r.samples, r.count, SYNTH_RATE)) {
            fprintf(stderr, "cannot write %s\n", out_path);
            rc = 1;
        }
    } else if (r.count > 0) {
        char err[128];
        if (!playback_pcm16(r.samples, r.count, SYNTH_RATE, err, sizeof err)) {
            fprintf(stderr, "live playback failed: %s\n", err);
            rc = 4;
        }
    }
    render_result_free(&r);
    free(file_text);
    return rc;
}
