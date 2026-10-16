# Preparing the SD card



## What you need

- A microSD card formatted as FAT32. The 1GB card shipped by Freenove is suitable.
- A computer and card reader.
- The [Retroreader SD-card template](sd-card-template.zip).
- EPUB, plain-text, MP3, M4A or M4B files you want to use.

## Install the template

1. Download and extract the SD-card template.
2. Copy the **contents** of the extracted "sd-card" folder to the root of the card. Do not copy the containing folder itself.
3. Keep the ".rr" directory. It stores configuration, book indexes, reading state and recordings.
4. Copy books and audio into any folder structure you prefer. Retroreader browses the card directly and does not require a special library folder.
5. Eject the card safely before removing it from the computer.

## Configure Wi-Fi

Inside ".rr", rename "WIFI.INI.example" to "WIFI.INI". Edit it as plain text:

```ini
ssid=YOUR_WIFI_NETWORK
password=YOUR_WIFI_PASSWORD
```

Retroreader supports 2.4 GHz Wi-Fi. Keep this file private: it contains the network password in readable text.

The File transfer menu item appears only when both Wi-Fi values are available. Other network modes similarly appear only when their respective configuration files are present.

## Optional files

- "nvdaremote.ini" configures NVDA Remote listening mode.
- "remsound.ini" configures RemSound listening mode.
- "substitutions.tsv" contains tab-separated pronunciation substitutions. Put the text to find in the first column and its replacement in the second.

The template contains commented examples. Rename an example only when you intend to configure that service.

## Updating or replacing a card

Keep a backup of the entire card, especially ".rr/recordings" and any books that are not stored elsewhere. Cached book data can be regenerated, but recordings and personal files cannot.

## Troubleshooting checklist

- Confirm the card is FAT32 rather than exFAT or NTFS.
- Confirm ".rr" is at the card root.
- On Windows, enable **Hidden items** if ".rr" is not visible.
- Check that configuration files end in ".ini", not ".ini.txt".
- Use plain UTF-8 text and do not add quotation marks around Wi-Fi values.


