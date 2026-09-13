#!/usr/bin/env python3
"""
@file bootstrap_tools.py
@brief Automated Toolchain & Dependency Bootstrapper for Xenithra OS.
Downloads standalone portable compilers (Clang/LLVM, NASM) and OVMF firmware
directly into the project's 'tools/' directory without requiring admin privileges.
"""

import os
import sys
import shutil
import urllib.request
import zipfile

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_DIR = os.path.join(PROJECT_ROOT, "tools")
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")

TOOLS_CONFIG = {
    "ovmf": {
        "name": "OVMF UEFI BIOS",
        "url": "https://github.com/rust-osdev/ovmf-prebuilt/releases/latest/download/OVMF-pure-efi.fd",
        "dest_file": os.path.join(BUILD_DIR, "ovmf.fd"),
        "is_archive": False
    },
    "llvm-mingw": {
        "name": "LLVM / Clang / LLD Toolchain (x86_64)",
        "url": "https://github.com/mstorsjo/llvm-mingw/releases/download/20240619/llvm-mingw-20240619-ucrt-x86_64.zip",
        "dest_dir": os.path.join(TOOLS_DIR, "llvm-mingw"),
        "check_binary": os.path.join(TOOLS_DIR, "llvm-mingw", "bin", "clang.exe"),
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
        mb_down = downloaded // (1024 * 1024)
        mb_tot = total_size // (1024 * 1024)
        sys.stdout.write(f"\r    Downloading... {percent}% ({mb_down}MB / {mb_tot}MB)")
        sys.stdout.flush()

def ensure_tool(key, cfg):
    print(f"\n[*] Checking tool: {cfg['name']}...")
    os.makedirs(TOOLS_DIR, exist_ok=True)
    os.makedirs(BUILD_DIR, exist_ok=True)

    if not cfg["is_archive"]:
        dest = cfg["dest_file"]
        if os.path.exists(dest):
            print(f"    [+] Already present at {dest}")
            return True
        print(f"    [*] Fetching {cfg['url']} -> {dest}...")
        try:
            req = urllib.request.Request(cfg["url"], headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req) as resp, open(dest, 'wb') as out_f:
                total_size = int(resp.headers.get('Content-Length', 0))
                downloaded = 0
                block_size = 65536
                while True:
                    chunk = resp.read(block_size)
                    if not chunk:
                        break
                    out_f.write(chunk)
                    downloaded += len(chunk)
                    if total_size > 0:
                        pct = int((downloaded / total_size) * 100)
                        sys.stdout.write(f"\r    Downloading... {pct}% ({downloaded // 1024}KB / {total_size // 1024}KB)")
                        sys.stdout.flush()
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
            req = urllib.request.Request(cfg["url"], headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req) as resp, open(zip_path, 'wb') as out_f:
                total_size = int(resp.headers.get('Content-Length', 0))
                downloaded = 0
                block_size = 65536
                while True:
                    chunk = resp.read(block_size)
                    if not chunk:
                        break
                    out_f.write(chunk)
                    downloaded += len(chunk)
                    if total_size > 0:
                        pct = min(100, int((downloaded / total_size) * 100))
                        mb_down = downloaded // (1024 * 1024)
                        mb_tot = total_size // (1024 * 1024)
                        sys.stdout.write(f"\r    Downloading... {pct}% ({mb_down}MB / {mb_tot}MB)")
                        sys.stdout.flush()

            print(f"\n    [*] Extracting to {cfg['dest_dir']}...")
            with zipfile.ZipFile(zip_path, 'r') as z:
                z.extractall(TOOLS_DIR)
            if os.path.exists(zip_path):
                os.remove(zip_path)

            # Detect extracted folder name and rename to target dest_dir
            if not os.path.exists(cfg["dest_dir"]):
                for item in os.listdir(TOOLS_DIR):
                    item_path = os.path.join(TOOLS_DIR, item)
                    if os.path.isdir(item_path):
                        if key == "llvm-mingw" and "llvm-mingw" in item and item != "llvm-mingw":
                            os.rename(item_path, cfg["dest_dir"])
                            break
                        elif key == "nasm" and "nasm" in item.lower() and item != "nasm":
                            os.rename(item_path, cfg["dest_dir"])
                            break

            if os.path.exists(chk):
                print(f"    [+] Successfully installed {cfg['name']} in {cfg['dest_dir']}")
                return True
            else:
                print(f"    [!] Warning: Binary {chk} not found after extraction.")
                return False
        except Exception as e:
            print(f"\n    [!] Download/extraction failed for {cfg['name']}: {e}")
            if os.path.exists(zip_path):
                os.remove(zip_path)
            return False

def generate_toolchain_env():
    """Generates env.ps1 and env.cmd to automatically add portable tools to PATH."""
    env_ps1 = os.path.join(TOOLS_DIR, "env.ps1")
    llvm_bin = os.path.join(TOOLS_DIR, "llvm-mingw", "bin")
    nasm_bin = os.path.join(TOOLS_DIR, "nasm")

    ps1_content = f"""# Xenithra OS Environment Setup
$env:PATH = "{llvm_bin};{nasm_bin};" + $env:PATH
Write-Host "[+] Xenithra OS Portable Toolchain (Clang + NASM) added to PATH." -ForegroundColor Green
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

