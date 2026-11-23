#!/usr/bin/env python3
"""Patch OpenBST's src/frames.c so each audio frame can be handed over as it is made.

Why: stock OpenBST writes every frame of a phrase into a buffer (8192 frames, 128 KB) and
only afterwards turns them into samples. emit() is the one place a frame leaves the
generator, it runs once per frame in order, and the generator never reads the buffer back.
So this adds bst_frame_hook: when it is set, emit() passes the frame to it instead of
copying it into the buffer. The patched speak.c (patch_openbst_speak.py) sets it to turn
each frame into samples at once, so the engine needs no frame buffer at all.

Usage: patch_openbst_frames.py <input frames.c> <output frames.c>

Every replacement must match exactly once, or the script stops with an error.
"""
import sys

REPLACEMENTS = [
    (
        'static void emit(bst_gen *g) {\n',
        '/* ESP port: when set, each frame goes here as it is made instead of into g->out. */\n'
        'void (*bst_frame_hook)(const uint8_t *frame, void *ctx);\n'
        'void *bst_frame_hook_ctx;\n'
        '\n'
        'static void emit(bst_gen *g) {\n',
    ),
    (
        '    if (g->nout < g->maxout) memcpy(g->out + g->nout * 16, g->frame, 16);\n',
        '    if (bst_frame_hook) bst_frame_hook(g->frame, bst_frame_hook_ctx);\n'
        '    else if (g->nout < g->maxout) memcpy(g->out + g->nout * 16, g->frame, 16);\n',
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
            sys.exit('patch_openbst_frames: expected exactly one match, found %d, for:\n%s' % (count, old))
        src = src.replace(old, new)
    if crlf:
        src = src.replace('\n', '\r\n')
    with open(sys.argv[2], 'w', encoding='utf-8', newline='') as f:
        f.write(src)


if __name__ == '__main__':
    main()
