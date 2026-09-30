# Xenithra OS v3.0 — High-Security 64-bit OS with Separate Render Engine

A custom x86_64 Operating System built from scratch, redesigned in v3.0 with a **strict architectural separation**:

- **C Kernel** → Pure backend: hardware, scheduling, security, IPC. **No GUI drawing.**
- **Render Engine** → Separate Electron + React + TypeScript process. **All rendering here.**
- **V8/Chromium** → Acts as the DWM compositor, replacing legacy C framebuffer code.

---

## 🏗️ v3.0 Architecture — Four Layers

```
┌─────────────────────────────────────────────────────────────────────┐
│  LAYER 4 — RENDER ENGINE  (render_engine/)                          │
│  Electron + React 18 + TypeScript + Vite                            │
│  • Fluent Design: Mica / Acrylic shaders (CSS backdrop-filter)      │
│  • Spring-physics window animations                                 │
│  • NetworkPanel: WiFi, Ethernet, VPN, DNS, Firewall rules           │
│  • ManagementPanel: Device Manager, Performance, Event Log          │
│  • Start Menu, Action Center, Toast notifications                   │
│  • Connects to kernel via  \\.\pipe\XenithraGUI  (Named Pipe)       │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ Named Pipe IPC (newline-delimited JSON)
┌───────────────────────────▼─────────────────────────────────────────┐
│  LAYER 3 — SERVICES  (services/)                                    │
│  C daemons running as kernel threads in Session 0                   │
│  • netmgr/   — DHCP client, DNS resolver, routing table             │
│  • drvmgr/   — PCI enumeration, PnP driver manager                  │
│  • panelmgr/ — Telemetry collector → GUI IPC publisher             │
│  • dwm_proxy, sysmain, mmcss, audiosrv, wmi                         │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ Syscall / direct call
┌───────────────────────────▼─────────────────────────────────────────┐
│  LAYER 2 — KERNEL  (kernel/)   ← C ONLY, ZERO GUI DRAWING           │
│  • arch/x86_64: GDT/IDT/TSS/APIC/SMP                               │
│  • mm:          VMM, PFN database, 64MB kernel heap                 │
│  • sched:       RTOS 6-level priority scheduler + DPC queue         │
│  • security:    Session guard (128-bit), SMEP/SMAP, Firewall        │
│  • drivers:     PS/2, Sound, e1000, RTL8139, xHCI, VBE, ACPI       │
│  • gui/gui_ipc: Named Pipe IPC server (ONLY gui file kept)          │
└───────────────────────────┬─────────────────────────────────────────┘
                            │ UEFI Handoff
┌───────────────────────────▼─────────────────────────────────────────┐
│  LAYER 1 — BOOTLOADER  (bootloader/)   ← UNCHANGED                  │
│  • UEFI PE32+ EFI application                                       │
│  • GOP graphical boot selector                                      │
│  • ELF64 kernel loader + memory map                                 │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 📁 Directory Structure (v3.0)

```
OS_Kernal/
├── bootloader/                    # UEFI bootloader (C, UNCHANGED)
├── kernel/                        # 64-bit kernel — C ONLY, NO GUI
│   ├── arch/x86_64/               # GDT, IDT, APIC, SMP trampoline
│   ├── mm/                        # VMM, PFN database, heap
│   ├── exec/                      # KPCR, SMSS, kshell
│   ├── sched/                     # RTOS priority scheduler
│   ├── security/                  # session.c, firewall.c
│   ├── drivers/
│   │   ├── ps2.c/.h               # PS/2 mouse + keyboard
│   │   ├── sound.c/.h             # PC Speaker / HDA audio
│   │   ├── net/
│   │   │   ├── e1000.h            # ★ NEW: Intel e1000 Gigabit NIC
│   │   │   └── rtl8139.h          # ★ NEW: Realtek RTL8139 NIC
│   │   ├── gpu/
│   │   │   └── vbe.h              # ★ NEW: VESA VBE framebuffer (boot only)
│   │   ├── usb/
│   │   │   └── xhci.h             # ★ NEW: USB 3.0 xHCI controller
│   │   └── acpi/
│   │       └── acpi.h             # ★ NEW: ACPI power management
│   ├── gui/
│   │   └── gui_ipc.c/.h           # Named Pipe IPC server (ONLY gui file)
│   │   # REMOVED: compositor.c, v8_engine.c, dom_engine.c, anim.c
│   ├── apps/                      # Syscall providers (IPC-backed, no draw)
│   └── main.c                     # ★ UPDATED: no C drawing, adds new drivers
│
├── render_engine/                 # ★ NEW: Separate Render Engine
│   ├── electron/
│   │   ├── main.cjs               # Electron main process
│   │   ├── preload.cjs            # Context bridge (xenithra API)
│   │   └── ipc_bridge.cjs         # Named Pipe ↔ IPC relay
│   ├── src/
│   │   ├── design/
│   │   │   ├── tokens.css         # All CSS custom properties
│   │   │   ├── fluent.css         # Mica/Acrylic/Glass components
│   │   │   └── animations.css     # Micro-animations library
│   │   ├── components/
│   │   │   ├── DesktopShell.tsx   # Main OS shell
│   │   │   ├── NetworkPanel.tsx   # ★ NEW: Network management
│   │   │   └── ManagementPanel.tsx# ★ NEW: System management
│   │   ├── types.ts               # ★ EXTENDED: network + driver types
│   │   ├── index.css              # CSS entry (imports all layers)
│   │   └── main.tsx               # React root
│   ├── package.json
│   ├── vite.config.ts             # Port 5174
│   └── tsconfig.json
│
├── services/                      # C backend daemons
│   ├── netmgr/
│   │   └── netmgr.h               # ★ NEW: DHCP, DNS, routing
│   ├── drvmgr/
│   │   └── drvmgr.h               # ★ NEW: PCI PnP driver manager
│   ├── panelmgr/
│   │   └── panelmgr.h             # ★ NEW: Telemetry → IPC publisher
│   ├── dwm_proxy/                 # DWM compositor proxy
│   ├── sysmain/                   # Session lifecycle
│   ├── mmcss/                     # Multimedia Class Scheduler
│   ├── audiosrv/                  # Audio service
│   └── wmi/                       # WMI provider
│
├── shared/                        # ABI headers (bootinfo.h, font.h)
├── desktop_shell/                 # Legacy shell (kept for reference)
├── build.ps1                      # ★ UPDATED: builds kernel + render_engine
├── Makefile                       # GNU Makefile (Linux/macOS)
└── README.md                      # This file
```

---

## 🛡️ Security Architecture (UNCHANGED)

1. **Anti-Session-Hijacking** (`kernel/security/session.c`):
   - 128-bit cryptographic session tokens (RDRAND/RDSEED + jitter)
   - Token bound to (SessionID, PID, UID, Ring, Capabilities, IP)

2. **Hardware SMEP + SMAP** (`CR4.bit20/21`):
   - Prevents ring-3 shellcode execution in kernel mode

3. **Kernel Private Firewall** (`kernel/security/firewall.c`):
   - Stateful TCP/UDP/ICMP packet filter
   - Managed via `NetworkPanel → Firewall` tab in Render Engine

---

## ★ New in v3.0

| Feature | Description |
|---------|-------------|
| `render_engine/` | Separate Electron + React + TypeScript render engine |
| `NetworkPanel` | WiFi scanner, VPN, DNS, Firewall UI — fully interactive |
| `ManagementPanel` | Device Manager, Performance graphs, Event Log, Startup |
| `e1000` driver | Intel 82540EM/82574L Gigabit NIC (QEMU compatible) |
| `rtl8139` driver | Realtek RTL8139 Fast Ethernet (QEMU fallback) |
| `xhci` driver | USB 3.0 xHCI host controller |
| `vbe` driver | VESA framebuffer (boot splash + kernel panic only) |
| `acpi` driver | ACPI power management (S0-S5, battery, reboot/shutdown) |
| `netmgr` service | DHCP client, DNS resolver, routing table daemon |
| `drvmgr` service | PCI bus scanner, PnP driver loader, hot-plug support |
| `panelmgr` service | 1-second telemetry publisher → ManagementPanel |
| Fluent Design System | Mica/Acrylic shaders, Inter/Outfit fonts, spring animations |
| Web3 Integration | EIP-6963 wallets, ENS, IPFS — in `Web3App` |

---

## 🚀 Building and Running

### 1. Install dependencies

```powershell
# Kernel + Bootloader toolchain
python scripts/bootstrap_tools.py

# Render Engine (NEW — separate from desktop_shell)
cd render_engine
npm install
cd ..
```

### 2. Build everything

```powershell
# Build kernel ELF + UEFI bootloader + disk image
.\build.ps1

# Build render engine (dev mode)
.\build.ps1 -RenderEngine

# Build all layers
.\build.ps1 -All
```

### 3. Run in QEMU

```powershell
# Boot kernel in QEMU
.\build.ps1 -Run

# Start render engine separately (dev mode, port 5174)
.\build.ps1 -Shell
```

### 4. Test in VirtualBox

```powershell
.\build.ps1 -VBox
```

---

## 🖥️ Render Engine Architecture

```
User Input / Event
      ↓
UI Thread / Dispatcher  →  React Tree Mutation
      ↓
Layout pass (CSS Flexbox)
      ↓
WebGL + Canvas Draw Calls  +  DirectWrite (fonts via canvas)
      ↓
Electron BrowserWindow (Chromium GPU compositor)
      ↓
Named Pipe \\.\pipe\XenithraGUI
      ↓
Kernel IPC Server (gui_ipc.c)
      ↓
Kernel Subsystems (security, network, scheduler, drivers)
```

### Pixel Shader Pipeline (CSS)
- **Mica Shader**: `backdrop-filter: blur(40px) saturate(160%)` — pre-blurred bg
- **Acrylic Shader**: `backdrop-filter: blur(24px) saturate(200%)` — dynamic blur
- **Drop shadows**: `box-shadow` with multi-layer depth
- **Rounded corners**: `border-radius` matching Windows 11 geometry

---

## 💿 VirtualBox / QEMU Configuration

| Setting | Value |
|---------|-------|
| OS Type | Other 64-bit |
| RAM | 2048 MB |
| CPUs | 2 |
| EFI | Enabled |
| VRAM | 128 MB |
| Network | e1000 (Intel) adapter |
| USB | xHCI (USB 3.0) |
| Display | VBoxVGA / VMSVGA |
