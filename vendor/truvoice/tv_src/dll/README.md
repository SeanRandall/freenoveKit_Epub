# The replacement TV_ENG32.DLL

The C front end (`src/frontend`) and back end (`src/backend`) behind TruVoice's own API: a drop-in replacement for
`TV_ENG32.DLL`, with the same 24 `tts_*` exports (cdecl, `short` results) at the same ordinals. AGENTS.md TODO 14,
REFERENCE §10.

| File | What |
|---|---|
| `tts.c` | The exports, the handle table (at most 2 handles) and `DllMain`. Each export names the VA of its original. |
| `speak.c` | `tts_Speak`, `tts_Pause`, `tts_Resume`, `tts_Stop`: the text queue, the speaking thread (`Speak_Thread`), the wave device with its two buffers, and the end-of-speech callback. |
| `userdic.c` | The user dictionary: where it is kept (registry, `english.dic` beside the DLL), `tts_UserDic` and the pronunciation manager's three dialogs. The dictionary file itself is `src/backend/port/userdic.c`. |
| `tts.h` | The handle, shared by the three. |
| `tv_eng32.def` | Export names and ordinals, as in the original (generated once from it; no data). |
| `release/README.txt` | The release package's README, for users (`@VERSION@` is filled in by `tools/backend/release.sh`). |

`sh tools/backend/build.sh` builds `build/backend/dll/TV_ENG32.DLL` from the same objects as `tvsay.exe`. The
dialogs, bitmaps, icon, strings and version resource are the original's, copied from `TV_ENG32.DLL` at build time by
`tools/backend/gen_res.py` (nothing extracted is committed).

**Threads.** TV_ENG32's engines share Stage 2's working state, so two that synthesise at once garble each other
(REFERENCE §10.7). Here engine steps run one at a time, and each engine keeps its own copy of that state
(`tv_engine_step` in `src/backend/port/truvoice.c`). The C front end, whose buffers are static, runs one call at a
time. A single thread sees no change.

**Front-end fixes.** By default the email front end corrects the original's defects (REFERENCE §3.4c). With
`TRUVOICE_EXACT=1` in the environment when the DLL loads, it keeps them, and the output is TV_ENG32's (the setting
of `tvsay -0`).

## Release
`sh tools/backend/release.sh` builds everything, strips the DLL's debug information, checks the stripped DLL with
`apitest` (its log must be TV_ENG32's), and writes `build/release/truvoice-c-DATE-COMMIT/` (`TV_ENG32.DLL` and
`README.txt`, from `release/README.txt` with the version filled in) and a zip of it. The commit gets `-modified` when
`src` or `tools` have uncommitted changes. The package contains the tables and resources copied from TV_ENG32.DLL;
it carries no OpenTV licence (AGENTS.md, licence decision).

## Checking it against TV_ENG32
```sh
tools/probe/apitest.exe TV_ENG32.DLL build/api/dll.txt                              # the oracle's log (add -real for the sound card)
TRUVOICE_EXACT=1 tools/probe/apitest.exe build/backend/dll/TV_ENG32.DLL build/api/c.txt
diff build/api/dll.txt build/api/c.txt
python tools/backend/compare.py -d build/backend/dll/TV_ENG32.DLL -l tools/probe/tests_rules.txt   # tts_SaveFile
tools/probe/apitest.exe <DLL> log.txt -phon lines.txt                               # tts_Phoneme per line
tools/probe/apitest.exe <copy>/TV_ENG32.DLL log.txt -dic                             # the dictionary dialogs; english.dic beside the copy
tools/probe/apitest.exe <DLL> log.txt -stress 40                                    # repeated use; leaks on stdout
tools/probe/tvrace.exe <DLL> text.txt 10 [ctx [text2.txt]]                          # two engines at once
tools/probe/tvspeak.exe <DLL> "text" [ctx [options]]                                # listen: tts_Speak on the sound card
```

## Status (2026-09-29)
- [x] Step 2: exports, handles, settings, `tts_SaveFile`, `tts_Phoneme`, `tts_SpeakPhoneme`. The `apitest` log is
  identical to TV_ENG32's up to the first `tts_Speak`. `tts_SaveFile` through `tvwav.exe` gives the same results as
  `tvsay`: byte-identical on all probe test lines in both contexts and all rate/format combinations, and on the corpus
  apart from the known cases (REFERENCE §9.6, §9.7). `tts_Phoneme` is identical on 5,581 lines.
- [x] Step 3: `tts_Speak` and the sound card. The whole `apitest` log is identical to TV_ENG32's: speaking, queued
  texts, both contexts, `tts_SpeakPhoneme`, pause / resume / stop / close while speaking, `tts_Reset` while speaking,
  and all seven rate/format combinations (REFERENCE §10.5). With `-real` every utterance ends with its callback.
- [x] Step 4: the user dictionary and `tts_UserDic` (REFERENCE §10.6). With the same `english.dic` beside both DLLs,
  speech, `tts_Phoneme` over awkward dictionary files and the `-dic` script (every control of the three dialogs,
  saving, `tts_Close` with the manager open) log the same as TV_ENG32; the dialogs are pixel-identical.
- [x] Step 5: the whole DLL against TV_ENG32 (REFERENCE §10.7):
  - the same exports and imported system DLLs;
  - `apitest` is identical: the script, `-phon`, `-dic` and `-stress`, with flat handles, threads and memory;
  - `compare.py -d` is byte-identical on 2,806 of 2,811 inputs (every rate and format, both contexts; the other 5
    are REFERENCE §9.7's);
  - `tvspeak` plays the same through the sound card;
  - `tvrace`: the fix for engines running at once above.
