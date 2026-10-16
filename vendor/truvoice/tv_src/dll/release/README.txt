TruVoice TV_ENG32.DLL, rebuilt in C
====================================

A drop-in replacement for TV_ENG32.DLL, Centigram's TruVoice English
text-to-speech engine for 32-bit Windows (1997). It has the same 24 tts_*
functions at the same ordinals, and it says the same things: it has been
checked sample for sample against the original DLL.


Installing
----------
1. Find the TV_ENG32.DLL your program uses (usually in the program's own
   directory, or in the Windows system directory).
2. Keep a copy of the original.
3. Put this TV_ENG32.DLL in its place.

It is a 32-bit DLL, for 32-bit programs, and needs nothing but Windows itself.
Nothing is installed and nothing is written to the registry, except by the
Pronunciation Manager when you save a dictionary (as the original does).


What is the same
----------------
- The API: every function, its checks and its error codes.
- The audio: tts_Speak and tts_SaveFile give the original's samples, in every
  rate (11025 and 8000 Hz), format (16-bit, 8-bit, mu-law, A-law), file type
  (WAVE, raw, AU), voice and setting, and for every escape sequence.
- tts_Phoneme's spellings, the callbacks, pause, resume and stop.
- The user dictionary: the file named in the registry
  (HKEY_CURRENT_USER\Software\Centigram\TextToSpeech\Dictionary, value
  ENGLISH), otherwise english.dic beside the DLL; and the Pronunciation
  Manager (tts_UserDic), with the original's dialogs. Its Help button opens
  cgrm_en.hlp beside the DLL; that file is not included.
- The version information (5.1.0.16), so programs that check it see no
  change.


What is different
-----------------
- The email reader (tts_SetCtx 1) has its bugs fixed: wrong expansions,
  garbled or dropped words, and inputs that crashed or hung the original.
  To get the original's exact behaviour instead, set the environment variable
  TRUVOICE_EXACT=1 for the program before it loads the DLL.
- Two voices at once work. In the original, speech on one handle while the
  other handle is speaking or saving a file (or two files saved from two
  threads) could come out garbled, because the two share part of the
  engine's working state. Here each has its own. One voice at a time sounds
  exactly as before.
- In the email reader, a few long texts can differ slightly from the
  original even with TRUVOICE_EXACT=1, where the original read memory it had
  never set (its output there changes from run to run).


How it was made
---------------
The email reader was ported to C from TV_ENG32.DLL itself, routine by
routine. The speech engine is adapted from OpenTV, a C decompilation of
TruVoice's sibling build CGRM_EN.DLL, moved over to TV_ENG32.DLL's layout,
with the functions that differ rewritten from TV_ENG32.DLL. The tables and
resources inside this DLL are copied from the original TV_ENG32.DLL when it
is built.

Build @VERSION@.
