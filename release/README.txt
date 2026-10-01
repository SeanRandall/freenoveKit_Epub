EVV Reader alpha releases

Target hardware:
Freenove Media Kit for ESP32-S3, 1.14-inch model, 16 MB flash.

For a normal Windows installation, download EVV-Reader-Installer.exe and one
of the following complete firmware images:

freenoveKit_Epub-en-US.bin - US English voice and English interface
freenoveKit_Epub-en-GB.bin - British English voice and English interface
freenoveKit_Epub-fr-FR.bin - French voice and English interface

The French interface has not yet been translated. Book text is spoken using
the French voice, but menus and fixed status messages remain in English.

Connect the kit through its exposed USB-UART socket, run the installer, and
select the chosen .bin file. Python and ESP-IDF are not required.

All three .bin files are complete merged images and are flashed at address
0x0. See the main repository README for SD-card setup, recovery instructions,
command-line flashing and source-build information.
