<#
.SYNOPSIS
    AuraOS Automated Windows Build and Test Script
.DESCRIPTION
    Compiles the UEFI Bootloader (BOOTX64.EFI) and Higher-Half Kernel (kernel.elf),
    generates the FAT32 ESP Disk Image, and launches QEMU with OVMF firmware.
#>

param (
    [switch]$Run,
    [switch]$Iso,
    [switch]$Clean,
    [string]$QemuPath = "qemu-system-x86_64"
)

$ErrorActionPreference = "Stop"

$RootDir   = $PSScriptRoot
$BuildDir  = Join-Path $RootDir "build"
$BootDir   = Join-Path $RootDir "bootloader"
$KernDir   = Join-Path $RootDir "kernel"
$SharedDir = Join-Path $RootDir "shared"
$ScriptDir = Join-Path $RootDir "scripts"

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
Write-Host "       AuraOS x86_64 Build Pipeline (Phase 1 & 2)         " -ForegroundColor Cyan
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
    Write-Host "    Please install LLVM/Clang (e.g. winget install LLVM.LLVM) or run in an environment with Clang." -ForegroundColor Yellow
}

if (-not $Nasm) {
    Write-Host "[!] 'nasm' not found in PATH." -ForegroundColor Yellow
    Write-Host "    Please install NASM (e.g. winget install NASM.NASM)." -ForegroundColor Yellow
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
Write-Host "`n[2/4] Assembling & Compiling Kernel (kernel.elf)..." -ForegroundColor White
$KernelEntryAsm = Join-Path $KernDir "arch\x86_64\entry.asm"
$KernelEntryObj = Join-Path $BuildDir "kern_entry.o"
$KernelMainC    = Join-Path $KernDir "main.c"
$KernelMainObj  = Join-Path $BuildDir "kern_main.o"
$KernelFontC    = Join-Path $SharedDir "font.c"
$KernelFontObj  = Join-Path $BuildDir "kern_font.o"
$KernelElf      = Join-Path $BuildDir "kernel.elf"
$LinkerScript   = Join-Path $KernDir "linker.ld"

if ($Nasm -and $Clang) {
    & nasm -f elf64 $KernelEntryAsm -o $KernelEntryObj
    & clang -target x86_64-unknown-none-elf -ffreestanding -mno-red-zone -mcmodel=kernel -I$SharedDir -O2 -c $KernelMainC -o $KernelMainObj
    & clang -target x86_64-unknown-none-elf -ffreestanding -mno-red-zone -mcmodel=kernel -I$SharedDir -O2 -c $KernelFontC -o $KernelFontObj

    if ($Lld) {
        & ld.lld -T $LinkerScript -nostdlib $KernelEntryObj $KernelMainObj $KernelFontObj -o $KernelElf
    } else {
        & clang -target x86_64-unknown-none-elf -nostdlib "-Wl,-T,$LinkerScript" $KernelEntryObj $KernelMainObj $KernelFontObj -o $KernelElf
    }
    Write-Host "[+] kernel.elf built successfully." -ForegroundColor Green
} else {
    Write-Host "[!] Skipping kernel.elf compile (nasm / clang not present)." -ForegroundColor Red
}

# 3. Generate Disk and ISO Images
Write-Host "`n[3/4] Packaging FAT32 ESP Disk Image & Bootable ISO..." -ForegroundColor White
$DiskImg = Join-Path $BuildDir "disk.img"
$IsoImg  = Join-Path $BuildDir "auraos.iso"
$BuildDiskScript = Join-Path $ScriptDir "build_disk.py"
$BuildIsoScript  = Join-Path $ScriptDir "build_iso.py"

if ($Python) {
    & python $BuildDiskScript $DiskImg $BootEfi $KernelElf
    & python $BuildIsoScript $IsoImg $BootEfi $KernelElf
} else {
    Write-Host "[!] Python required to package disk/ISO image." -ForegroundColor Red
}

# 4. Download OVMF and Run in QEMU if requested
if ($Run) {
    Write-Host "`n[4/4] Setting up OVMF firmware and launching QEMU..." -ForegroundColor White
    $DownloadOvmfScript = Join-Path $ScriptDir "download_ovmf.py"
    if ($Python) {
        & python $DownloadOvmfScript
    }

    $OvmfPath = Join-Path $BuildDir "ovmf.fd"
    if (Test-Path $OvmfPath) {
        if ($Iso) {
            Write-Host "[+] Starting QEMU with Bootable ISO (CD-ROM)..." -ForegroundColor Green
            & $QemuPath -bios $OvmfPath -cdrom $IsoImg -m 2G -vga std -serial stdio -no-reboot
        } else {
            Write-Host "[+] Starting QEMU with UEFI ESP Disk Image..." -ForegroundColor Green
            & $QemuPath -bios $OvmfPath -drive format=raw,file=$DiskImg -m 2G -vga std -serial stdio -no-reboot
        }
    } else {
        Write-Host "[!] OVMF firmware file not found at $OvmfPath" -ForegroundColor Red
    }
} else {
    Write-Host "`n[+] Build Pipeline complete." -ForegroundColor Cyan
    Write-Host "    - Raw Disk Image: build/disk.img" -ForegroundColor Green
    Write-Host "    - Shareable ISO:  build/auraos.iso" -ForegroundColor Green
    Write-Host "    To test in QEMU:  .\build.ps1 -Run" -ForegroundColor White
    Write-Host "    To test ISO:      .\build.ps1 -Run -Iso" -ForegroundColor White
}
