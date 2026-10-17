#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render/render_native.h"
#include "engine/native_engine.h"

void render_default_options(render_options *opt)
{
    memset(opt, 0, sizeof *opt);
    opt->voice = 'a';
    opt->stream = RENDER_STREAM_PRODUCED;
    opt->prosody = 4;
    synth_default_options(&opt->synth);
}

void render_result_free(render_result *r)
{
    free(r->samples);
    memset(r, 0, sizeof *r);
}

char *render_input(const char *text, const render_options *opt)
{
    char prefix[16];   /* at most "@K1 @R7 " */
    int plen = snprintf(prefix, sizeof prefix, "%s", opt->voice == 'b' ? "@K1 " : "");   /* manual §6 @Kb */
    if (opt->prosody >= 0 && opt->prosody <= 7)                                       /* manual §6 @Ro */
        plen += snprintf(prefix + plen, sizeof prefix - (size_t)plen, "@R%d ", opt->prosody);
    size_t n = strlen(text);
    char *input = malloc((size_t)plen + n + 1);
    if (!input)
        abort();
    memcpy(input, prefix, (size_t)plen);
    memcpy(input + plen, text, n + 1);
    return input;
}

void render_synthesize(const frame_list *fl, const render_options *opt, render_result *out)
{
    memset(out, 0, sizeof *out);
    if (fl->count == 0)
        return;
    synth s;
    synth_options so = opt->synth;
    so.f0_centre_hz = params_f0_centre(&fl->frames[0][0], fl->count);   /* pitch-range centre */
    synth_init(&s, &so);
    out->samples = malloc(fl->count * SYNTH_FRAME_SAMPLES * sizeof out->samples[0]);
    if (!out->samples)
        abort();
    for (size_t i = 0; i < fl->count; i++)
        synth_frame(&s, fl->frames[i], out->samples + i * SYNTH_FRAME_SAMPLES);
    out->frames = fl->count;
    out->count = fl->count * SYNTH_FRAME_SAMPLES;
    out->clipped = s.clipped;
}

engine_status render_text_native(native_engine *e, const char *text, const render_options *opt, render_result *out)
{
    char *input = render_input(text, opt);
    frame_list produced = { 0 }, raw = { 0 };
    bool want_raw = opt->stream == RENDER_STREAM_RAW;
    engine_status st = native_engine_speak(e, NULL, input, false, &produced, want_raw ? &raw : NULL);
    memset(out, 0, sizeof *out);
    if (st == ENGINE_IDLE)
        render_synthesize(want_raw ? &raw : &produced, opt, out);
    frame_list_free(&produced);
    frame_list_free(&raw);
    free(input);
    return st;
}
