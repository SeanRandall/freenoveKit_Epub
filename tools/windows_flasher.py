"""Small Windows firmware-image flasher for the Freenove EVV reader."""

from __future__ import annotations

import ctypes
import os
import tkinter as tk
from tkinter import filedialog

import esptool
import serial
from serial.tools import list_ports


LIKELY_USB_VIDS = {0x303A, 0x10C4, 0x1A86, 0x0403}
MB_TOPMOST_FOREGROUND = 0x10000 | 0x40000


def message(title: str, text: str, error: bool = False) -> None:
    icon = 0x10 if error else 0x40
    ctypes.windll.user32.MessageBoxW(None, text, title, icon | MB_TOPMOST_FOREGROUND)


def choose_image() -> str:
    root = tk.Tk()
    root.withdraw()
    root.attributes("-topmost", True)
    image = filedialog.askopenfilename(
        parent=root,
        title="Choose EVV Reader firmware image",
        filetypes=(("EVV firmware images", "*.bin"), ("All files", "*.*")),
    )
    root.destroy()
    return image


def detect_port() -> tuple[str | None, str]:
    ports = list(list_ports.comports())
    likely = [port for port in ports if port.vid in LIKELY_USB_VIDS]
    if len(likely) == 1:
        return likely[0].device, ""
    if not likely and len(ports) == 1:
        return ports[0].device, ""
    if not ports:
        return None, "No serial device was found. Connect the reader's exposed USB-UART port and try again."
    names = ", ".join(port.device for port in (likely or ports))
    return None, (
        "The reader could not be identified unambiguously. Disconnect other USB serial devices "
        f"and try again. Candidate ports: {names}"
    )


def port_is_available(port: str) -> tuple[bool, str]:
    try:
        with serial.Serial(port, 115200, timeout=0.25, write_timeout=0.25):
            pass
        return True, ""
    except serial.SerialException as exc:
        return False, (
            f"The reader at {port} is busy. Close any serial monitor or other installer and try again.\n\n{exc}"
        )


def main() -> int:
    image = choose_image()
    if not image:
        return 0
    port, problem = detect_port()
    if not port:
        message("EVV Reader Installer", problem, True)
        return 1
    available, problem = port_is_available(port)
    if not available:
        message("EVV Reader Installer", problem, True)
        return 1

    filename = os.path.basename(image)
    answer = ctypes.windll.user32.MessageBoxW(
        None,
        f"Install {filename} on the reader at {port}?\n\n"
        "Books and configuration on the SD card will not be erased.",
        "Install EVV Reader firmware",
        0x21 | MB_TOPMOST_FOREGROUND,
    )
    if answer != 1:
        return 0

    print(f"Installing {filename} on {port}. Do not disconnect the reader.", flush=True)
    common = [
        "--chip", "esp32s3", "--port", port, "--baud", "460800",
        "--connect-attempts", "3", "--before", "default-reset", "--after", "hard-reset",
    ]
    operation = [
        "write-flash", "--flash-mode", "dio", "--flash-freq", "80m",
        "--flash-size", "16MB", "0x0", image,
    ]
    try:
        try:
            esptool.main(common + operation)
        except BaseException as exc:
            if "stub data is missing" not in str(exc).lower():
                raise
            print("Packaged flasher stub unavailable; retrying in ROM mode.", flush=True)
            esptool.main(common + ["--no-stub"] + operation)
    except BaseException as exc:
        message(
            "Installation failed",
            "The firmware could not be installed. If necessary, hold BOOT while briefly pressing "
            f"RESET, then try again.\n\n{exc}",
            True,
        )
        return 1

    message("Installation complete", "Firmware installed successfully. The reader is restarting.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
