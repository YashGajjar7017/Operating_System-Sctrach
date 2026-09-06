# AuraOS: 64-bit Graphical Operating System

A custom, modern x86_64 Operating System built from scratch with a sleek Windows-style bootloader experience, GOP framebuffer compositing, and higher-half microkernel core.

---

## Architecture & Design Overview

```
+-------------------------------------------------------------------------+
|                          UEFI Firmware / OVMF                           |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                  AuraOS Bootloader (BOOTX64.EFI)                        |
|  - Freestanding PE32+ application (no external gnu-efi dependencies)    |
|  - Double-buffered GOP Graphics & Fluent Windows-Style Boot Menu        |
|  - ELF64 Kernel Loader & Memory Map Extraction                          |
|  - ACPI 1.0 / 2.0 RSDP Table Locator                                    |
|  - ExitBootServices() & System V AMD64 Handoff                          |
+-------------------------------------------------------------------------+
                                    |
                            (RDI = BootInfo*)
                                    v
+-------------------------------------------------------------------------+
|                     AuraOS 64-bit Kernel Core                           |
|  - Higher-half address space: 0xFFFFFFFF80000000 (Load Base: 0x200000)  |
|  - Linear Framebuffer Rendering Engine (32-bit ARGB/XRGB)               |
|  - Phase 3: GDT, IDT, 4-Level Paging, Buddy/Slab Allocator, Scheduler   |
|  - Phase 4: Windows-Style Animated Spinner & 2D Vector Compositor       |
|  - Phase 5: Window Server, Taskbar, Start Menu & Input Drivers          |
+-------------------------------------------------------------------------+
```

---

## Directory Structure

```
c:\Data\Coding\OS_Kernal
├── bootloader/             # UEFI Freestanding Bootloader (PE32+)
│   ├── efi.h               # Complete UEFI 2.8 freestanding specification header
│   ├── gop.h               # GOP Framebuffer interface & color constants
│   ├── gop.c               # Double-buffered software rendering engine
│   ├── font.h              # Font typography definitions
│   ├── ui.h                # Boot menu UI header & data models
│   ├── ui.c                # Sleek Windows-style graphical boot selector UI
│   └── main.c              # EfiMain, ELF loader, memory map, & kernel handoff
├── kernel/                 # 64-bit OS Kernel
│   ├── arch/x86_64/
│   │   └── entry.asm       # 64-bit kernel entry point (System V AMD64 ABI)
│   ├── linker.ld           # Higher-half 64-bit ELF kernel linker script
│   └── main.c              # Kernel main & hardware diagnostics
├── shared/                 # Shared ABI headers between bootloader & kernel
│   ├── bootinfo.h          # AuraBootInfo, MemoryMap, & Framebuffer structs
│   ├── font.h              # Shared 8x16 font header
│   └── font.c              # Standard 8x16 ASCII glyph table
├── scripts/                # Python Build & Simulation Utilities
│   ├── build_disk.py       # Pure-Python FAT32 UEFI ESP disk image generator
│   └── download_ovmf.py    # OVMF UEFI firmware fetcher
├── build.ps1               # PowerShell automated build & test script
├── Makefile                # Multi-platform GNU Makefile
└── README.md               # Project documentation & reference
```

---

## Toolchain Requirements

To compile and test AuraOS:

| Tool | Recommended Package | Purpose |
|------|--------------------|---------|
| **Clang / LLVM** | `LLVM` (`clang`, `ld.lld`) | Freestanding C compiler for EFI PE32+ and Kernel ELF |
| **NASM** | `NASM` | x86_64 Assembly compiler |
| **Python 3** | `Python 3.10+` | Cross-platform FAT32 disk image packager & firmware scripts |
| **QEMU** | `QEMU` (`qemu-system-x86_64`) | Bare-metal x86_64 hardware emulator |

### Installation Commands:
- **Windows (Winget):**
  ```powershell
  winget install LLVM.LLVM
  winget install NASM.NASM
  winget install SoftwareFreedomConservancy.QEMU
  ```
- **Linux (Debian/Ubuntu):**
  ```bash
  sudo apt install clang lld nasm qemu-system-x86 ovmf python3
  ```

---

## Building and Running

### On Windows (PowerShell):
```powershell
# Build Bootloader, Kernel, FAT32 ESP Disk Image, and shareable ISO (build/auraos.iso)
.\build.ps1

# Test the UEFI Disk image in QEMU:
.\build.ps1 -Run

# Test the Bootable ISO image in QEMU:
.\build.ps1 -Run -Iso
```

### On Linux / macOS (Makefile):
```bash
# Build both disk.img and auraos.iso
make

# Build only the ISO image
make iso

# Launch QEMU with the UEFI disk image
make run

# Launch QEMU directly with the bootable ISO CD-ROM
make run-iso
```

---

## Sharing and Virtual Machine Deployment

The generated file `build/auraos.iso` is a standard **El Torito UEFI Bootable ISO 9660** image.

You can directly:
1. **Share `build/auraos.iso`** with anyone.
2. **VirtualBox**: Create a new VM (`Type: Other`, `Version: Other/Unknown (64-bit)`), check **Enable EFI (special OSes only)** in *Settings -> System -> Motherboard*, and attach `auraos.iso` to the Optical Drive.
3. **VMware Workstation**: Create a new VM (`Guest OS: Other 64-bit`), switch Firmware type to **UEFI** in *VM Settings -> Options -> Advanced*, and mount `auraos.iso` as the CD/DVD drive.
4. **Flash to USB**: Use Rufus, Etcher, or `dd` to write the ISO to a physical USB thumb drive for bare-metal UEFI booting.
