# OpenEVV ESP32 reader UI

This document defines the speech-first interface for the ESP32 reader. The hardware is a [Freenove Media Kit](https://store.freenove.com/products/fnk0102), and the text-to-speech output provided by a port of [Openevv](https://github.com/mudb0y/openevv). Usable hardware controls consist of a 5-way d-pad only.
 

The interface has three primary places: the reading screen, the library and the
menu. There is no visual-only operation and no save or apply command. Focus is
spoken, changed values are spoken, and non-destructive changes are stored as
they are made.

## Input and speech conventions

The logical controls are Up, Down, Left, Right and Centre.

- A press performs the focused control's primary action.
- A hold is recognised after a configurable interval, initially 500 ms, and
  must not also perform the press action. The setting ranges from 250 to 1000 ms
  in 50 ms increments.
- Held directions repeat for list and text navigation.
- New navigation speech interrupts old navigation speech.
- Hold Centre locks or unlocks the controls from anywhere. While locked, other
  logical controls are ignored.
- Every key press while the controls are unlocked plays the keypress sound,
  provided UI sounds are enabled. Holds play it once on the initial press, not
  again when the hold threshold is reached.
- At list boundaries, a soft boundary sound is 
  used. Wrapping is optional and off by default.

High verbosity speaks control hints and verbal confirmations. Low verbosity
speaks necessary content and values, but replaces routine confirmations with
tones where a defined tone exists. A key-help action makes the next control
describe what it would do without performing it. If UI sounds are disabled or
a required tone is unavailable, the verbal confirmation is used instead.

## Reading screen

- Centre plays or pauses.
- Left/Right during playback seeks backward or forward by the chosen
  fine-grained navigation unit within the current EPUB section and continues.
  It never changes EPUB sections during playback. While paused it moves by the
  chosen navigation unit, speaks the new position and remains paused. Section
  navigation announces the destination heading without synthesizing its body;
  Centre subsequently prepares and plays the selected section.
- Up/Down while playing raises or lowers listening volume without interrupting
  the book.
- Up while paused opens the menu at the first status item.
- Down while paused speaks the next status announcement.
- Hold Left while paused opens the library at the current book.
- Hold Right while paused starts a book-note recording.
- Hold Centre locks or unlocks the controls.

Navigation units are character, word, sentence, line, paragraph and EPUB
section. Units which do not exist in a format are skipped. The default is
sentence. EPUB section is available only while paused; playback seeking never
crosses a section boundary. Navigation can optionally be remembered separately
for each book.

Pausing stores the exact location. An optional rewind-on-pause setting can move
back to the start of the current word, sentence, line or paragraph before the
next resume.

Finishing a book follows the selected completion action: stop at the end,
restart the book, or open the next book. Stop at the end is the default.

## Paused-screen status announcements

Down follows the Rockbox WPS-hotkey model: each press speaks one configured
group of information. Repeated presses within ten seconds advance through the
groups; after ten seconds without another press, the next Down press starts at
the first group again.

The available announcement fields are:

- Time; date; date and time.
- Sleep-timer time remaining.
- Estimated reading time elapsed, total and remaining. Combined forms can say
  "elapsed … remaining" or "elapsed … of …". referring to current epub section or entire file
- Current document number, total supported-document count and documents
  remaining in the current folder. A combined form says "document … of …".
- Battery percentage and estimated battery time remaining. Combined forms may
  include the spoken prefix "battery level".
- Current document title, reading percentage and selected navigation unit.

The default sequence contains only:

1. Current time.
2. Current battery percentage.
3. Estimated reading time remaining in the current document.

The sequence is an ordered checklist editable through **Interface and library >
Playing-screen status items**. Every field available to the paused reading
screen appears in the editor, including fields which are currently disabled or
temporarily unavailable. Up/Down moves focus through the complete list. Centre
toggles the focused field between live and not live. Left moves it one place
earlier in the sequence; Right moves it one place later. Focus follows the item
when it moves. A move at the first or last position plays the boundary sound.

On focus, the reader speaks the field name, whether it is live, and its
position. Toggling speaks the new state; moving speaks the new position. The
order of disabled fields is retained, so enabling one later inserts it at its
chosen position. Changes take effect and are stored immediately. Hold Up or
Hold Left returns to the Interface and library menu.

Down on the paused reading screen cycles through live fields in this stored
order. Disabled fields and unavailable values are skipped rather than guessed.
If no live field currently has an available value, the reader says "status
unavailable". A new announcement interrupts the previous status speech and the
reader remains paused throughout.

## Book-note recording

Holding Right while paused speaks "recording" at high verbosity or plays the
recording-started tone at low verbosity, then opens the microphone
and begins a PCM WAV file. Releasing Right after the initiating hold does not
stop the recording. The next new press of any logical control stops recording,
commits the file and its metadata, and speaks "recording saved" at high
verbosity or plays the recording-stopped tone at low verbosity. That stopping
press is consumed and does not perform its normal reader action. Audio capture
is stopped before the keypress sound is played, so the sound is not included in
the recording.

The filename uses a collision-safe timestamp or sequence number. Metadata stores
the source book's stable identifier, title, text location and reading percentage.
Recording writes to a temporary filename and atomically renames it only after a
valid WAV header and all audio data have been flushed. If recording cannot
start or storage becomes unavailable, the partial file is retained for recovery of any usable data.

## Bookmarks and sleep timer

The menu's bookmark action stores the current marked text position and speaks
its number. The bookmarks item lists bookmarks for the open book; Up/Down moves,
Centre jumps to one, and Hold Right opens remove-bookmark confirmation. A
separate clear-all action is available from book actions and always requires
confirmation. The current reading position is maintained on pause
automatically; pausing does not create a bookmark.

The sleep timer offers Off, 5, 10, 15, 30, 45, 60 and 90 minutes. Left/Right
chooses a value and Centre starts it. Starting with the previously used value is
one Centre press. When it expires, reading stops after the current speech unit
and the position is saved. Settings determine whether any control press restarts
the countdown and whether a saved timer starts again when the reader wakes. No
sound or announcement is played when the timer expires: the book stopping is
the feedback. Note that a speech unit is different to the navigation unit.

Pause between sections is a separate, off-by-default option. When enabled, the
reader finishes the current section, pauses before speaking the first content
of the next section, and saves that next-section position. Centre resumes from
there. It works whether or not a sleep timer is active and has no effect on
formats in which no section boundaries can be identified.

## Library

- The library is a live view of the SD card's actual directory tree, rooted at
  the card. It does not copy files into a separate library, impose a directory
  layout, or require a catalogue to be built before browsing. User-created
  folders and files retain their original names and locations.
- Up/Down moves through folders and files and speaks each name.
- Centre opens a folder. On a supported file it selects the appropriate reader
  or player from the file type, looks up that file's internal reader state, and
  resumes at the saved position when one exists. On an unsupported file it
  speaks that the type cannot currently be opened without hiding the file.
- Left moves to the parent folder; at the root it returns to reading.
- Right speaks details: type, size, modified date when available, reading
  percentage and bookmark count.
- Hold Right opens file actions.
- Hold Centre locks or unlocks the controls.

Actions are offered only when meaningful for the selected file. Document
actions include Resume/Open, Restart, Bookmarks, Clear reading position, Clear
bookmarks, Lock against deletion and Delete. Other supported types, including
plain text, HTML and audio, may provide their own applicable actions. Destructive
actions use a two-step confirmation: Centre confirms and any other control
cancels. A locked file cannot be deleted until it is unlocked.

Sorting choices are title, modified date and size. An optional Recent documents
virtual folder lists references to the most recently opened documents without
moving their files or creating a second library structure.

## Menu structure

Up/Down moves item by item, Hold Down jumps to the next group, Left/Right changes
a value, and Centre activates an action. Hold Up or Hold Left closes the menu
and returns to the place from which it was opened. On an informational item,
Centre also returns. Changes take effect and are stored immediately. A hold used
to close the menu must not alter the focused value or perform its press action.

### Status

1. Time and date, when available.
2. Power state and battery percentage, when available.
3. Free storage space, supported-file and folder count, and storage capacity.
4. Current document title and reading percentage.
5. Approximate reading time remaining, calculated from recent speech rate.
6. Active sleep-timer time remaining.

Unavailable information is spoken as unavailable and is not invented.

### Speech

The reader sets its speech profile directly on the active OpenEVV instance.
`vc_setVoiceParam(instance, 0, parameter, value)` changes the current voice and
the value remains in force across later `et_addText` and synthesis calls. A
preset voice is copied into the active voice, then any saved custom values are
applied. Settings therefore change the engine only when the user changes them
or when a saved profile is restored; normal book chunks need no command prefix.

1. Voice preset: copy voice 1 through 8 into active voice 0; embedded equivalent
   `` `vN``.
2. Speed: `V_SPEED`; embedded equivalent `` `vsN``.
3. Voice strength: `V_VOLUME`; embedded equivalent `` `vvN`` (an advanced
   voice-shaping setting, not the
   everyday listening-volume control).
4. Pitch baseline: `V_PITCH`; embedded equivalent `` `vbN``.
5. Roughness: `V_ROUGHNESS`; embedded equivalent `` `vrN``.
6. Breathiness: `V_BREATHINESS`; embedded equivalent `` `vfN``.
7. Head size: `V_HEAD_SIZE`; embedded equivalent `` `vhN``.
8. Gender: `V_GENDER`; embedded equivalent `` `vgN``.
9. Punctuation: none, some or all, implemented by the reader's text layer.
10. Listening volume, implemented after synthesis by the audio codec/amplifier
    when it offers a suitable gain control, otherwise by saturating PCM scaling
    before I2S output. This affects book speech, menus, tones and other audio
    consistently.

The engine also supports embedded voice, speed, volume, pitch, pause, index and
audio commands, but book annotations are disabled by default. In normal mode,
EPUB and text content is submitted as plain input and backtick sequences have
no control effect.

An `Allow embedded OpenEVV commands` setting enables annotated input for users
who deliberately prepare compatible books. It is off by default and may be
remembered globally or per book. On leaving an opted-in book, and before any UI
announcement, the reader restores the saved speech profile through the direct
voice API so an inline command cannot leak into menus or another book.

The settings menu exposes friendly values and translates them to direct voice
API values. Exact limits and defaults are taken from OpenEVV's verified
behaviour.
Listening volume is stored separately from the speech profile. In opted-in
books, an inline `` `vv`` command may change synthesized voice strength but must
never change the user's listening-volume setting.

### Reading

1. Navigation unit.
2. Remember navigation per book.
3. Default navigation unit.
4. Rewind on pause.
5. Set bookmark.
6. Browse bookmarks.
7. Sleep timer.
8. Restart timer on control press.
9. Start saved timer on wake.
10. Pause between sections: off by default.
11. End-of-book action.
12. Allow embedded OpenEVV commands: off by default.

### Interface and library

1. Verbosity: high or low.
2. Key help.
3. Wrapping.
4. File sorting.
5. Show Recent documents.
6. UI sound volume.
7. Playing-screen status items: opens the ordered-checklist editor described
   under Paused-screen status announcements.
8. Hold time: 250 to 1000 ms in 50 ms increments; default 500 ms.
9. Deep-sleep timeout: 5 to 30 minutes in 5-minute increments; default 15
   minutes.

### Optional capabilities

Capabilities appear only when implemented by this board and firmware build.
An example is wireless file transfer. Its absence must not leave a dead menu
item.

Wireless transfer is an exclusive mode. Entering it stops playback and speech
generation, closes all open files, flushes reader state and prevents the reader
from accessing the card until transfer mode ends. It then starts an
unauthenticated WebDAV service on the local Wi-Fi network, announces its local
hostname or IP address and provides an explicit Stop transfer action. The
service exposes the SD card's real directory tree rather than a special upload
folder. Authentication and remote Internet exposure are outside the initial
scope.

Uploads use temporary names and are atomically renamed when complete. Ending
transfer mode stops WebDAV, discards incomplete temporary uploads, refreshes the
current folder and restores normal card access. Standby is inhibited while a
transfer is active.

### System actions

1. Software version and build identifier.
2. Usage figures: reading time, books opened and words spoken.
3. Re-scan book storage.
4. Initialise book storage with supplied documentation, if bundled.
5. Reset reader data.
6. Format removable book storage, if present.
7. Standby, if supported.

Reset and format are destructive and require the action to be selected, a full
warning to be spoken, and a second Centre press. Any other control cancels.
Firmware update controls belong here only after a recoverable update path has
been implemented and tested.

## Persistent reader data

Reader data is metadata, not a duplicate library or file catalogue. It is held
in internal persistent storage and maps a stable file identity to last position,
bookmarks, per-file navigation, locks and recency. Global settings are stored
alongside it. A file identity should use the card identity and relative path,
with size, modification time and a small content fingerprint available to
reconcile safe renames or replacements. Files which have no stored state remain
immediately browsable, and stale state for missing files does not create ghost
library entries. The old manual's "Babel file" and its hardware-specific limits
are not requirements.

State is written atomically before book changes, destructive actions and
standby. Reading position is checkpointed after explicit navigation and at a
bounded interval during continuous reading so sudden power loss loses little
progress without causing excessive flash writes. On startup, the newest valid
checkpoint is restored and its book title and percentage are spoken.

## Lock and standby

Locking while reading prevents accidental controls but does not stop playback
or enter standby. When playback subsequently stops, the locked reader enters
light sleep after saving its position and switching off unused peripherals.

Locking while already paused saves the position and enters light sleep
immediately. A supported control wake returns from light sleep with the reader
still paused and locked; Hold Centre unlocks it. Standby and wake do not play
sounds.

If the reader remains paused and locked for the configured deep-sleep timeout,
it saves an atomic checkpoint and enters deep sleep. The timeout choices are 5,
10, 15, 20, 25 and 30 minutes. Centre is the designated deep-sleep wake control
because the five-way switch uses a single analogue input and not every direction
can reliably trigger digital deep-sleep wake. Deep-sleep wake performs a normal
firmware start, restores the last book and position, and remains paused and
locked until Hold Centre unlocks it.

## First-run and empty states

On first run, the reader speaks a short, skippable tutorial covering play/pause,
navigation, paused-screen status, recording, the library, the menu, volume and
key help. The full manual can be included as an ordinary book but is not assumed
to exist on any particular storage device.

An empty library says how to add a supported book using the capabilities in the
current build. Unsupported formats and protected content are reported clearly
without making claims inherited from the abandoned hardware design.
