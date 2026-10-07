# DECtalk DTC-01 Reborn porting notes

Source candidate: `vendor/dectalk-dtc01-reborn`, pinned as a Git submodule.
It reconstructs DECtalk DTC-01 firmware version 1.8 as C and does not require
the original ROM images.

## Suitability for EVV Reader

The public API is a good match for the reader's speech-engine abstraction:

- `TextToSpeechSpeak`, `TextToSpeechSync` and `TextToSpeechReset` provide
  synthesis, completion and interruption;
- `TextToSpeechOpenInMemory`, `TextToSpeechAddBuffer` and
  `TextToSpeechReturnBuffer` can provide PCM without opening a desktop audio
  device;
- `TTS_MANUAL_CLOCK` allows the reader to drive synthesis without a POSIX
  worker thread;
- `TTS_MSG_INDEX_MARK` can supply sentence/navigation markers;
- rate is exposed as 120--350 words per minute;
- nine DTC-01 speaker slots are exposed;
- user dictionaries and DECtalk's inline command language are supported.

The engine produces signed 16-bit PCM at 10,000 Hz. A dedicated DECtalk
firmware variant should therefore set `EVV_ENGINE_SAMPLE_RATE` to 10000 and
feed returned buffers through the existing PCM callback. This avoids a
resampler and keeps the engine's native output intact.

## Work required

1. Create `speech_engine_dectalk.c` implementing the existing
   `speech_engine.h` entry points.
2. Build only the speech, API and minimal kernel sources; exclude SAY, the GUI,
   terminal emulator, desktop audio and telephone support.
3. Use manual-clock, in-memory output so the ESP32 build does not depend on
   pthreads, WinMM, ALSA or `dlopen`.
4. Map EVV settings to rate, speaker, volume and supported DECtalk voice
   parameters. Preserve inline annotations when the substitution dictionary
   contains them.
5. Translate reader markers to DECtalk index commands and return their sample
   positions through the abstraction callback.
6. Measure internal RAM, PSRAM, stack use and synthesis ratio on the ESP32-S3
   before enabling book playback. The upstream speech/API/kernel sources are
   approximately 1.1 MB of C source, so this is a substantial port rather than
   a drop-in library.

## Distribution status

No licence or notice file was present at the pinned upstream revision when it
was added. Keep the source candidate and any resulting binaries out of public
releases until the author confirms redistribution terms.
