# RSynth in RetroReader

This directory contains the C RSynth engine adapted from the historical
`master` branch of [github.com/rhdunn/rsynth](https://github.com/rhdunn/rsynth),
commit `c0304ce`. The original LGPL 2.1-or-later licence and notices are kept
alongside the source.

The ESP32 port renders signed 16-bit mono PCM at 16 kHz through RetroReader's
normal speech-engine interface. Rate, pitch, and engine volume are exposed in
the speech settings menu. Synthesis is streamed through a callback, so the
adapter does not need to hold an entire utterance in RAM.

This initial test port uses RSynth's built-in NRL letter-to-sound rules. The
CMU American and British pronunciation tables are deliberately not advertised:
upstream compiles several megabytes of dictionary data into the program, which
is a poor fit for every RetroReader image. An indexed SD-backed dictionary can
be added separately without changing the reader-facing engine interface.
