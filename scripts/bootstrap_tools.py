#!/usr/bin/env python3
"""
@file bootstrap_tools.py
@brief Automated Toolchain & Dependency Bootstrapper for Xenithra OS.
Solves winget/admin installation failures by downloading standalone portable
compilers and emulators directly into the project's 'tools/' directory.
"""

import os
import sys
import shutil
import urllib.request
import zipfile
import tarfile

TOOLS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools")

TOOLS_CONFIG = {
    "ovmf": {
        "name": "OVMF UEFI BIOS",
        "url": "https://github.com/rust-osdev/ovmf-prebuilt/releases/latest/download/OVMF-pure-efi.fd",
        "dest_file": os.path.join("build", "ovmf.fd"),
        "is_archive": False
    },
    "w64devkit": {
        "name": "W64devkit (GCC, Make, GDB, Binutils)",
        "url": "https://github.com/skeeto/w64devkit/releases/download/v1.20.0/w64devkit-1.20.0.zip",
        "dest_dir": os.path.join(TOOLS_DIR, "w64devkit"),
        "check_binary": os.path.join(TOOLS_DIR, "w64devkit", "bin", "gcc.exe"),
        "is_archive": True
    },
    "nasm": {
        "name": "NASM Assembler (x86_64)",
        "url": "https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/win64/nasm-2.16.01-win64.zip",
        "dest_dir": os.path.join(TOOLS_DIR, "nasm"),
        "check_binary": os.path.join(TOOLS_DIR, "nasm", "nasm.exe"),
        "is_archive": True
    }
}

def download_progress_hook(block_num, block_size, total_size):
    downloaded = block_num * block_size
    if total_size > 0:
        percent = min(100, int((downloaded / total_size) * 100))
        sys.stdout.write(f"\r    Downloading... {percent}% ({downloaded // (1024*1024)}MB / {total_size // (1024*1024)}MB)")
        sys.stdout.flush()

def ensure_tool(key, cfg):
    print(f"\n[*] Checking tool: {cfg['name']}...")
    os.makedirs(TOOLS_DIR, exist_ok=True)
    os.makedirs("build", exist_ok=True)

    if not cfg["is_archive"]:
        dest = cfg["dest_file"]
        if os.path.exists(dest):
            print(f"    [+] Already present at {dest}")
            return True
        print(f"    [*] Fetching {cfg['url']} -> {dest}...")
        try:
            urllib.request.urlretrieve(cfg["url"], dest, download_progress_hook)
            print(f"\n    [+] Successfully saved {dest}")
            return True
        except Exception as e:
            print(f"\n    [!] Failed to download {cfg['name']}: {e}")
            return False
    else:
        chk = cfg["check_binary"]
        if os.path.exists(chk):
            print(f"    [+] Already present at {chk}")
            return True

        zip_path = os.path.join(TOOLS_DIR, f"{key}_temp.zip")
        print(f"    [*] Downloading portable package from {cfg['url']}...")
        try:
            urllib.request.urlretrieve(cfg["url"], zip_path, download_progress_hook)
            print(f"\n    [*] Extracting to {cfg['dest_dir']}...")
            with zipfile.ZipFile(zip_path, 'r') as z:
                z.extractall(TOOLS_DIR)
            if os.path.exists(zip_path):
                os.remove(zip_path)

            # Fix nested folders if needed
            if key == "nasm":
                extracted_nasm = os.path.join(TOOLS_DIR, "nasm-2.16.01")
                if os.path.exists(extracted_nasm) and not os.path.exists(cfg["dest_dir"]):
                    os.rename(extracted_nasm, cfg["dest_dir"])

            print(f"    [+] Successfully installed {cfg['name']} in tools/{key}")
            return True
        except Exception as e:
            print(f"\n    [!] Download/extraction failed for {cfg['name']}: {e}")
            if os.path.exists(zip_path):
                os.remove(zip_path)
            return False

def generate_toolchain_env():
    """Generates env.ps1 and env.cmd to automatically add portable tools to PATH."""
    env_ps1 = os.path.join(TOOLS_DIR, "env.ps1")
    w64_bin = os.path.join(TOOLS_DIR, "w64devkit", "bin")
    nasm_bin = os.path.join(TOOLS_DIR, "nasm")

    ps1_content = f"""# Xenithra OS Environment Setup
$env:PATH = "{w64_bin};{nasm_bin};" + $env:PATH
Write-Host "[+] Xenithra OS Portable Toolchain added to PATH." -ForegroundColor Green
"""
    with open(env_ps1, "w") as f:
        f.write(ps1_content)

    print(f"\n[+] Toolchain environment helper generated: tools/env.ps1")

def main():
    print("==========================================================")
    print("   Xenithra OS - Automated Portable Toolchain Installer  ")
    print("==========================================================")
    for k, v in TOOLS_CONFIG.items():
        ensure_tool(k, v)
    generate_toolchain_env()
    print("\n[+] Bootstrap complete. Run 'powershell -File build.ps1' to build Xenithra OS.")

if __name__ == "__main__":
    main()
