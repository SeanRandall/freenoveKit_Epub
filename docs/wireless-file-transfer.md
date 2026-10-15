# Wireless file transfer

This page explains how to access Retroreader's microSD card over local Wi-Fi.

## Before connecting

Configure ".evv/WIFI.INI" as described in [Preparing the SD card](preparing-the-sd-card.md). Open **File transfer** from Retroreader's menu (hold Up, then press Up again). Reading and other card access stop while transfer mode is active, preventing two services from changing the card simultaneously.

Retroreader announces an address such as "EVVXXXX" and its IP address. Your identifier will differ. In the examples below, replace "evvXXXX" with the identifier announced by your unit. You can also see your unit's identifier during the installation process.

File transfer currently has no password and should be used only on a trusted private network. Wi-Fi is 2.4 GHz, although the computer may use another band if both bands belong to the same local network.

## Browser interface

Open the following address in a web browser:

```text
http://evvXXXX.local/
```

The browser interface can browse folders, upload and download files, create folders, rename or move items, and delete files or folders. 

If the ".local" name does not resolve, use the announced numeric address, for example "http://192.168.1.77/".

## Windows File Explorer

1. Open **File Explorer** and select **This PC**.
2. Choose **See more**, then **Map network drive**. Older versions of Windows show **Map network drive** directly on the ribbon.
3. Choose an unused drive letter.
4. Enter "http://evvXXXX.local/" in the Folder box.
5. Select **Finish**.

You can also use Command Prompt:

```bat
net use * http://evvXXXX.local/
```

Windows uses its WebClient service for WebDAV. If Explorer says the network name cannot be found, start the **WebClient** service from the Services application and try again. Explorer can be slow when calculating progress or opening folders; the browser interface may be quicker for occasional transfers.

## macOS Finder

1. In Finder, choose **Go**, then **Connect to Server**, or press **Command-K** (⌘K).
2. Enter "http://evvXXXX.local/".
3. Choose **Connect**. If asked, connect as a guest.
4. Retroreader appears as a mounted network location in Finder.

Eject the mounted location in Finder before leaving transfer mode.

## Linux desktop file managers

Most desktop file managers use GVfs or KIO and can open WebDAV directly.

### GNOME Files, Nautilus, Nemo and similar file managers

Open **Other Locations**, find **Connect to Server**, and enter:

```text
dav://evvXXXX.local/
```

The command-line equivalent on a GVfs desktop is:

```sh
gio mount dav://evvXXXX.local/
```

### KDE Plasma and Dolphin

Enter this in Dolphin's location bar:

```text
webdav://evvXXXX.local/
```

### Mounting with davfs2

Install "davfs2" using the distribution's package manager:

```sh
# Debian, Ubuntu, Linux Mint and Raspberry Pi OS
sudo apt install davfs2

# Fedora
sudo dnf install davfs2

# Arch Linux and Manjaro
sudo pacman -S davfs2
```

Create a mount point and mount the card:

```sh
sudo mkdir -p /mnt/retroreader
sudo mount -t davfs http://evvXXXX.local/ /mnt/retroreader
```

If prompted for credentials, leave them blank. Unmount before leaving transfer mode:

```sh
sudo umount /mnt/retroreader
```

## Ending transfer mode

Finish or cancel active copies, then unmount or disconnect the WebDAV location. Hold Centre on Retroreader to exit transfer mode and return to the library. Do not reset or remove power while a file is being written.

## Troubleshooting

- Press any key in transfer mode to repeat the hostname and IP address.
- Try the numeric IP address if ".local" name resolution is unavailable or filtered by the router.
- Confirm the computer and Retroreader are on the same local network and client isolation is disabled.
- A Pi-hole normally does not prevent mDNS ".local" discovery because mDNS is local multicast rather than ordinary DNS; use the numeric address if discovery remains unreliable.
- If a transfer stalls, wait before retrying. Avoid sending the same file from two clients simultaneously.
- Hold Centre to leave transfer mode only after transfers have stopped.

