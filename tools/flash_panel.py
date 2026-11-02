"""Flash an openHASP build onto a known panel, refusing if the connected board is a different panel.

Usage (with the PlatformIO python, it ships pyserial):
    ~/.platformio/penv/Scripts/python.exe tools/flash_panel.py guition   [--port COM4] [--erase]
    ~/.platformio/penv/Scripts/python.exe tools/flash_panel.py waveshare [--port COM9] [--erase]

The board MAC is read first and compared with the panel's known MAC, so a Guition build
can never land on the Waveshare (and vice versa) even if the COM ports get swapped.
"""

import argparse
import os
import re
import subprocess
import sys

PIO_HOME = os.path.join(os.path.expanduser("~"), ".platformio")
ESPTOOL = os.path.join(PIO_HOME, "packages", "tool-esptoolpy", "esptool.py")
BOOT_APP0 = os.path.join(PIO_HOME, "packages", "framework-arduinoespressif32", "tools", "partitions", "boot_app0.bin")
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PANELS = {
    "guition": {
        "name": "Guition ESP32-S3 4848S040",
        "env": "esp32-s3-4848s040_16MB",
        "mac": "28:84:85:85:48:b0",
        "port": "COM4",
        "baud": 921600,
    },
    "waveshare": {
        "name": "Waveshare ESP32-S3-Touch-LCD-4 V4",
        "env": "waveshare-s3-lcd4-v4_16MB",
        "mac": "e8:3d:c1:f7:79:48",
        "port": "COM9",
        "baud": 921600,
    },
}


def esptool(port, baud, *args, capture=False):
    cmd = [sys.executable, ESPTOOL, "--chip", "esp32s3", "--port", port, "--baud", str(baud)] + list(args)
    print(">", " ".join(cmd[1:]))
    return subprocess.run(cmd, capture_output=capture, text=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("panel", choices=PANELS.keys())
    parser.add_argument("--port", help="override COM port")
    parser.add_argument("--erase", action="store_true", help="erase the whole flash first (wipes config and pages)")
    args = parser.parse_args()

    panel = PANELS[args.panel]
    port = args.port or panel["port"]
    build = os.path.join(ROOT, ".pio", "build", panel["env"])
    images = [
        ("0x0", os.path.join(build, "bootloader.bin")),
        ("0x8000", os.path.join(build, "partitions.bin")),
        ("0xe000", BOOT_APP0),
        ("0x10000", os.path.join(build, "firmware.bin")),
    ]

    result = esptool(port, panel["baud"], "--after", "no_reset", "read_mac", capture=True)
    print(result.stdout)
    macs = re.findall(r"mac:\s*([0-9a-f:]{17})", result.stdout.lower())
    if result.returncode != 0 or not macs:
        sys.exit(f"Could not read MAC on {port}")
    if macs[0] != panel["mac"]:
        known = {p["mac"]: p["name"] for p in PANELS.values()}
        sys.exit(
            f"REFUSING: {port} is {known.get(macs[0], 'an unknown board')} ({macs[0]}), "
            f"not {panel['name']} ({panel['mac']})"
        )
    for _, path in images:
        if not os.path.isfile(path):
            sys.exit(f"Missing {path}\nBuild first: pio run -e {panel['env']}")
    print(f"OK: {port} is {panel['name']}, flashing env {panel['env']}")

    if args.erase and esptool(port, panel["baud"], "--after", "no_reset", "erase_flash").returncode != 0:
        sys.exit("erase_flash failed")

    flash_args = ["--before", "default_reset", "--after", "hard_reset", "write_flash", "--flash_mode", "dio",
                  "--flash_size", "16MB", "--flash_freq", "80m"]
    for offset, path in images:
        flash_args += [offset, path]
    sys.exit(esptool(port, panel["baud"], *flash_args).returncode)


if __name__ == "__main__":
    main()
