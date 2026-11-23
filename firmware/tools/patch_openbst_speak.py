#!/usr/bin/env python3
"""Patch OpenBST's src/speak.c so the engine can hand out samples as it makes them.

Why: stock bst_say() writes the whole utterance into one caller buffer, so the
caller needs RAM for all of it (about 120 KB for 5 seconds), and none of it can
be heard until all of it has been made. The engine itself works one
phrase at a time and only copies the samples to the buffer at the very end, so
this adds bst_say_stream(): the same synthesis, but each block of samples is
passed to a callback (return 0 to stop). The samples are identical to bst_say's.

It also takes away the two big working buffers that held a whole phrase:

- Frames. Stock OpenBST makes all of a phrase's audio frames into a buffer (8192 frames,
  128 KB, about a hundred seconds of speech) and only then turns them into samples. The frames
  are written once, in order, and never read back by the code that makes them, so each frame is
  now turned into samples the moment it is made (live_frame, called from frames.c's emit through
  bst_frame_hook; see patch_openbst_frames.py). No frame buffer is needed, and no phrase is ever
  cut short for lack of room.
- Samples of one frame. Stock OpenBST synthesises each frame into a 4096-sample buffer (8 KB) in
  one call, because a frame can in theory make up to 15 x 256 samples. live_frame instead calls
  the synthesiser again while the frame has periods left, which continues it exactly where it
  stopped, so the buffer can be small (BST_PCM_COUNT, 256 by default; a typical frame is about
  110 samples).

The phrase-event buffer stays adjustable with BST_MAXEMIT; the engine clamps to it.
bst_hw_frames and bst_hw_emits record the most any phrase has needed, for interest.

The samples are identical to the stock engine's: checked with bst_golden on the device.

Usage: patch_openbst_speak.py <input speak.c> <output speak.c>

Every replacement must match exactly once, or the script stops with an error,
so a change in OpenBST's source is noticed instead of silently ignored.
"""
import sys

REPLACEMENTS = [
    (
        '    uint8_t         frames[8192 * 16];\n'
        '    int16_t         pcm[4096];\n'
        '};\n',
        '    uint8_t         frames[16];   /* ESP port: unused, frames go to live_frame */\n'
        '    int16_t         pcm[BST_PCM_COUNT];\n'
        '\n'
        '    /* ESP port: streaming sink (see bst_say_stream) */\n'
        '    int           (*sink)(const int16_t *pcm, long n, void *ctx);\n'
        '    void           *sink_ctx;\n'
        '};\n'
        '\n'
        '/* ESP port: frames.c calls this for each frame as it is made (see patch_openbst_frames.py). */\n'
        'extern void (*bst_frame_hook)(const uint8_t *frame, void *ctx);\n'
        'extern void *bst_frame_hook_ctx;\n'
        '\n'
        'typedef struct {\n'
        '    bst       *h;\n'
        '    bst_synth *synth;\n'
        '    int16_t   *out;\n'
        '    long       max, used;\n'
        '    int        stop;\n'
        '} bst_live;\n'
        '\n'
        '/* Turns one frame into samples straight away, in pieces of at most BST_PCM_COUNT: the\n'
        '   synthesiser stops when the buffer is full and carries on from the same place when it is\n'
        '   called again, so the pieces join into exactly what one big call would have made. */\n'
        'static void live_frame(const uint8_t *frame, void *ctx) {\n'
        '    bst_live *L = ctx;\n'
        '    bst *h = L->h;\n'
        '    if (L->stop || !bst_synth_frame(L->synth, frame)) return;\n'
        '    const size_t cap = sizeof h->pcm / sizeof h->pcm[0];\n'
        '    size_t got;\n'
        '    do {\n'
        '        got = bst_synth_run(L->synth, h->pcm, cap);\n'
        '        if (h->sink) {\n'
        '            L->used += (long)got;\n'
        '            if (got > 0 && !h->sink(h->pcm, (long)got, h->sink_ctx)) { L->stop = 1; return; }\n'
        '        } else if (L->out) {\n'
        '            long room = L->max - L->used;\n'
        '            long take = (long)got < room ? (long)got : room;\n'
        '            if (take > 0) memcpy(L->out + L->used, h->pcm, (size_t)take * 2);\n'
        '            L->used += (long)got;\n'
        '        } else {\n'
        '            L->used += (long)got;\n'
        '        }\n'
        '    } while (got == cap && L->synth->periods_left > 0);\n'
        '}\n',
    ),
    (
        '        int m = bst_generate(&g);\n'
        '        if (m > g.maxout) m = g.maxout;\n'
        '\n'
        '        for (int i = 0; i < m; i++) {\n'
        '            if (!bst_synth_frame(&synth, h->frames + i * 16)) continue;\n'
        '            size_t got = bst_synth_run(&synth, h->pcm,\n'
        '                                       sizeof h->pcm / sizeof h->pcm[0]);\n'
        '            if (out) {\n'
        '                long room = max - used;\n'
        '                long take = (long)got < room ? (long)got : room;\n'
        '                if (take > 0) memcpy(out + used, h->pcm, (size_t)take * 2);\n'
        '                used += (long)got;\n'
        '            } else {\n'
        '                used += (long)got;\n'
        '            }\n'
        '        }\n',
        '        /* ESP port: each frame becomes samples as bst_generate makes it (live_frame), in\n'
        '           the same order as the stock loop over the frame buffer did. */\n'
        '        bst_live live = { h, &synth, out, max, used, 0 };\n'
        '        bst_frame_hook = live_frame;\n'
        '        bst_frame_hook_ctx = &live;\n'
        '        int m = bst_generate(&g);\n'
        '        bst_frame_hook = NULL;\n'
        '        bst_frame_hook_ctx = NULL;\n'
        '        used = live.used;\n'
        '        if (m > bst_hw_frames) bst_hw_frames = m;\n'
        '        if (live.stop) return used;   /* the caller asked to stop */\n',
    ),
    (
        '#define MAXEMIT 4096\n',
        '#ifndef BST_MAXEMIT\n'
        '#define BST_MAXEMIT 4096\n'
        '#endif\n'
        '#ifndef BST_PCM_COUNT\n'
        '#define BST_PCM_COUNT 256\n'
        '#endif\n'
        '#define MAXEMIT BST_MAXEMIT\n'
        'int bst_hw_frames, bst_hw_emits;   /* the most any phrase has needed so far */\n',
    ),
    (
        '        if (ne < 0) ne = MAXEMIT;\n',
        '        if (ne < 0) ne = MAXEMIT;\n'
        '        if (ne > bst_hw_emits) bst_hw_emits = ne;\n',
    ),
    (
        'long bst_length(bst *h, const char *text) {\n'
        '    if (!h || !text) return -1;\n'
        '    return run(h, text, NULL, 0);\n'
        '}\n',
        'long bst_length(bst *h, const char *text) {\n'
        '    if (!h || !text) return -1;\n'
        '    return run(h, text, NULL, 0);\n'
        '}\n'
        '\n'
        '/* Says the text, passing each block of samples to sink as it is made. Returns\n'
        '   the number of samples made, or -1 on a bad argument. If sink returns 0 the\n'
        '   speaking stops there. */\n'
        'long bst_say_stream(bst *h, const char *text,\n'
        '                    int (*sink)(const int16_t *pcm, long n, void *ctx), void *ctx) {\n'
        '    if (!h || !text || !sink) return -1;\n'
        '    h->sink = sink;\n'
        '    h->sink_ctx = ctx;\n'
        '    long n = run(h, text, NULL, 0);\n'
        '    h->sink = NULL;\n'
        '    h->sink_ctx = NULL;\n'
        '    return n;\n'
        '}\n',
    ),
]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src = open(sys.argv[1], encoding='utf-8', newline='').read()
    crlf = '\r\n' in src
    src = src.replace('\r\n', '\n')
    for old, new in REPLACEMENTS:
        count = src.count(old)
        if count != 1:
            sys.exit('patch_openbst_speak: expected exactly one match, found %d, for:\n%s' % (count, old))
        src = src.replace(old, new)
    if crlf:
        src = src.replace('\n', '\r\n')
    with open(sys.argv[2], 'w', encoding='utf-8', newline='') as f:
        f.write(src)


if __name__ == '__main__':
    main()
