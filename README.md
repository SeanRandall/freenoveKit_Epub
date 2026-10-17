# RetroReader for the Freenove Media Kit

Experimental speech-first EPUB and text reader firmware for the **Freenove
Media Kit for ESP32-S3, 1.14-inch model**.

This repository is for both alpha testers and developers. It contains:

- a Windows installer;
- complete firmware images for the speech engines and variants in the dated
  firmware manifest;
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
   ".rr" directory.
3. Add EPUB books to the root of the card.
4. Insert the card before starting the reader.

"RetroReader Guide.epub" is included as a spoken introduction to the controls.

Network features are optional. To configure them:

1. Rename ".rr/WIFI.INI.example" to ".rr/WIFI.INI" and replace the example
   Wi-Fi name and password.
2. For NVDA Remote, rename ".rr/nvdaremote.ini.example" to
   ".rr/nvdaremote.ini" and replace the server, port and private channel key.

No real Wi-Fi credentials or NVDA Remote settings are included in this
repository or its public firmware images.



## Install the firmware

Use the [browser-based firmware installer](https://retroreader.uk/install.html)
in a current desktop Chrome or Edge browser on Windows, macOS, Linux or
ChromeOS. It groups firmware choices by speech engine and voice or build
variant, and includes the reader guide and a physical description of the
Freenove kit.

Windows users can instead download the standalone installer and firmware from
the [newest GitHub release](https://github.com/SeanRandall/freenoveKit_Epub/releases).

Download these two files from the "release" directory:

1. "RetroReader-Installer.exe"
2. One complete firmware image for the desired speech engine and voice or
   build variant. The filenames and available variants are documented in
   `release/README.txt`.

Connect the kit via USB, then run
"RetroReader-Installer.exe". Choose the downloaded ".bin" in the standard
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
python -m esptool --chip esp32s3 --port COM5 write-flash 0x0 release/RetroReader-openevv-en-GB.bin
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
Remote support are in "transfer_mode.c" and "network_services_main.c". EPUB
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
idf.py -B build-transfer -D RR_APP_SOURCE=network_services_main.c "
  -D RR_SPEECH_ENGINE=openevv -D EVV_LANGUAGE=enus "
  -D TRANSFER_WIFI_SSID= -D TRANSFER_WIFI_PASSWORD= build
"""


The OpenEVV tree under "vendor/openevv-perf" includes the ESP32-specific
changes required by this firmware. Pico, OpenBST, DECtalk DTC-01 Reborn,
DECtalk 4.3 and First Byte ProVoice Reborn are pinned as upstream Git submodules for the
alternative-engine builds. The First Byte engine is exposed as Monologue and
uses the compact XEN11K8 English voice package. The experimental DoubleTalk
Hybrid build combines the original DoubleTalk PC firmware front end with a
native ESP32 acoustic renderer. Dolphin Apollo II uses rommix0's native C
reconstruction of the British Apollo II firmware and its newly written 10 kHz
formant renderer. Speech Plus Prose 2000 uses the portable reconstruction of
the 1988 v3.4.1 hardware firmware and its formant DSP.

The project firmware is MIT licensed. Vendored speech engines retain their own
licences and notices.

