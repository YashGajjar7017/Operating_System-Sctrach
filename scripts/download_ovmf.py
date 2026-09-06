#!/usr/bin/env python3
"""
@file download_ovmf.py
@brief Helper script to obtain OVMF UEFI BIOS firmware for QEMU testing.
"""

import os
import urllib.request

OVMF_URL = "https://github.com/rust-osdev/ovmf-prebuilt/releases/latest/download/OVMF-pure-efi.fd"
OVMF_LOCAL = "build/ovmf.fd"

def fetch_ovmf():
    os.makedirs("build", exist_ok=True)
    if os.path.exists(OVMF_LOCAL):
        print(f"[+] OVMF firmware found at: {OVMF_LOCAL}")
        return True

    print(f"[*] Downloading prebuilt OVMF UEFI firmware from {OVMF_URL}...")
    try:
        urllib.request.urlretrieve(OVMF_URL, OVMF_LOCAL)
        print(f"[+] Successfully saved OVMF firmware to {OVMF_LOCAL}")
        return True
    except Exception as e:
        print(f"[!] Warning: Could not download OVMF automatically ({e}).")
        print("[!] If running QEMU on Linux, pass '-bios /usr/share/ovmf/OVMF.fd' or equivalent.")
        return False

if __name__ == "__main__":
    fetch_ovmf()
