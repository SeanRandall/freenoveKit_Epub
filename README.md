# freenoveKit_Epub

Alpha firmware for turning the Freenove Media Kit for ESP32-S3, 1.14-inch
model, into a speech-first EPUB reader.

This repository contains the complete editable firmware source, the modified
OpenEVV source used by the firmware, an SD-card template, and a prebuilt alpha
image. The binary is provided for convenience; testers are free to inspect,
change and rebuild the code.

## Alpha warning

This is experimental firmware. It may crash, lose reading position or corrupt
files if power is removed while the SD card is being written. Keep another
copy of every book and recording. File transfer has no authentication and
must only be used on a trusted local network. NVDA Remote channel keys and
Wi-Fi passwords are secrets.

## Preparing the SD card

1. Format a micro-SD card as FAT32.
2. Copy the *contents* of `sd-card` to its root, including the hidden `.evv`
   directory.
3. In `.evv`, rename `WIFI.INI.example` to `WIFI.INI` and enter the local
   Wi-Fi name and password.
4. If NVDA Remote is required, rename `nvdaremote.ini.example` to
   `nvdaremote.ini` and enter the server, port and private channel key.
5. Add EPUB files anywhere in the card root. The alpha library currently
   presents EPUB files from the root.

No real network credentials or remote channel settings are included in this
repository or in the prebuilt image.

## Installing the prebuilt image

The target is the Freenove Media Kit for ESP32-S3 with the 1.14-inch media
board and 16 MB flash. Install Python and esptool, connect the exposed
USB-UART socket, identify its COM port, then run:

```powershell
python -m pip install esptool
python -m esptool --chip esp32s3 --port COM5 write-flash 0x0 release/freenoveKit_Epub-alpha.bin
```

Replace `COM5` with the board's port. Flashing replaces the existing firmware.

## Building and modifying the firmware

Install ESP-IDF 5.5.x and activate its environment. From the repository root:

```powershell
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

The reader entry point is `firmware/main/button_speech_main.c` through the
small `paragraph_reader_stable_main.c` wrapper. Network transfer and NVDA
Remote support are in `transfer_mode.c` and `webdav_transfer_main.c`. EPUB
extraction is in `epub_text.c`.

The release image also contains the network application in the OTA partition
at `0x610000`. Build that application separately with empty fallback
credentials; Wi-Fi and NVDA Remote settings are then read from the SD card:

```powershell
idf.py -B build-transfer -D EVV_APP_SOURCE=webdav_transfer_main.c `
  -D TRANSFER_WIFI_SSID= -D TRANSFER_WIFI_PASSWORD= build
```

Do not embed real credentials in a binary intended for distribution.

The OpenEVV tree under `vendor/openevv-perf` includes the ESP32-specific local
changes required by this build. It is deliberately vendored so testers can
modify the speech engine without relying on an unpublished binary.

The project firmware is MIT licensed. The vendored OpenEVV files retain their
own MIT licence and notices.

