# DoubleTalk Hybrid: drawing a practical boundary between emulation and native synthesis

## Introduction: why the hybrid exists

RC Systems' DoubleTalk PC was a self-contained speech synthesizer on an 8-bit ISA card. Its firmware ran on an Intel 80C188EB-compatible processor at approximately 10 MHz, accepted text and commands from a host computer, and drove an 8-bit digital-to-analogue converter at approximately 10.5 kHz. The card did not ask the host PC to understand English or construct speech. It performed the complete job itself.

That makes emulation an attractive preservation technique. If the original firmware is placed in a sufficiently accurate model of the card, it retains the dictionary, pronunciation rules, voices, prosody and peculiarities that made DoubleTalk recognisable. David Sexton's `doubletalk-pc` project does this with a standalone board model and a CPU core derived from MAME.

The complete emulator works comfortably on a modern Windows computer. It did not work comfortably on the ESP32-S3 used by Retroreader. Although the ESP32-S3 is vastly faster than the original DoubleTalk processor in raw terms, emulation is not a one-for-one comparison. Every 80C188 instruction has to be fetched, decoded and reproduced using multiple Xtensa instructions. Registers, flags, memory accesses, interrupts, timers and I/O devices must all be represented in software. The original firmware's acoustic renderer then executes this machinery repeatedly to produce individual DAC samples.

The ESP32 must do this while also running FreeRTOS, the reader interface, SD-card access, buffering, keypad handling and I²S audio output. Complete emulation ran substantially below real time, making it unsuitable for reading a book.

DoubleTalk Hybrid draws a boundary inside the original synthesis pipeline. It keeps the authentic firmware for language processing and replaces the computationally expensive acoustic back end with native ESP32 code. The objective is not byte-identical emulation. It is to preserve the parts that most strongly determine pronunciation and character while making continuous portable reading practical.

## The original card as a pipeline

The DoubleTalk PC can be understood as two broad stages:

1. **The linguistic front end** receives text and commands, normalises the text, consults dictionaries, determines pronunciation, chooses phonemes and constructs prosodic instructions.
2. **The acoustic back end** turns those instructions into timed waveform frames and writes the resulting samples to the DAC.

On the original card both stages are ordinary routines in the same firmware, running on the same processor. The division is conceptual rather than a hardware interface. Creating Hybrid required finding a stable point in the firmware at which the linguistic decisions were complete but the expensive sample production had not yet begun.

## End-to-end Hybrid flow

```text
Book or interface text
        |
        v
Retroreader's common speech-engine interface
        |
        v
DoubleTalk commands and text sent to the emulated card
        |
        v
Authentic firmware: dictionary, pronunciation and prosody
        |
        v
Compact DoubleTalk acoustic bytecode in emulated RAM
        |
        +---- emulator/native boundary ----+
        |                                  |
        v                                  v
Firmware acoustic call skipped     Native ESP32 renderer
                                           |
                                           v
                                  unsigned 8-bit PCM
                                           |
                                           v
                              modelled output stage
                                           |
                                           v
                                    signed 16-bit PCM
                                           |
                                           v
                         Retroreader buffering and I²S
```

## Stage 1: Retroreader prepares an utterance

Retroreader talks to every synthesizer through a common speech-engine interface. The DoubleTalk adapter converts the stored settings into commands understood by the original firmware. It currently supplies:

- voice;
- rate;
- rate boost;
- pitch;
- voice volume;
- tone;
- articulation;
- expression; and
- index markers used for navigation and interruption.

The settings and the text are assembled into an ASCII utterance terminated by a carriage return. Unsupported non-ASCII bytes are made safe before they can be mistaken for firmware control characters.

The adapter does not pass this text directly to the native renderer. It queues the bytes to the emulated card. That distinction is fundamental: native rendering begins only after the real DoubleTalk firmware has interpreted the text.

## Stage 2: the retained emulator

The retained board model contains the pieces needed to make the firmware believe that it is running on a DoubleTalk PC:

- a MAME-derived 80C188-compatible CPU core;
- the original 512 KB firmware ROM;
- 32 KB of emulated RAM;
- a host-to-card mailbox;
- the interrupt used to signal incoming bytes;
- firmware status registers;
- index-marker output; and
- the relevant I/O and timing behaviour.

The card model boots the ROM rather than jumping into a reconstructed function. Text is delivered through the mailbox, with the same ready/interrupt relationship expected by the firmware. The firmware parses its command language and performs its normal linguistic work.

This retained work includes the difficult and distinctive parts of DoubleTalk:

- text normalisation;
- number and abbreviation handling;
- dictionary lookup;
- letter-to-sound rules;
- phoneme selection;
- contextual pronunciation;
- stress and intonation decisions;
- punctuation handling;
- voice presets; and
- conversion into the firmware's compact acoustic instruction stream.

Keeping this stage is the difference between Hybrid and merely applying DoubleTalk-like waveform data to an unrelated text-to-phoneme engine. The authentic front end is responsible for much of what experienced listeners recognise as DoubleTalk behaviour.

## Stage 3: the emulator/native boundary

The board model installs an execution trace hook. During synthesis it watches for the firmware to reach physical address `0x849fc`, the call site at which a completed compact acoustic block is about to be dispatched to the ROM's acoustic renderer.

At that point Hybrid:

1. checks the firmware state indicating that a block is ready;
2. reads its length from emulated RAM;
3. copies the bytecode from the firmware's work area;
4. preserves front-end settings that are held elsewhere in RAM, notably articulation and expression;
5. keeps the previous block pending until the next block proves that an apparent terminator was only a block boundary;
6. sends the confirmed bytecode to the native renderer; and
7. skips the firmware call that would have generated the acoustic samples under emulation.

The one-block delay is important. A zero at the end of an internal block is not necessarily the end of the utterance. Hybrid waits until the firmware either supplies another block or reaches a settled idle state. This prevents words or acoustic chains from being cut at an arbitrary internal boundary.

The emulator continues to run after the call is skipped. It remains responsible for accepting more text, constructing further blocks, reporting status and producing index markers. Only the acoustic call has been replaced.

## Stage 4: native interpretation of the acoustic bytecode

The native renderer is written in C and compiled directly for the ESP32. It does not invent a new phoneme stream. It interprets the stream just produced by the firmware.

The bytecode contains phonemes and controls for such things as:

- requested pitch;
- relative pitch movement;
- rate selection;
- waveform-table banks;
- tone mode; and
- acoustic-chain state.

For each phoneme, the renderer consults the authentic ROM data to select context-dependent descriptor chains. The previous phoneme can affect the transition into the current one, so the renderer maintains state across a complete utterance rather than treating each phoneme as an isolated sound.

The descriptor scheduler then reproduces the important operations of the original acoustic routines:

- reading waveform fragments from the ROM;
- determining frame duration;
- repeating voiced or unvoiced frames;
- gliding the live pitch towards a target;
- selecting transformation tables;
- applying voiced lead-ins;
- joining adjacent frames;
- smoothing discontinuities with ramps; and
- preserving continuity state between descriptors.

The result is unsigned 8-bit PCM centred on `0x80`, matching the basic sample representation of the original card.

## Rate boost

Normal rate commands are interpreted by the authentic firmware. The optional rate boost requires cooperation from the native scheduler.

The complete emulator implements boost by altering a writable overlay of the ROM's rate-period table. In Hybrid, the native renderer reads the immutable ROM data directly, so it applies the equivalent percentage to selected duration values itself. The current maximum boost uses 80 per cent of the normal duration.

The scheduler clamps the result to the lowest safe period found in the authentic table. Earlier experiments showed that driving the firmware below this region could produce malformed frames or lose final consonants. The clamp provides extra speed without entering the original renderer's broken sub-minimum timing range.

## Stage 5: PCM and the ESP32 audio path

The renderer's unsigned 8-bit output is passed through a model of the card's output stage. This removes DC, applies reconstruction filtering and produces signed 16-bit PCM suitable for Retroreader's common audio interface.

The DoubleTalk engine then returns PCM in blocks of 2,048 samples. Retroreader decides what happens next. Depending on the activity, it can buffer the samples, associate them with navigation markers, interrupt them, cache them or send them to the Freenove board's I²S audio output.

This separation is useful beyond DoubleTalk: the speech engine produces PCM, while the reader owns playback policy.

## Cancellation and navigation markers

Index markers are still interpreted by the authentic firmware. Retroreader inserts a numbered marker command into the utterance, the emulated card exposes the marker through its expected I/O latch, and the adapter maps it back to the reader's navigation event.

On cancellation, Retroreader stops accepting PCM and sends the original DoubleTalk clear command to the emulated card. Pending acoustic data and output-filter history are discarded so that a later utterance does not begin with stale samples or a synthetic click.

## Comparison with the NVDA DoubleTalk add-on

The NVDA add-on and DoubleTalk Hybrid share a common ancestry but draw the boundary in different places.

### NVDA add-on flow

```text
NVDA speech sequence
        |
        v
Python synth driver
        |
        v
Compiled dtalk64.dll
        |
        v
Complete DoubleTalk card emulation
        |
        v
Emulated firmware acoustic renderer and DAC interrupts
        |
        v
Modelled 16-bit PCM output
        |
        v
NVDA WavePlayer
```

Python is not emulating the DoubleTalk processor. It provides the NVDA integration layer:

- translating NVDA settings and speech commands;
- loading the ROM and compiled DLL;
- queuing utterances on a worker thread;
- calling the DLL through `ctypes`;
- passing PCM to NVDA's WavePlayer;
- handling cancellation; and
- reporting index markers and completion.

The compiled DLL contains the MAME-derived CPU core and board model. On Windows it runs the complete firmware acoustic path, including the emulated timer-driven DAC writes. A modern desktop processor has enough headroom for this to be a negligible practical burden.

### Hybrid flow

```text
Retroreader text
        |
        v
C/C++ speech adapter
        |
        v
Emulated firmware linguistic front end
        |
        v
Native ESP32 acoustic renderer
        |
        v
Retroreader PCM callback and I²S
```

There is no Python on the ESP32. More importantly, the firmware's sample-producing routines are not emulated. The linguistic front end remains emulated; the acoustic back end runs as native Xtensa machine code compiled from C.

### Direct comparison

| Area | NVDA add-on | Retroreader DoubleTalk Hybrid |
|---|---|---|
| Host system | Modern Windows PC | ESP32-S3 embedded board |
| Integration layer | Python | C and C++ |
| CPU emulator | Compiled MAME-derived 80C188 core | Same family of compiled core |
| Firmware boot and text parsing | Emulated authentically | Emulated authentically |
| Dictionary and pronunciation | Original firmware | Original firmware |
| Prosody decisions | Original firmware | Original firmware |
| Acoustic bytecode | Produced and consumed by firmware | Produced by firmware, consumed natively |
| Frame rendering | Emulated firmware instructions | Native ESP32 C |
| DAC behaviour | Timer and DAC writes emulated | PCM generated directly |
| Output | NVDA WavePlayer | Retroreader buffers and I²S |
| Primary goal | Maximum hardware fidelity | Portable real-time performance |

## Why the ESP32 needs the boundary when the original card did not

The ESP32-S3 can execute far more native operations per second than a 10 MHz 80C188. It does not follow that it can emulate the entire card cheaply.

For a native instruction, the ESP32 performs the requested operation directly. For an emulated instruction, it must first determine which 80C188 instruction is represented, reproduce its registers and flags, translate its memory accesses, update virtual time and check for simulated interrupts. An original instruction can therefore expand into many host instructions and less cache-friendly memory activity.

The acoustic renderer is the worst possible place to pay this cost. It consists of short operations repeated continuously at audio rates, with simulated timer interrupts and DAC writes layered on top. Desktop processors hide the cost with very high clock speeds, large caches and substantial memory bandwidth. The ESP32 does not have that margin, especially while running the rest of the reader.

Hybrid uses emulation where authenticity has high value and native code where repetition makes emulation expensive.

## Fidelity and limitations

Hybrid is intended to preserve DoubleTalk's identity, but it should not be described as sample-identical hardware emulation.

The following remain authentic firmware behaviour:

- command parsing;
- dictionaries and pronunciation;
- phoneme and context selection;
- stress and intonation instructions;
- voice presets; and
- index-marker semantics.

Possible differences from the card or complete NVDA emulator include:

- exact frame timing;
- unusual transition cases;
- analogue coloration;
- filtering;
- commands not yet exposed by Retroreader;
- reverb and formant controls, which are not yet ported to the public Hybrid settings; and
- obscure firmware paths that have not yet appeared in ordinary reading.

The implementation also depends on known firmware addresses and RAM layouts. A different DoubleTalk firmware revision could move the interception point or internal buffers and would need to be analysed separately.

## Source map

The important Retroreader files are:

- `firmware/main/speech_engine_doubletalk.cpp` — the common speech-engine adapter, parameter mapping, utterance construction, PCM callback and logging;
- `vendor/doubletalk-pc-port/doubletalk/doubletalk_board.cpp` — the emulated board, address spaces, mailbox, I/O and firmware execution environment;
- `vendor/doubletalk-pc-port/doubletalk/dtalk.cpp` — the synthesis controller, trace hook and emulator/native hand-off;
- `vendor/doubletalk-native/doubletalk_utterance.c` — compact bytecode and phoneme interpretation;
- `vendor/doubletalk-native/doubletalk_scheduler.c` — context chains, duration, pitch and descriptor scheduling;
- `vendor/doubletalk-native/doubletalk_frame.c` — waveform transformation, joins and continuity ramps; and
- `vendor/doubletalk-native/doubletalk_voice.c` — voice-related ROM data handling.

The board and 80C188 emulation derive from David Sexton's `doubletalk-pc` project and its MAME-derived CPU core. DoubleTalk and its firmware originate with RC Systems.

## Suggested talk structure

1. Play a short sample from the NVDA add-on and the Hybrid.
2. Introduce the original ISA card as a complete computer dedicated to speech.
3. Explain why preserving firmware preserves more than just a voice sample.
4. Demonstrate why “240 MHz is faster than 10 MHz” is an incomplete emulation argument.
5. Walk through the complete-emulator pipeline used on Windows.
6. Identify the compact acoustic bytecode as the practical boundary.
7. Walk through the native frame renderer and PCM output.
8. Compare fidelity, performance and maintainability.
9. Discuss the value of retro voices to experienced screen-reader users.
10. End with live portable reading on the Freenove kit.

## Summary: the benefits of the hybrid design

DoubleTalk Hybrid retains the original firmware where authenticity matters most and replaces it where emulation costs the most.

Its principal benefits are:

- **Authentic language behaviour.** The original dictionaries, pronunciation rules, phoneme selection and prosody still run in the firmware.
- **Recognisable voice character.** The native renderer uses the firmware's own acoustic instructions and ROM waveform data rather than substituting an unrelated synthesizer.
- **Faster-than-real-time generation.** The repetitive acoustic loops run as native ESP32 code instead of expanded 80C188 emulation.
- **Practical portable reading.** The engine can coexist with the reader, SD card, controls and audio pipeline on constrained hardware.
- **Responsive integration.** PCM, cancellation and navigation markers fit Retroreader's common speech interface.
- **A useful preservation pattern.** The design demonstrates that an old engine need not be treated as either a completely opaque emulator or a total rewrite. A carefully chosen internal boundary can preserve its linguistic identity while replacing the part least suited to modern constrained hardware.

The NVDA add-on remains the better choice when a desktop computer can afford complete emulation and maximum hardware fidelity is the priority. DoubleTalk Hybrid exists for the different problem of carrying that familiar reading voice in a small, inexpensive device whose processor cannot afford to imitate every original instruction for every sample.
