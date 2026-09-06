# Xenithra OS: High-Security 64-bit Graphical Operating System

A custom, modern x86_64 Operating System built from scratch with a sleek Windows-style boot experience, hardware-enforced **Session Hijacking Prevention**, an integrated **Private Firewall**, and a **32-bit Graphical Window Compositor & Desktop Environment**.

---

## 🛡️ Security Architecture & Anti-Hijack Guard

Xenithra OS is engineered from the ground up for resilience against memory exploitation, privilege escalation, and session interception:

1. **Anti-Session-Hijacking Subsystem (`kernel/security/session.c`)**:
   - **128-bit Cryptographically Random Session Tokens**: Generated using hardware CPU entropy sources (`RDRAND`/`RDSEED` with jitter state mixing fallback).
   - **Strict Process ID & Capability Binding**: Tokens are bound to `(SessionID, PID, UID, RingLevel, CapabilityBitmask, IP)`. If a rogue or unauthorized process attempts to borrow or forge a token, access is instantly blocked, the event is logged, and the anomalous session is revoked.
2. **Hardware SMEP & SMAP Enforcement**:
   - Supervisor Mode Execution Prevention (`CR4.bit20`) and Supervisor Mode Access Prevention (`CR4.bit21`) prevent kernel-mode execution of userland shellcode or arbitrary memory hijacking.
3. **Integrated Private Firewall (`kernel/security/firewall.c`)**:
   - Stateful packet filtering engine (TCP, UDP, ICMP).
   - Stealth mode dropping unsolicited probe requests and mitigating port-scanners.
   - Built-in live rules table management and security stats.

---

## 🖥️ Graphical Desktop & Declarative DOM UI Engine

- **32-bit Window Compositor (`kernel/gui/compositor.c`)**:
  - Double-buffered software rendering engine with Mica/Acrylic dark aesthetics.
  - Full Window Management: Titlebars, Close `[X]`, Maximize/Restore `[+]`, Minimize `[-]`, window dragging, and z-order focus stacking.
  - Windows 11-style centered Taskbar with Start Menu, active app pills, security status badge, and clock.
  - Smooth mouse cursor compositing.
- **Declarative HTML/CSS DOM Engine (`kernel/gui/dom_engine.c`)**:
  - Declarative markup UI layout engine in C that renders modern flex containers, styled cards, rounded buttons, progress bars, and typography without the security overhead of full web browsers.
  - **Locked Kiosk Security**: Developer inspect tools / DOM tampering are disabled by design.
- **Built-in Applications**:
  - **Xenithra Security & Firewall Center**: Real-time monitor for active sessions, blocked hijack attempts, and firewall packet stats.
  - **Diagnostic Console**: Kernel telemetry and hardware diagnostic logs.

---

## 📁 Directory Structure

```
c:\Data\Coding\OS_Kernal
├── bootloader/             # UEFI Freestanding Bootloader (PE32+)
│   ├── efi.h               # Complete UEFI 2.8 freestanding specification header
│   ├── gop.h & gop.c       # Double-buffered GOP rendering engine
│   ├── ui.h & ui.c         # Sleek Windows-style graphical boot selector UI
│   └── main.c              # EfiMain, ELF64 loader, memory map, ExitBootServices
├── kernel/                 # 64-bit Secure Kernel
│   ├── arch/x86_64/
│   │   └── entry.asm       # 64-bit kernel entry point (System V AMD64 ABI)
│   ├── security/
│   │   ├── session.h & .c  # 128-bit Anti-Session-Hijack Guard & SMEP/SMAP
│   │   └── firewall.h & .c # Private stateful packet filter & network guard
│   ├── gui/
│   │   ├── compositor.h/.c # 32-bit Window Compositor, Taskbar, & Start Menu
│   │   └── dom_engine.h/.c # Declarative HTML/CSS DOM UI layout engine
│   ├── apps/
│   │   ├── firewall_app.h/.c # Graphical Firewall & Security Center App
│   │   └── terminal_app.h/.c # Secure Diagnostics Console App
│   ├── linker.ld           # Higher-half 64-bit ELF linker script (0xFFFFFFFF80000000)
│   └── main.c              # Kernel main & system initialization
├── shared/                 # Shared ABI headers
│   ├── bootinfo.h          # XenithraBootInfo & Framebuffer structs
│   └── font.h & font.c     # 8x16 bitmap font glyph table
├── scripts/                # Python Build & Simulation Utilities
│   ├── bootstrap_tools.py  # Portable toolchain installer (bypasses winget issues)
│   ├── build_disk.py       # Pure-Python FAT32 UEFI ESP disk image generator
│   ├── build_iso.py        # Standalone El Torito UEFI bootable ISO builder
│   └── download_ovmf.py    # OVMF UEFI firmware fetcher
├── build.ps1               # PowerShell automated build & test script
├── Makefile                # Multi-platform GNU Makefile
└── README.md               # Project documentation
```

---

## 🚀 Building and Running

### 1. Toolchain Setup (No Admin/Winget Required)

If `winget` fails on your system, run the automated portable toolchain bootstrapper:
```powershell
# Automatically downloads portable GCC/Clang, NASM, and OVMF into tools/
python scripts/bootstrap_tools.py
```
Or with `build.ps1`:
```powershell
.\build.ps1 -Bootstrap
```

*(Alternatively, install via winget: `winget install LLVM.LLVM NASM.NASM SoftwareFreedomConservancy.QEMU`)*

---

### 2. Build Disk Image & ISO:

- **On Windows (PowerShell):**
  ```powershell
  # Builds BOOTX64.EFI, kernel.elf, build/disk.img, and build/xenithra.iso
  .\build.ps1
  ```
- **On Linux / macOS:**
  ```bash
  make
  ```

---

### 3. Test in QEMU / VirtualBox:

- **Test in VirtualBox (Automated):**
  ```powershell
  .\build.ps1 -VBox
  ```
  *(Or run `powershell -File vm/setup_vbox.ps1`)*

- **Test Disk Image in QEMU:**
  ```powershell
  .\build.ps1 -Run
  ```
- **Test Bootable ISO (`xenithra.iso`) in QEMU:**
  ```powershell
  .\build.ps1 -Run -Iso
  ```

---

## 💿 VirtualBox Deployment & VM Configuration

We provide a pre-configured VirtualBox machine file: **[`vm/XenithraOS.vbox`](file:///c:/Data/Coding/OS_Kernal/vm/XenithraOS.vbox)**.

### Method A: One-Click Automatic Setup
Run:
```powershell
.\build.ps1 -VBox
```
This automatically registers the VM with optimal parameters (64-bit UEFI enabled, 2GB RAM, 2 CPUs, 128MB VRAM, AHCI SATA controller), attaches `build/xenithra.iso`, and starts the VM.

### Method B: Manual VirtualBox GUI Import
1. Open Oracle VM VirtualBox.
2. Click **Machine -> Add...** (`Ctrl+A`).
3. Select **`vm/XenithraOS.vbox`**.
4. Click **Start** to boot Xenithra OS directly.

### Method C: Create VM from Scratch
- **OS Type**: `Other_64` (64-bit).
- **RAM**: `2048 MB` (2 GB).
- **Processors**: `2 CPUs`.
- **System -> Motherboard -> Extended Features**: Check **Enable EFI (special OSes only)**.
- **Display -> Video Memory**: `128 MB`.
- **Storage**: Attach `build/xenithra.iso` to the Optical Drive.
