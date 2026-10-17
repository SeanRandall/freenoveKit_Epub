![](apollo2.png)

# Dolphin Apollo II Reborn

A decompilation of the Dolphin Apollo 2 speech synthesizer. The original firmware is ported to C, and its DSP frames match the ROM byte for byte; a Holmes-style synthesizer turns them into 10 kHz audio.

The DSP program has been rewritten from scratch as the original NEC ROM has been lost to time.

The program needs no main ROM to run.

Please note that Claude Opus 5.5 was used to decompile the synthesizer, and some info may be verbose despite vigorous testing. If you notice anything off, feel free to do a pull request or raise an issue.

## Build and test

Requires MSVC 2022 Build Tools. From Git Bash in the repo root:

    cmd //c ".\build.bat"                 # build only
    cmd //c ".\build.bat test"            # build and run every test suite (about 16 minutes)
    cmd //c ".\build.bat test cpu"        # only the suites whose name contains "cpu"
    cmd //c ".\build.bat test-norom"      # every suite as if no ROM existed, then the program itself

The ROM (`xtras/UK_V2.01_4.04_ROM.BIN`) is used only by the tests and the oracle tools, which find it there; `APOLLO2_ROM` can point them at another copy. Without it, the tests that need it print SKIP.

`cmd //c ".\build.bat dist"` builds only the program and packs it into `dist\apollo2-COMMIT-win64.zip`, with no tests.

## Usage

    build/apollo2.exe "Hello world."                  # play live
    build/apollo2.exe -o hello.wav "Hello world."     # write a WAV (10 kHz, mono, 16-bit)
    build/apollo2.exe --voice b "Hello world."        # second speaker table (@K1)
    build/apollo2.exe --prosody 3 "Hello world."      # intonation 0-7 (@Rn); default 4, the device's own is 3
    build/apollo2.exe "@W5 @F6 Faster and lower."     # device commands (xtras/apollo2.txt section 6)
    build/apollo2.exe --text-file story.txt -o story.wav
    build/apollo2.exe -o minus.wav -- "-5 degrees."   # -- ends the options
    build/apollo2.exe --audio-check                   # is live 10 kHz playback available?

`--hyp b11,b13` and `--stream raw` are for listening tests (`xtras/calibration/report.md`). Exit codes: 0 ok, 1 usage or file error, 2 firmware fault, 3 cycle limit, 4 audio error, 5 unmodeled access logged.

## Layout

| Path                      | Contents                                                                                                 |
| ------------------------- | -------------------------------------------------------------------------------------------------------- |
| `src/firmware`            | The firmware in C; `data/` holds its converted tables                                                    |
| `src/board`, `src/engine` | The CPU-less board (timers, UART, ports, DSP port) and the engine that runs the C on it                  |
| `src/synth`, `src/audio`  | The synthesizer; WAV output and live playback                                                            |
| `src/render`, `src/apps`  | Text to audio, and the `apollo2` program (built from `src/` alone)                                       |
| `tests/`                  | The test suites, the corpus texts and the golden frame streams (`tests/golden/`)                         |
| `tests/oracle`            | The test oracle: a 740 emulator that runs the ROM, and a hybrid engine that swaps in the C               |
| `tools/`                  | `extract_tables.py` (converted tables), `gen_fw_ram.py` (`fw_ram.h`), `calibrate.py` (synth calibration) |
| `ghidra740/`              | A Ghidra MELPS 740 module and the annotations that rebuild a labelled ROM project (see its README)       |

## How it is checked

The emulator runs the original ROM and records the golden frame streams. The native engine must reproduce every one byte for byte without the ROM (`test golden_native`), and the diff suites compare the C with the ROM routine by routine and run by run (`diff_lts`, `diff_play`, `diff_serial`, `diff_main`, `diff_native`).
Each suite is marked in `tests/suites.h` as ROM-free or as needing the ROM.

The oracle tool `apollo2_frames` prints the ROM's frames for a text (it refuses an unknown ROM unless given `--rom-any`):

    build/apollo2_frames.exe "Hello world."       # one DSP frame per line (hex)
    build/apollo2_frames.exe --boot "Hi"          # include the startup announcement

Regenerate the goldens only after reviewing an intentional emulator change:

    APOLLO2_UPDATE_GOLDEN=1 cmd //c ".\build.bat test golden"
