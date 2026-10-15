# Using the reader

This is the Markdown source for Retroreader's current operating guide. Retroreader remains alpha software, so controls and announcements may change.

## Orientation and controls

Hold the unit with its screen at the top and exposed USB-C connector at the bottom. The headphone socket and microSD-card slot are along the top edge. The five-way navigation control presses Up, Right, Down, Left and inward for Centre.

The rear Reset button immediately restarts the unit. The separate power button cuts battery power and cannot be captured by the keypad lock.

## The library

- Up and Down move through files and folders.
- Left returns to the containing folder.
- Centre opens the selected item.
- Repeated Right presses report file information and expose file actions.
- Hold Up opens settings.
- Hold Left returns to the previously loaded reading.

Supported reading material currently includes EPUB, plain text, MP3, M4A and M4B. A configurable recent-files group can appear at the top of the library.

## Reading EPUB and plain text

- Centre pauses or resumes.
- Hold Centre locks or unlocks the keypad.
- Hold Left opens the library.
- Hold Up opens settings.
- Down cycles through enabled status information.
- While playing, Up and Down adjust volume.
- Hold Right starts recording; the next button press stops and saves it.

While paused, Up cycles through available navigation units. Left and Right move through the selected unit. EPUB navigation includes sections and headings. Plain-text navigation includes sentences, paragraphs, form-feed pages when present, and a narrowing percentage search.

## Playing audio

- Up and Down adjust volume.
- Centre pauses or resumes.
- Left and Right seek and immediately continue playback.
- Up cycles through 30 seconds, one minute, five minutes and narrowing search. M4B files can also expose chapters.
- Down cycles through enabled status information.

## Settings

- Up and Down move through the menu in either direction.
- Left and Right change the focused setting.
- Centre activates or toggles it.
- Hold Up closes the menu and returns to the interface from which it opened.

The menu includes the sleep timer, speech settings, clock, library sorting, section pauses, startup behaviour, unlock action, recent files, book-opening mode, status items, interface sounds and sleep-timer behaviour. Available speech controls depend on the installed synthesizer.

The status submenu lets you enable, disable and reorder individual announcements. Network modes appear only when their corresponding configuration files are present on the SD card.

## Book opening and cache

Indexed opening processes a book before reading starts, enabling full navigation and duration estimates immediately. Instant opening begins reading sooner and builds the complete index while paused. Large books take longer the first time. Extracted text, structure and metadata are cached under `.evv/.cache` on the SD card.

Clearing the book cache removes generated metadata but does not remove books. Previously processed books then take longer to open again.

## File transfer and network modes

See [Wireless file transfer](wireless-file-transfer.md) for browser, Windows, macOS and Linux instructions. NVDA Remote and RemSound use their respective files under `.evv` and remain hidden when not configured.

## Recordings

Recordings are stored beneath `.evv/recordings`, grouped by the current book when possible. Back them up: they are personal files and cannot be regenerated from the book cache.

## Alpha limitations

Keep backups of books and recordings. When reporting a fault, include the installed speech engine, the action being performed, the spoken message, whether playback was active or paused, and whether the unit recovered after reset.

