# C port of the email front end (patent US 5,634,084)

This is the part of TV_ENG32.DLL that neither OpenTV nor the Prose C covers: the abbreviation, acronym, number and
email front end (REFERENCE §3, §8.3-8.4). The goal is a faithful C port, verified call by call against the DLL.

| File | What |
|---|---|
| `textproc.h` | `TextProc`, byte for byte the DLL's 0x3008-byte object (offsets checked by `_Static_assert`). It starts with the paragraph text. |
| `frontend.h` | Prototypes. Every function names the VA of its original. |
| `fe_crt.c` | Allocation hooks, `strpos`, `strlwr`/`strupr`, `str_trim_spaces`, `str_replace_all`. |
| `fe_data.c/.h` | **Generated** by `tools/fe_extract.py` (data policy: table contents go only here). |
| `tokenize.c` | `Tokenize_SplitSeparators` (the word loop's end and separator search), `Tokenize_FirstPass` and `Tokenize_Dots` (REFERENCE §3.2b). |
| `paragraph.c` | The paragraph driver (patent 140; REFERENCE §3.11): `TextExpander_Paragraph` (text clean-up, the word loop, the expansion of each word), `Tokenize_JoinLetters` (spelled letters) and `Output_AppendText` (the output buffer). |
| `control.c` | The control procedure (patent 132; REFERENCE §3.12): `FrontEnd_ProcessText` (the entry: the input cut into lines, long ones into pieces) and `TextReaderControl` (per line: the email stage's code, then the paragraph added to, expanded or emitted). |
| `numline.c` | `FrontEnd_ProcessText`'s mode 2, which no caller uses: `NumLine_Read` and its 6 routines read each line's fractions, dates, minus signs and exponents as in a formula (REFERENCE §3.12). |
| `classify.c` | `ClassifyWord` (patent 136) and `Abbrev_LetterRange` (§3.2a). |
| `rules_dispatch.c` | `AbbrevRuleDispatch` (patent 148). |
| `rules_*.c` | The 33 rule handlers and their helpers (§3.4a). |
| `num_expander.c` | `NumberExpander` (patent 142; REFERENCE §3.8). |
| `num_*.c` | The number routines: `num_phone.c` (phone numbers), `num_digits.c` (special numbers, phone context, digit strings), `num_agree.c` (ranges and agreement), `num_dates.c` (fractions, dates, ordinals), `num_years.c` (years, ranges, QUAL context), `num_money.c`, `num_time.c`, `num_roman.c` (with `Classify_PeriodContext`). |
| `acr_*.c` | The acronym stage (patent 150; REFERENCE §3.9): `acr_expander.c` (`AcronymInitialismExpander`, `Acronym_CountUpper`), `acr_lists.c` (known lists and respellings, morph stripping, military ratings), `acr_syllable.c` (syllable parser, hyphen joiner), `acr_onset.c`, `acr_coda.c`. |
| `em_*.c` | The email stage (patent 134; REFERENCE §3.10): `em_header.c` (`EmailHeaderBodyAnalyzer`, `Email_Separators`, `str_trim_leading`), `em_from.c` (`Email_FromLine`, `Email_AddressList`, `Email_UncapWords`), `em_date.c` (`Email_DateLine`, `Email_DateMDY`, `Email_DateYMD`, `Email_OrdinalSuffix`). |
| `fe_bridge.c`, `num_bridge.c` | **Harness only:** run a function that is not ported yet from the loaded DLL. None is in use. |

## Verification
`tools/fediff/` loads the DLL and hooks `Tokenize_FirstPass`, `ClassifyWord`, `AbbrevRuleDispatch`, the 14 number
routines, the 10 acronym routines, the 9 email routines, `Tokenize_SplitSeparators`, `Tokenize_Dots`, the
paragraph driver with its two helpers, and the control procedure (`FrontEnd_ProcessText`, `TextReaderControl`, and
the 7 mode-2 routines, which run with `-mode 2`). At every call it deep-copies the `TextProc`, runs this C on the copy
and the original on the real object, and compares:
- every field and every live string;
- the return value and `*end`;
- the emphasis global and the email stage's header-keyword global (`fe_hdr_key`);
- for the paragraph driver and the control procedure, the text they append to the output buffer.

`sh run.sh` builds it and runs all corpora. Any mismatch fails the run. REFERENCE §8.8 has the details.

What matters about the harness:
- **The harness leaves the original alone.** The C port and the comparison run on their own stack, and all C-side
  memory comes from a private heap that exists for one call. The original reads uninitialised stack and freed
  blocks, and those bytes are then the same as in a run without the harness.
- **Where the original reads uninitialised stack**, the harness gives the port the real bytes (`fe_tok_stack`,
  `fe_cw_stack`, `fe_frame_of`). Without the harness they are zero. `NumberExpander` and the paragraph driver are
  checked after the original has run, so that their C callees get the frames the original's callees found.
- **Stale slots.** Word-table slots after `cur_word`, and slot 0, hold stale pointers. They are shared raw rather than
  copied. Slots that share one string keep sharing it in the copy. Copies are padded to 256 bytes, because the
  original writes past the end of some word strings.
- **Crashes and hangs of the original** (`Num_DigitString`, `State_Helper_B`, `Acronym_SyllableParse`,
  `Email_Separators`, `Email_FromLine`, `Email_AddressList`, `Tokenize_SplitSeparators`, `Tokenize_JoinLetters`) are
  caught. The line is
  abandoned and logged, a fresh copy of the DLL is loaded, and the run goes on.

## Bugs of the original: kept or fixed (`fe_fixes`)
REFERENCE §3.4c lists every defect found. The port has both behaviours, chosen at run time:
- **`fe_fixes = 1` (the default):** the defects are corrected. That covers stale next-word reads (the rule stage
  guards the two slots after the last word), reads in front of short strings (`fe_tail`), buffer run-ons,
  uninitialised stack read as data, `Rule_Range`'s wrong neighbour, the
  city-table loops that compare the state column, the `##` conjunction, the tokenizer's long-paragraph exit, and the
  number stage's in-place string growth, doubled punctuation, crashes and garbage reads, and the acronym stage's
  list and comparison slips (`#yo`, swapped `strpos` arguments, off-grid respelling keys) and frame overrun, and the
  email stage's stack overruns, colon rewrite, lost times, 19yy years and stale suffixes, and the paragraph
  driver's stale flags (sentence-end blanks and periods), spelled-letter overrun, wrong neighbour and lost output
  character, and the control procedure's settings line taken for the first line, lines lost after a long paragraph
  and long lines cut into a blank line (REFERENCE §3.4c).
- **`fe_fixes = 0`:** the port behaves exactly as the DLL. `tools/fediff` checks it this way. Every place that
  differs is marked `if (fe_fixes)`.

`fediff -fixed` (or `sh run.sh -fixed`) runs the fixed port against the DLL. Every mismatch is then a fix changing
the result. On all corpora, every difference was traced to one of the fixes.

Kept in both modes: `Rule_Direction2`'s suffix test, which never matches in the original. Ordinals already pass
through QUAL `ordnl`, and a working test would also accept `SW Smith St` (REFERENCE §3.4c).

Not reproduced in either mode:
- the stack overflows of `Rule_Measure4` and `Rule_Context_StreetTitleCity` (the port's buffers are large enough);
- the hangs of `State_Helper_B` and `Num_Agreement`'s mark stripping (the port ends the loops).


## Use in the back end (2026-09-29)
`tools/backend/build.sh` links all of this directory except the two bridges into `build/backend/tvsay.exe`, and
`src/backend/port/email.c` calls `fe_FrontEnd_ProcessText` as `tts_SaveFile` does in email context (`tvsay text
out.wav 1`; REFERENCE §9.7). With `tvsay -0` (`fe_fixes = 0`) the engine's text equals TV_ENG32's on 27,751 of the
28,734 inputs of the fediff corpora. The rest depend on memory the original reads uninitialised: the harness supplies
that memory, and a standalone run cannot. Most of them are `Tokenize_Dots`'s `prev`: a stale `.` sets `lc_period`
and adds a sentence-end blank. `fediff -whole` speaks a whole file as one text and finds 0 mismatches on all of them.

## Status (2026-09-28)
Ported and verified, with 0 mismatches over 35 corpora (every stage hooked at once). **The whole front end is
ported:** given the input text and an output buffer, `fe_FrontEnd_ProcessText` produces the text the DLL hands to the
engine, without the DLL.
- **The control procedure:** `FrontEnd_ProcessText` and `TextReaderControl`. 146,835 calls; also in modes 0, 2
  (`NumLine_*`), 3 and 4, which the DLL never uses (7 runs with `-mode N`, 19,664 calls).
- **The paragraph driver:** `TextExpander_Paragraph` with `Tokenize_JoinLetters` and `Output_AppendText`. 182,643
  calls.
- **The word stages:** `Tokenize_SplitSeparators`, `Tokenize_FirstPass` with `Tokenize_Dots`, then `ClassifyWord`
  with `Abbrev_LetterRange`. 451,446 tokenizer and classifier calls; 834,549 separator searches and `Tokenize_Dots`
  calls.
- **The rule stage:** the dispatcher and all 33 handlers with their helpers. 223,019 calls.
- **The number stage:** `NumberExpander` and its 15 routines. 566,350 calls.
- **The acronym stage:** `Acronym_MilitaryRating`, `AcronymInitialismExpander` and its 8 routines. 1,387,773 calls.
- **The email stage:** `EmailHeaderBodyAnalyzer` and its 8 routines (header keywords, From/To/Cc names, dates,
  separators). 230,435 calls.
- **Bug fixes** for all defects found so far, behind `fe_fixes`.

The corpora are tests_all, tests_edge, corpus_rules, corpus_citystate, corpus_dates, tests_classify, tests_tokenize,
corpus_classify, tests_numbers, corpus_numbers, the six tests_num_* files, tests_acronyms, corpus_acronyms, the
six tests_acr_* files, tests_email, corpus_email, the four tests_em_* files, tests_split, tests_para, corpus_para,
tests_ctl and corpus_ctl; the mode runs add tests_numline and corpus_numline. Everything builds and links without
the DLL.
