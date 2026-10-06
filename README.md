# EVV Reader for the Freenove Media Kit

Experimental Eloquence-sounding EPUB reader firmware for the **Freenove Media Kit
for ESP32-S3, 1.14-inch model**.

This repository is for both alpha testers and developers. It contains:

- a Windows installer;
- complete US English, British English and French firmware images;
- an SD-card template and spoken EPUB guide;
- the complete editable firmware and modified OpenEVV source.

## Required hardware

This firmware targets the **1.14-inch Freenove Media Kit for ESP32-S3**, model
"FNK0102A". It is not intended for Freenove's larger-screen media kits.

- [Buy directly from Freenove](https://store.freenove.com/products/fnk0102)
- Amazon: [United Kingdom](https://www.amazon.co.uk/s?k=Freenove+FNK0102A),
  [United States](https://www.amazon.com/s?k=Freenove+FNK0102A),
  [Germany](https://www.amazon.de/s?k=Freenove+FNK0102A),
  [Canada](https://www.amazon.ca/s?k=Freenove+FNK0102A),
  [France](https://www.amazon.fr/s?k=Freenove+FNK0102A),
  [Italy](https://www.amazon.it/s?k=Freenove+FNK0102A),
  [Spain](https://www.amazon.es/s?k=Freenove+FNK0102A),
  [Australia](https://www.amazon.com.au/s?k=Freenove+FNK0102A),
  [Japan](https://www.amazon.co.jp/s?k=Freenove+FNK0102A), and
  [Mexico](https://www.amazon.com.mx/s?k=Freenove+FNK0102A).

Amazon availability and delivery regions vary. Check that the selected kit is
the 1.14-inch "FNK0102A" model before ordering. The larger screen model will work with modifications, but not using the installer for users at the moment.

## Important alpha warning

This is unfinished test firmware. It will crash, lose a reading position or
damage a file while the SD card is being read or written. Keep a
separate copy of every book. 

File transfer currently has no authentication. Use it only on a trusted local
network. 

## Prepare the SD card

1. Format a micro-SD card as FAT32.
2. Copy the *contents* of "sd-card" to the root of the card. Include the hidden
   ".evv" directory.
3. Add EPUB books to the root of the card.
4. Insert the card before starting the reader.

"EVV Reader Guide.epub" is included as a spoken introduction to the controls.

Network features are optional. To configure them:

1. Rename ".evv/WIFI.INI.example" to ".evv/WIFI.INI" and replace the example
   Wi-Fi name and password.
2. For NVDA Remote, rename ".evv/nvdaremote.ini.example" to
   ".evv/nvdaremote.ini" and replace the server, port and private channel key.

No real Wi-Fi credentials or NVDA Remote settings are included in this
repository or its public firmware images.



## Install the firmware

Use the [browser-based firmware installer](https://seanrandall.github.io/freenoveKit_Epub/)
in a current desktop Chrome or Edge browser on Windows, macOS, Linux or
ChromeOS. It includes all three voice choices, the reader guide and a physical
description of the Freenove kit.

Windows users can instead download the standalone installer and firmware from
the [latest GitHub release](https://github.com/SeanRandall/freenoveKit_Epub/releases/latest).

Download these two files from the "release" directory:

1. "EVV-Reader-Installer.exe"
2. One firmware image:
   - "freenoveKit_Epub-en-US.bin" — US English voice and English interface
   - "freenoveKit_Epub-en-GB.bin" — British English voice and English interface
   - "freenoveKit_Epub-fr-FR.bin" — French voice with an **English interface**

The French firmware speaks book text in French, but its menus and fixed status
messages have not yet been translated.

Connect the kit via USB, then run
"EVV-Reader-Installer.exe". Choose the downloaded ".bin" in the standard
Windows file-open dialog and confirm the detected COM port. Leave the device
connected until the installer reports that flashing has finished.

If several USB serial devices are
connected, disconnect the unrelated devices before starting. If installation
cannot connect, hold **BOOT**, briefly press **RESET**, release **BOOT**, and
try again.

Flashing replaces the firmware in internal flash. It does not erase content or
configuration stored on the SD card.

## Command-line installation

Developers can flash any complete release image with Python and esptool. Change
"COM5" and the image name as required:

"""powershell
python -m pip install esptool
python -m esptool --chip esp32s3 --port COM5 write-flash 0x0 release/freenoveKit_Epub-en-GB.bin
"""

## Build and modify the firmware

Install ESP-IDF 5.5.x and activate its environment. A normal US English build
from the repository root is:

"""powershell
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
"""

The reader entry point is "firmware/main/button_speech_main.c", reached through
the small "paragraph_reader_stable_main.c" wrapper. Network transfer and NVDA
Remote support are in "transfer_mode.c" and "webdav_transfer_main.c". EPUB
extraction is in "epub_text.c".

"EVV_LANGUAGE" selects the compiled OpenEVV language without forking the
reader. Current release values are "enus", "engb" and "frfr". For example:

"""powershell
idf.py -B build-engb -D EVV_LANGUAGE=engb build
"""

Generated compiled-rule files for all release languages are committed, so an
ordinary build does not require OpenEVV's Python conversion tools. Reader,
parser, playback and UI fixes are shared by every language build.

The complete release image also contains the network application in the OTA
partition at "0x610000". To build that application separately, always use
empty fallback credentials so settings are read from the SD card:

"""powershell
idf.py -B build-transfer -D EVV_APP_SOURCE=webdav_transfer_main.c "
  -D TRANSFER_WIFI_SSID= -D TRANSFER_WIFI_PASSWORD= build
"""


The OpenEVV tree under "vendor/openevv-perf" includes the ESP32-specific
changes required by this firmware. It is separated deliberately; eventually we'll add firmware with different synthesizers, as we do with different languages. 

The project firmware is MIT licensed. Vendored OpenEVV files retain their own
MIT licence and notices.
