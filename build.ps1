<#
.SYNOPSIS
    Xenithra OS Automated Windows Build, Bootstrap, and Test Script
.DESCRIPTION
    Compiles the UEFI Bootloader (BOOTX64.EFI) and Higher-Half Secure Kernel (kernel.elf),
    generates the FAT32 ESP Disk Image, packages the bootable xenithra.iso, and launches QEMU.
#>

param (
    [switch]$Run,
    [switch]$Iso,
    [switch]$VBox,
    [switch]$Clean,
    [switch]$Bootstrap,
    [string]$QemuPath = "qemu-system-x86_64"
)

$ErrorActionPreference = "Stop"

$RootDir   = $PSScriptRoot
$BuildDir  = Join-Path $RootDir "build"
$BootDir   = Join-Path $RootDir "bootloader"
$KernDir   = Join-Path $RootDir "kernel"
$SharedDir = Join-Path $RootDir "shared"
$ScriptDir = Join-Path $RootDir "scripts"
$ToolsDir  = Join-Path $RootDir "tools"
$VmDir     = Join-Path $RootDir "vm"

# Add portable tools/ directory to PATH if present
$W64Bin  = Join-Path $ToolsDir "w64devkit\bin"
$NasmBin = Join-Path $ToolsDir "nasm"
if (Test-Path $W64Bin)  { $env:PATH = "$W64Bin;" + $env:PATH }
if (Test-Path $NasmBin) { $env:PATH = "$NasmBin;" + $env:PATH }

if ($Bootstrap) {
    Write-Host "[*] Bootstrapping portable toolchain..." -ForegroundColor Cyan
    & python (Join-Path $ScriptDir "bootstrap_tools.py")
    exit 0
}

if ($Clean) {
    Write-Host "[*] Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    Write-Host "[+] Clean complete." -ForegroundColor Green
    if (-not $Run) { exit 0 }
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "       Xenithra OS x86_64 High-Security Build Pipeline    " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Check for required tools
$Clang = Get-Command clang -ErrorAction SilentlyContinue
$Nasm  = Get-Command nasm -ErrorAction SilentlyContinue
$Lld   = Get-Command ld.lld -ErrorAction SilentlyContinue
$Python= Get-Command python -ErrorAction SilentlyContinue

if (-not $Python) {
    $Python = Get-Command python3 -ErrorAction SilentlyContinue
}

if (-not $Clang) {
    Write-Host "[!] 'clang' not found in PATH." -ForegroundColor Yellow
    Write-Host "    You can run 'python scripts/bootstrap_tools.py' or '.\build.ps1 -Bootstrap' to get portable compilers." -ForegroundColor Yellow
}

if (-not $Nasm) {
    Write-Host "[!] 'nasm' not found in PATH." -ForegroundColor Yellow
}

# 1. Compile UEFI Bootloader
Write-Host "`n[1/4] Compiling UEFI Bootloader (BOOTX64.EFI)..." -ForegroundColor White
$BootSources = @(
    (Join-Path $BootDir "main.c"),
    (Join-Path $BootDir "gop.c"),
    (Join-Path $BootDir "ui.c"),
    (Join-Path $SharedDir "font.c")
)

$BootEfi = Join-Path $BuildDir "BOOTX64.EFI"

if ($Clang) {
    & clang -target x86_64-unknown-windows `
        -ffreestanding -fshort-wchar -mno-red-zone `
        -I$SharedDir -I$BootDir `
        -nostdlib "-Wl,-subsystem:efi_application" "-Wl,-entry:EfiMain" `
        -O2 -o $BootEfi $BootSources
    Write-Host "[+] BOOTX64.EFI built successfully." -ForegroundColor Green
} else {
    Write-Host "[!] Skipping BOOTX64.EFI compile (clang not present)." -ForegroundColor Red
}

# 2. Assemble and Compile Kernel
Write-Host "`n[2/4] Assembling & Compiling Kernel with Security Subsystems (kernel.elf)..." -ForegroundColor White
$KernelEntryAsm = Join-Path $KernDir "arch\x86_64\entry.asm"
$KernelEntryObj = Join-Path $BuildDir "kern_entry.o"
$KernelElf      = Join-Path $BuildDir "kernel.elf"
$LinkerScript   = Join-Path $KernDir "linker.ld"

$KernelSources = @(
    @{ Src = (Join-Path $KernDir "main.c"); Obj = (Join-Path $BuildDir "kern_main.o") },
    @{ Src = (Join-Path $KernDir "security\session.c"); Obj = (Join-Path $BuildDir "kern_session.o") },
    @{ Src = (Join-Path $KernDir "security\firewall.c"); Obj = (Join-Path $BuildDir "kern_firewall.o") },
    @{ Src = (Join-Path $KernDir "gui\compositor.c"); Obj = (Join-Path $BuildDir "kern_compositor.o") },
    @{ Src = (Join-Path $KernDir "gui\dom_engine.c"); Obj = (Join-Path $BuildDir "kern_dom.o") },
    @{ Src = (Join-Path $KernDir "apps\firewall_app.c"); Obj = (Join-Path $BuildDir "kern_app_firewall.o") },
    @{ Src = (Join-Path $KernDir "apps\terminal_app.c"); Obj = (Join-Path $BuildDir "kern_app_terminal.o") },
    @{ Src = (Join-Path $SharedDir "font.c"); Obj = (Join-Path $BuildDir "kern_font.o") }
)

if ($Nasm -and $Clang) {
    & nasm -f elf64 $KernelEntryAsm -o $KernelEntryObj

    $ObjList = @($KernelEntryObj)
    foreach ($item in $KernelSources) {
        & clang -target x86_64-unknown-none-elf -ffreestanding -mno-red-zone -mcmodel=kernel -I$SharedDir -I$KernDir -O2 -c $item.Src -o $item.Obj
        $ObjList += $item.Obj
    }

    if ($Lld) {
        & ld.lld -T $LinkerScript -nostdlib $ObjList -o $KernelElf
    } else {
        & clang -target x86_64-unknown-none-elf -nostdlib "-Wl,-T,$LinkerScript" $ObjList -o $KernelElf
    }
    Write-Host "[+] kernel.elf (Security & GUI Engine) built successfully." -ForegroundColor Green
} else {
    Write-Host "[!] Skipping kernel.elf compile (nasm / clang not present)." -ForegroundColor Red
}

# 3. Generate Disk and ISO Images
Write-Host "`n[3/4] Packaging FAT32 ESP Disk Image & Bootable xenithra.iso..." -ForegroundColor White
$DiskImg = Join-Path $BuildDir "disk.img"
$IsoImg  = Join-Path $BuildDir "xenithra.iso"
$BuildDiskScript = Join-Path $ScriptDir "build_disk.py"
$BuildIsoScript  = Join-Path $ScriptDir "build_iso.py"

if ($Python) {
    & python $BuildDiskScript $DiskImg $BootEfi $KernelElf
    & python $BuildIsoScript $IsoImg $BootEfi $KernelElf
} else {
    Write-Host "[!] Python required to package disk/ISO image." -ForegroundColor Red
}

# 4. Run in VirtualBox or QEMU if requested
if ($VBox) {
    Write-Host "`n[4/4] Launching Xenithra OS in VirtualBox..." -ForegroundColor White
    & powershell -File (Join-Path $VmDir "setup_vbox.ps1")
} elseif ($Run) {
    Write-Host "`n[4/4] Setting up OVMF firmware and launching QEMU..." -ForegroundColor White
    $DownloadOvmfScript = Join-Path $ScriptDir "download_ovmf.py"
    if ($Python) {
        & python $DownloadOvmfScript
    }

    $OvmfPath = Join-Path $BuildDir "ovmf.fd"
    if (Test-Path $OvmfPath) {
        if ($Iso) {
            Write-Host "[+] Starting QEMU with Xenithra Bootable ISO (CD-ROM)..." -ForegroundColor Green
            & $QemuPath -bios $OvmfPath -cdrom $IsoImg -m 2G -vga std -serial stdio -no-reboot
        } else {
            Write-Host "[+] Starting QEMU with Xenithra UEFI ESP Disk Image..." -ForegroundColor Green
            & $QemuPath -bios $OvmfPath -drive format=raw,file=$DiskImg -m 2G -vga std -serial stdio -no-reboot
        }
    } else {
        Write-Host "[!] OVMF firmware file not found at $OvmfPath" -ForegroundColor Red
    }
} else {
    Write-Host "`n[+] Build Pipeline complete." -ForegroundColor Cyan
    Write-Host "    - Raw Disk Image:   build/disk.img" -ForegroundColor Green
    Write-Host "    - Shareable ISO:    build/xenithra.iso" -ForegroundColor Green
    Write-Host "    - VirtualBox VM:    vm/XenithraOS.vbox" -ForegroundColor Green
    Write-Host "    To test in QEMU:    .\build.ps1 -Run" -ForegroundColor White
    Write-Host "    To test ISO (QEMU): .\build.ps1 -Run -Iso" -ForegroundColor White
    Write-Host "    To run VirtualBox:  .\build.ps1 -VBox" -ForegroundColor White
}
