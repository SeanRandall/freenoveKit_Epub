# OpenEVV UI tone assets

Place final WAV assets in this folder using the filenames below. Keep tones
short, distinctive and comfortable through both headphones and the speaker.
Mono PCM WAV is preferred; final sample rate and bit depth will be normalised to
the reader's audio pipeline during integration.

## Current tones

| Filename | Meaning |
| --- | --- |
| `keypress.wav` | A key was pressed while the controls were unlocked. |
| `recording-started.wav` | Microphone is open and recording has begun. |
| `recording-stopped.wav` | Recording stopped and the WAV was committed. |
| `menu_boundary.wav` | Start or end of a non-wrapping list was reached. |
| `volume-minimum.wav` | Listening volume reached its minimum. |
| `volume-maximum.wav` | Listening volume reached its maximum. |
| `locked.wav` | Controls were locked. |
| `unlocked.wav` | Controls were unlocked. |
| `warning.wav` | Attention is required before proceeding. |
| `error.wav` | An operation failed or required data is unavailable. |

High verbosity uses spoken confirmations. Low verbosity uses these tones for
routine confirmations, while names, values, warnings requiring a decision and
errors needing explanation remain spoken. If a tone is missing or UI sounds are
disabled, speech is the fallback.

There are deliberately no sounds for sleep-timer expiry, entering standby or
waking. Timer expiry is conveyed by the book stopping, and standby may occur
too long after the triggering interaction for a sound to be useful.

Generic action-confirmed and action-cancelled sounds are undecided and are not
currently required assets.
