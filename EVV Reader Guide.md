# EVV Reader Guide

This alpha firmware turns the Freenove Media Kit for ESP32-S3 into a speech-first ePub book reader. 

## What is the Freenove Media Kit?
The Freenove Media Kit is a small kit of components for people to play with and learn to code. it's basically an ESP32 room board (which has a processor and on-board memory, Wifi and Bluetooth radios, a Micro SD card slot and a USB port), connected to a "media shield" (providing a screen, headphone jack and speaker, and little keypad). These components are sandwiched together in a 3d printed case and are sold for hobbyests or students to learn coding or to make fun things. My fun thing was to be able to carry around something that sounds like eloquence in a device smaller than a pack of cards and read me ebooks.
## Physical description
Hold the unit with the screen at the top and the USB-C connector at the bottom. You'll find a headphone jack on the top right of the unit and feel the navigation stick slightly below and to the right-of-centre from the screen. You'll also feel a protruding antenna poking out at the top, directly in front of which sits the Micro SD card slot.

There is a power button at about 8 o'clock of the navigation stick. This button is not prominent, but it is a battery hardware cutout and will cause the unit to immediately lose power if pressed (unless you're powering it with a cable). This button *cannot* be captured as part of the keypad lock.

Similarly, on the back of the kit are 2 buttons, the right one of which is an immediate hard reset button. The standard case that Freenove ship exposes this button quite easily, so be cautious when operating the navigation stick if pressing the back of the kit down onto a hard surface. Eventually you'll be able to 3d print (or buy) a replacement back that fixes this.

the only control on the face of the unit is the navigation stick. It can be pressed "inward" (which we call centre), or in any of an up, right, down or leftward direction.



## The library

Powering the Kit with the firmware installed loads the library by default. Here, pressing:

* Up and down move through files and folders. 
* Left returns to the containing folder. 
* Centre opens the selected item. 
* Repeated presses of Right describe a file and offer file actions. 
* Hold Up to open settings. 
* Hold Left returns to the previously loaded reading.

The reader currently supports EPUB and plain-text books, MP3 audio, and M4B or M4A audiobooks. Recently opened items can appear at the top of the library if the setting is enabled in the menu.

## Reading EPUB and plain text

Pressing select on a file opens it for reading. From here:
* Centre pauses or resumes. 
* Hold Centre locks or unlocks the keypad. 
* Hold Left opens the library and Hold Up opens settings.

While paused, Up cycles the navigation unit. Left and Right move through the selected unit. EPUB navigation includes sections and headings. Plain-text navigation includes sentences, paragraphs, pages when form-feed page breaks exist, and a narrowing percentage search. During a narrowing search the reader announces the jump and starts reading around the new position.

Down moves through enabled status announcements. Hold Right begins a recording; the next button press stops and saves it.
While playing, up and down adjusts volume.

## Playing audio

* Up and down adjust volume.
* Centre pauses or resumes.
* Left and Right seek and immediately continue playback. 
* Up cycles through 30 seconds, one minute, five minutes, and a narrowing search; M4B files also offer chapter navigation. The narrowing search begins with half the file and halves the jump after every move.

Down moves through the same enabled status announcements used by book reading.

## Settings

* Up and Down move through the menu in either direction. 
* Left and Right change the focused setting. 
* Centre activates an action or toggles a setting. 
* Hold Up closes the menu and returns to the interface from which it was opened.

Playing-screen status items opens a submenu. You can move through these with up or down, toggle them on or off with Centre and Left or Right rearrange the announcement order. 

Network modes appear only when their corresponding configuration files are present on the SD card.

### settings
* Sleep timer: determines for how long the unit will speak or play a file. choose between 5 to 90 minutse.
* Speaking Rate and volume are used to adjust the speed and volume of the voice. These will eventually be added to with more voice choices and a variety of other options depending on which speech engine you have chosen to use.
* Clock: this lets you set the time from wifi (the kit reboots post clock setting) or by hand. The time is lost if the unit fully loses power, as it does whenever it restarts or the USB power is removed without a battery connected. Activating the file transfer mode also sets the clock. The current time is available as a status item whilst paused or on device unlock.
* sort files by: determines the order of files on the SD card.
*  Pause between sections: determine if the reader "keeps going" to the end of a book, or waits for you to press play at the end of a section (which is usually a fiction-book's chapter).
* On startup: Determines whether you are returned to your library or the current file is resumed when the kit is powered.
* Unlock Action: determines what happens when you unlock the unit from sleep by holding the centre key.
* Recent files: determines whether a list of recently opened files is maintained. If on, this is always placed at the top of the library, regardless of the sort order above.
* Book opening: Determines how files are read. in "Indexed" mode, a book is processed before reading starts, so you are immediately able to see how long it is, jump to a particular section and so forth. This takes more time, depending on the size of the book. In "instant" mode, the book is started reading immediately and the index is built when you pause playback, but you won't know how far through the book you are, be able to navigate passed a section more than 1 forward from the one you are reading at a time, nor look at the book's structure until the index has finished generating. If you leave an instant book open and paused, you'll hear a sound when the generation is complete.
* Playing screen status items. This is a toggleable list of what you hear when pressing down on the navigation stick whilst a file is paused.  You can choose to not hear an item by pressing select on it, and to hear an item earlier or later in sequence by moving it left or right.
\* Interface sounds: determines whether you hear beeps while the unit is doing things. There are noises to indicate a book is being processed, when you've moved between the library, reader and menu, and when recording has started and stopped.
* Keypress resets sleep timer: This determines whether your sleep timer is contiguous, which is the default, or whether pressing a button restarts it. In other words, do you want a 30 minute sleep timer regardless, or 30 minutes of uninterrupted listening. If this is turned on, the timer starts again whenever you press something.
* Start sleep timer on boot: This determines whether your last sleep timer is restarted whenever the device turns on.
### network modes: these appear near the bottom of the menu and can include File Transfer, NVDA Remote and Remsound. They only appear if they are configured for use.
* Clear book Cache: this deletes all the metadata about books the reader has generated. You can do this yourself from the SD card, with the caveat that it will mean longer opening times for books you had already processed.

## Network modes

### File transfer

File transfer uses the Wi-Fi details in ".evv/WIFI.INI" and starts a local WebDAV server. Its local name is EVV followed by four hexadecimal characters. Hold Centre deliberately to leave transfer mode and return to the library.

### NVDA Remote and RemSound

NVDA Remote listens for speech sent through the server configured in ".evv/nvdaremote.ini". RemSound uses ".evv/remsound.ini". These modes are hidden if their configuration files are absent.

### Configuration and privacy

Wi-Fi and remote-service details belong in the hidden ".evv" directory on the SD card. They are not built into public firmware. File transfer has no authentication and should only be used on a trusted private network.

## Alpha limitations

This is test software. Keep backup copies of books and recordings. A large EPUB can take a considerable time to index on first opening; its extracted reading data is then cached on the SD card. Please report the action being performed, the spoken message, and whether the unit recovered after reset when describing a fault.
