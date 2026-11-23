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

The battery status item, and the speak battery unlock action, give the battery percentage, rounded to the nearest five, and roughly how much reading time is left, such as "battery 65 percent, about 2 hours 10 minutes left". Near the end it says "battery almost empty". Both figures are worked out from the battery's voltage, so treat them as a guide rather than exact figures. After you unplug USB, give them about two minutes to settle. Right after the reader starts, the item is skipped for a few seconds until the first reading.

The board cannot tell directly whether USB power is connected, so the reader judges it from the voltage, which the charger lifts. While charging, the battery item gives the charge level and the time until the charger finishes, such as "battery charging, about 40 percent, about 50 minutes to full", and "battery charging, nearly full" at the end. For up to a minute after you plug in, and for the first few minutes after the reader starts, it may say "battery charge estimating" while it works out what the voltage means. If it cannot tell how far the charge has got, it says just "battery charging".

It says "battery on charger" when it can see the charger but not the battery: when the reader started on the charger with a nearly full battery, or when the battery has been switched off with the power button beside the five-way control. With USB connected, that button does not turn the reader off, so if you hear "battery on charger" for more than about an hour, check that the battery is switched on. With no battery connected, the item is skipped.

Unplugging USB restarts the reader. After a charge, the battery's voltage reads high for most of an hour, so for that time the level is not given as higher than the charge had reached when you unplugged.

The reader learns your battery. It starts with figures measured on a typical battery, about 3 hours 15 minutes of continuous reading from a full charge and about 65 minutes to charge from empty. Each time you read until the battery runs out and the reader switches itself off, it measures that run on the next start-up and adjusts its estimates, so they follow the battery actually fitted as it ages. Pausing during the run is fine. Discharge cycles interrupted by charging early do not recalibrate the device. In the same way, each charge that runs until the charger finishes adjusts the charging estimates.

## Book opening and cache

Indexed opening processes a book before reading starts, enabling full navigation and duration estimates immediately. Instant opening begins reading sooner and builds the complete index while paused. Large books take longer the first time. Extracted text, structure and metadata are cached under `.rr/.cache` on the SD card.

Clearing the book cache removes generated metadata but does not remove books. Previously processed books then take longer to open again.

## File transfer and network modes

See [Wireless file transfer](wireless-file-transfer.md) for browser, Windows, macOS and Linux instructions. NVDA Remote and RemSound use their respective files under `.rr` and remain hidden when not configured.

## Recordings

Recordings are stored beneath `.rr/recordings`, grouped by the current book when possible. Back them up: they are personal files and cannot be regenerated from the book cache.

## Alpha limitations

Keep backups of books and recordings. When reporting a fault, include the installed speech engine, the action being performed, the spoken message, whether playback was active or paused, and whether the unit recovered after reset.


