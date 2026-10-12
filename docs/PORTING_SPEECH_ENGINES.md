# Porting speech synthesizers to Retroreader

This document describes the speech-engine boundary used by Retroreader, the
procedure followed for the existing ports, and the practical requirements a
new engine must meet. It is intended for developers bringing another classic
or modern synthesizer to the Freenove Media Kit for ESP32-S3.

## Architecture

The reader does not call a synthesizer directly. It calls the common API in
`firmware/main/speech_engine.h`. Exactly one implementation of that API is
linked into each firmware binary.

The engine adapter is responsible for:

- creating and destroying an engine instance;
- accepting UTF-8 text and producing signed 16-bit mono PCM;
- reporting the PCM sample rate accurately;
- passing PCM to the reader in reasonably small blocks;
- stopping promptly when the PCM callback returns `false`;
- exposing supported settings and translating reader values to native values;
- optionally reporting navigation markers at the correct audio position; and
- yielding often enough that the keypad, audio playback and watchdog continue
  to work.

The reader owns playback, buffering, SD caching, book navigation and UI
announcements. An adapter must not open I2S or play audio itself. This
separation allows the same reader to use engines with callback, pull, or
whole-utterance APIs.

Only one engine is linked because several ports have large code, voice data,
or working-memory requirements. Separate firmware binaries are therefore
normal and expected.

## Hardware and build constraints

The target is an ESP32-S3 with limited internal RAM and external PSRAM. A port
should assume:

- CPU-intensive synthesis must coexist with I2S playback and keypad scanning;
- large working buffers should use PSRAM via `heap_caps_malloc(...,
  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`;
- callback-visible or latency-critical buffers may need internal RAM;
- large constant tables and embedded voices must remain in flash/rodata;
- the reader consumes signed, little-endian, mono, 16-bit PCM;
- each adapter must return the engine's real sample rate;
- long uninterrupted loops can trip the watchdog or make the UI unresponsive;
- allocations proportional to a chapter can fail, so streaming is preferred;
  and
- proprietary engine code, voice ROMs and dictionaries must not be committed
  or distributed without permission.

The current ESP-IDF toolchain is used through `idf.py`. A typical build is:

```powershell
. C:\Users\sean\esp\esp-idf-v5.5\export.ps1
idf.py -C firmware -B build-myengine `
  -D EVV_APP_SOURCE=paragraph_reader_stable_main.c `
  -D EVV_SPEECH_ENGINE=myengine build
```

## Required adapter API

Create `firmware/main/speech_engine_<name>.c` and implement every declaration
from `speech_engine.h`.

### Lifecycle

`speech_engine_create()` receives callbacks and their context. Validate that a
PCM callback exists, allocate the native engine, set safe defaults, and return
`NULL` on any partial failure. `speech_engine_destroy()` must tolerate a
partially constructed object and release all native and adapter resources.

`speech_engine_name()` returns the spoken/display name. The short value from
`speech_engine_storage_id()` must be stable and unique. Reader preferences are
kept separately by this ID, so incompatible voice editions should use
different IDs, as OpenBST does for each build.

### PCM delivery and cancellation

Call:

```c
engine->config.pcm(samples, sample_count, engine->config.context)
```

for each block of PCM. A `false` result means the utterance has been superseded
and synthesis must stop as soon as safely possible. Do not keep delivering old
audio after cancellation. This behaviour is essential when moving quickly
through menus or status items.

Blocks of roughly 2,048 samples have worked well. Very small blocks increase
callback and SD overhead; very large blocks increase latency and make
interruption coarse.

An engine which generates an entire utterance first may allocate the result in
PSRAM and then pass it to the reader in smaller blocks. This is the approach in
`speech_engine_openbst.c`. Streaming engines should pass each filled native
buffer immediately, as in `speech_engine_openevv.c` and
`speech_engine_dectalk.c`.

Set and clear a local `busy` flag around synchronous synthesis. Implement
`speech_engine_busy()` and `speech_engine_wait()` according to the native API.
Avoid holding reader or audio locks while generating speech.

### Markers

Markers let the reader associate a PCM position with a sentence or navigation
point. If the native engine supports index marks, insert the mark into the
native text stream and invoke the reader's marker callback when synthesis
reaches its corresponding sample.

If native marks are unavailable, retain the pending marker and emit it just
before the next generated PCM. This is less precise but still supports basic
navigation. Pico, OpenBST and Monologue currently use this fallback.

Never announce a marker only when synthesis begins if the engine buffers a
large amount of earlier audio: the marker must describe the PCM being emitted,
not merely the text being submitted.

### Settings

Return only settings the engine genuinely supports. Each
`speech_setting_info_t` supplies:

- the common setting ID;
- a spoken name;
- minimum, maximum and step;
- default value;
- whether it is a labelled boolean; and
- optional labels such as `male`/`female` or `off`/`on`.

The reader's scale does not have to equal the native scale. Translate it in the
adapter and log both values during development. For example, OpenBST's native
rate direction is inverted, so its adapter maps reader rate 60–140 to a safe
native range. Monologue similarly maps its reader rate and pitch onto compact
native indices.

`speech_engine_set()` must reject unsupported IDs and out-of-range values.
`speech_engine_get()` must return the actual stored reader-facing value. Apply
settings before every synthesis only when the native engine requires it;
otherwise apply them when changed.

Use a separate `speech_engine_storage_id()` when two editions interpret the
same numerical settings differently.

### Text, annotations and dictionaries

The reader passes UTF-8 text. Convert it in the adapter if the native engine
expects another encoding. Do not silently reinterpret UTF-8 bytes as an
8-bit legacy code page.

Before reaching the engine, reading text is processed by the optional user
file `/sdcard/.evv/substitutions.tsv`. This reader-level substitution feature
is independent of an engine's built-in pronunciation dictionary.

Return `true` from `speech_engine_supports_annotations()` only when replacement
text may safely contain that engine's inline command syntax. Otherwise the
reader treats replacement text as ordinary speech. Native dictionary controls,
when supported, should be exposed separately through
`SPEECH_SETTING_NATIVE_DICTIONARY`.

Escape or disable markup where required. Pico, for example, wraps text in its
control markup to apply rate, pitch and volume, so raw text must not be allowed
to break that wrapper. DECtalk-style inline commands require similarly careful
handling.

## Adding the engine to CMake

Add a new branch to `firmware/main/CMakeLists.txt`:

```cmake
elseif(EVV_SPEECH_ENGINE STREQUAL "myengine")
    set(MYENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../vendor/myengine")
    set(MYENGINE_SOURCES
        ${MYENGINE_ROOT}/src/engine.c
        ${MYENGINE_ROOT}/src/render.c)
    list(APPEND APP_SOURCES
        "speech_engine_myengine.c"
        ${MYENGINE_SOURCES})
    set(SPEECH_INCLUDE_DIRS "${MYENGINE_ROOT}/include")
endif()
```

Then add only the compile definitions and warning exceptions genuinely needed
by that legacy source. Prefer source-specific definitions over target-wide
ones. A target-wide `__linux__` definition once caused generated embedded WAV
assembly to put hundreds of kilobytes of tones in internal RAM instead of
flash. The DECtalk 4.3 port scopes that compatibility definition to its C
sources.

If the engine needs voice or language resources, either:

1. compile constant data directly into flash;
2. use `APP_EMBED_FILES` and copy the resource to `.evv` on first boot; or
3. document an SD resource package and fail with a clear error when absent.

Pico embeds its linguistic resources, installs them under `.evv/pico`, and
loads them from there because its native API expects files. DECtalk 4.3 uses
the same pattern for its dictionary. Monologue embeds a packed voice image.

Do not define platform macros globally merely to make old code compile. Check
their effect on ESP-IDF headers, generated assembly, structure sizes and linker
placement.

## Patterns used by existing ports

### OpenEVV: native callback streaming

`speech_engine_openevv.c` gives the engine a 2,048-sample output buffer and
registers a callback. Waveform callbacks are forwarded directly to the reader;
index callbacks become reader markers. Returning the engine's abort code when
the reader callback returns `false` provides good interruption.

This is the preferred model when an engine has a reliable streaming API.

### Pico: pull loop with PSRAM working memory

`speech_engine_pico.c` reserves approximately 2.5 MB of PSRAM, submits UTF-8
text, repeatedly pulls PCM, and yields periodically. Language resources are
embedded and installed on the SD card. Rate, pitch and volume are applied using
Pico's markup.

This demonstrates an engine which needs a large arena and synchronous polling.

### OpenBST: whole utterance followed by chunked delivery

`speech_engine_openbst.c` asks the engine for the required sample count,
allocates the complete PCM result in PSRAM, renders it, then delivers it in
2,048-sample blocks. Native rate is inverted and constrained to avoid unstable
extremes.

This is acceptable for short UI speech, but very large utterances require more
memory and cannot begin playback until rendering finishes. Prefer a streaming
native entry point when one exists.

### DECtalk DTC-01: in-memory native buffers

`speech_engine_dectalk.c` opens DECtalk in memory-only/manual-clock mode,
queues PCM and index buffers, advances the engine in small steps, and forwards
completed buffers. On cancellation it discards subsequent PCM while allowing
the old emulated kernel to unwind safely.

This demonstrates adapting a callback API whose reset operation is unsafe from
inside its own callback.

### Monologue: legacy PCM conversion

`speech_engine_monologue.c` converts text to phonetics, pulls native PCM, and
converts 8-bit unsigned or 16-bit native samples into the reader's signed
16-bit format. Volume is applied during conversion. It also demonstrates
mapping compact legacy parameter ranges into consistent reader settings.

### DECtalk 4.3: software DECtalk with an ESP compatibility layer

The 4.3 adapter follows the in-memory DECtalk pattern, adds an ESP
compatibility layer for dormant desktop audio calls, and installs its required
dictionary from embedded data. Its upstream source is referenced through the
`vendor/dectalk43` submodule; ESP32-specific overrides live in
`firmware/main/dectalk43_overrides`.

## Performance requirements

A technically correct voice can still be unusable. Test and log:

- generation speed versus playback duration at several rates;
- time from a key press to first audible PCM;
- interruption latency during long announcements;
- repeated rapid menu, library and status presses;
- sustained chapter reading, including section boundaries;
- cancellation followed immediately by a different utterance;
- silence after pause, ensuring stale PCM is not replayed;
- stack high-water marks and internal/PSRAM free space;
- watchdog behaviour during long synthesis;
- cold boot, resume, cache regeneration and SD errors; and
- recording and network modes, which compete for memory even if the speech
  engine is not currently reading a book.

The practical target is synthesis faster than real-time at the rates users
need. Small yields should preserve responsiveness without dropping below
real-time. Logging should include input length, generated samples, sample rate,
elapsed synthesis time, cancellation, and allocation failures.

## Hardware test checklist

Before considering a port ready:

1. Boot with a populated SD card and with required resources absent.
2. Open a small and a very large EPUB, plus a plain-text file.
3. Read continuously for at least several minutes at slow, normal and fast
   rates.
4. Change every exposed setting and confirm its spoken value and audible
   effect agree.
5. Move rapidly through speech settings and verify old announcements stop.
6. Pause, resume, move by sentence/heading/section, and cross a section
   boundary.
7. Enter and leave library/settings while speech is active and paused.
8. Exercise recording, transfer and remote modes for memory regressions.
9. Reboot and verify settings remain separate from other engine variants.
10. Capture the serial log for crashes, watchdog resets, underruns and stale
    audio.

## Preparing a website sample

Generate the sample with the same engine version, voice and parameter mapping
as the firmware—not an unrelated desktop imitation. Use a conservative public
rate and this standard wording:

> This is the NAME speech synthesizer, speaking at rate RATE. Select me during
> installation to have your electronic books and documents spoken by me whilst
> you are out-and-about carrying the Retroreader.

Render mono PCM at the native sample rate, check for clipping, and encode the
website copy to MP3. Keep samples private until the firmware has been heard on
the Freenove hardware and licensing permits publication.

## Licensing and repository hygiene

Before publishing source, firmware or samples, identify separately the licence
for:

- engine source;
- compiled engine code;
- voice ROM/data;
- dictionaries and language resources; and
- generated audio samples.

An open-source wrapper does not automatically grant rights to redistribute a
voice or dictionary. Keep uncertain material outside tracked public paths,
exclude private build trees and artefacts locally, and review a firmware image
for embedded proprietary data before release.

