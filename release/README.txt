EVV Reader development firmware

Target hardware:
Freenove Media Kit for ESP32-S3, 1.14-inch model, 16 MB flash.

For a normal Windows installation, use EVV-Reader-Installer.exe and select one
complete firmware image. The browser installer presents the same choices by
speech engine and voice or build variant.

OpenEVV images (existing public engine):
  freenoveKit_Epub-en-US.bin - US English voice and interface
  freenoveKit_Epub-en-GB.bin - British English voice, English interface
  freenoveKit_Epub-fr-FR.bin - French voice, English interface

Pico images:
  freenoveKit_Epub-pico-en-US.bin - US English voice and interface
  freenoveKit_Epub-pico-en-GB.bin - British English voice, English interface
  freenoveKit_Epub-pico-de-DE.bin - German voice, English interface
  freenoveKit_Epub-pico-es-ES.bin - Spanish voice, English interface
  freenoveKit_Epub-pico-fr-FR.bin - French voice, English interface
  freenoveKit_Epub-pico-it-IT.bin - Italian voice, English interface

DECtalk image:
  freenoveKit_Epub-dectalk-dtc01-en-US.bin - DTC-01 US English voice and interface

OpenBST image:
  freenoveKit_Epub-openbst-1998ENG.bin - 1998 English voice and interface

All variants currently use the English interface. Book text is spoken by the
selected engine and voice or build.

Connect the kit through its exposed USB-UART socket, run the installer, and
select the chosen .bin file. Python and ESP-IDF are not required.

Every .bin file in this folder is a complete merged image and is flashed at
address 0x0. See the main repository README for SD-card setup, recovery
instructions, command-line flashing and source-build information.

The Pico, DECtalk and OpenBST variants are development candidates and must not
be published until their redistribution terms have been reviewed.
