<#
.SYNOPSIS
    Automated Oracle VM VirtualBox Setup & Launcher for Xenithra OS
.DESCRIPTION
    Creates, configures, and starts a VirtualBox VM with 64-bit UEFI, 2GB RAM,
    and mounts build/xenithra.iso automatically.
#>

param (
    [string]$VmName = "Xenithra OS",
    [switch]$NoStart
)

$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $PSScriptRoot
$IsoPath = Join-Path $RootDir "build\xenithra.iso"

# 1. Locate VBoxManage.exe
$VBoxManage = Get-Command VBoxManage.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue

if (-not $VBoxManage) {
    $DefaultPaths = @(
        "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe",
        "C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe",
        "$env:VBOX_MSI_INSTALL_PATH\VBoxManage.exe"
    )
    foreach ($p in $DefaultPaths) {
        if ($p -and (Test-Path $p)) {
            $VBoxManage = $p
            break
        }
    }
}

if (-not $VBoxManage) {
    Write-Host "[!] VirtualBox (VBoxManage.exe) was not found on your system." -ForegroundColor Red
    Write-Host "    Please ensure Oracle VM VirtualBox is installed." -ForegroundColor Yellow
    Write-Host "    You can also double-click 'vm\XenithraOS.vbox' to import the VM directly in VirtualBox GUI." -ForegroundColor Cyan
    exit 1
}

Write-Host "[+] Found VirtualBox CLI: $VBoxManage" -ForegroundColor Green

# 2. Ensure ISO exists and is valid
if (-not (Test-Path $IsoPath)) {
    Write-Host "[*] build/xenithra.iso not found. Building now..." -ForegroundColor Yellow
    & python (Join-Path $RootDir "scripts\build_iso.py") $IsoPath (Join-Path $RootDir "build\BOOTX64.EFI") (Join-Path $RootDir "build\kernel.elf")
}

if (-not (Test-Path $IsoPath)) {
    Write-Host "[!] Error: Failed to locate or generate $IsoPath" -ForegroundColor Red
    exit 1
}

# 3. Check if VM already exists
$ExistingVms = & $VBoxManage list vms
$VmExists = $ExistingVms -match "`"$VmName`""

if ($VmExists) {
    Write-Host "[+] Existing VirtualBox VM '$VmName' found. Updating configuration..." -ForegroundColor Cyan
} else {
    Write-Host "[*] Creating new VirtualBox VM '$VmName'..." -ForegroundColor Cyan
    & $VBoxManage createvm --name $VmName --ostype "Other_64" --register
}

# 4. Apply Hardware & UEFI Configuration
Write-Host "[*] Configuring 64-bit UEFI, 2GB RAM, 2 CPUs, and 128MB VRAM..." -ForegroundColor White
& $VBoxManage modifyvm $VmName `
    --firmware efi `
    --memory 2048 `
    --cpus 2 `
    --vram 128 `
    --graphicscontroller vboxsvga `
    --ioapic on `
    --pae on `
    --longmode on `
    --hpet on `
    --boot1 dvd `
    --boot2 disk `
    --boot3 none `
    --boot4 none `
    --nic1 nat `
    --nictype1 82540EM `
    --mouse usbtablet `
    --keyboard ps2

$DiskImg = Join-Path $RootDir "build\disk.img"
$VdiPath = Join-Path $RootDir "build\disk.vdi"

# Convert disk.img to disk.vdi if disk.img is present and disk.vdi is missing or older
if (Test-Path $DiskImg) {
    if ((-not (Test-Path $VdiPath)) -or ((Get-Item $DiskImg).LastWriteTime -gt (Get-Item $VdiPath).LastWriteTime)) {
        Write-Host "[*] Converting disk.img to VirtualBox native VDI format ($VdiPath)..." -ForegroundColor Cyan
        if (Test-Path $VdiPath) {
            & $VBoxManage closemedium disk $VdiPath --delete -ErrorAction SilentlyContinue 2>$null
            Remove-Item $VdiPath -Force -ErrorAction SilentlyContinue
        }
        & $VBoxManage convertfromraw $DiskImg $VdiPath --format VDI
    }
}

# 5. Storage Controller & Attach Media
Write-Host "[*] Attaching Bootable ISO ($IsoPath) & VDI Hard Disk ($VdiPath)..." -ForegroundColor White

# Ensure SATA AHCI controller exists with 4 ports
$CtlCheck = & $VBoxManage showvminfo $VmName --machinereadable
if (($CtlCheck -match 'storagecontrollername.*"AHCI"') -eq $null -or ($CtlCheck -match 'storagecontrollername.*"AHCI"').Count -eq 0) {
    & $VBoxManage storagectl $VmName --name "AHCI" --add sata --controller IntelAHCI --portcount 4 --bootable on
}

# Attach ISO to Port 0 (DVD Drive)
& $VBoxManage storageattach $VmName `
    --storagectl "AHCI" `
    --port 0 `
    --device 0 `
    --type dvddrive `
    --medium $IsoPath

# Attach VDI to Port 1 (Hard Disk) if present
if (Test-Path $VdiPath) {
    & $VBoxManage storageattach $VmName `
        --storagectl "AHCI" `
        --port 1 `
        --device 0 `
        --type hdd `
        --medium $VdiPath
}

Write-Host "[+] VirtualBox VM '$VmName' successfully configured!" -ForegroundColor Green

# 6. Launch the VM
if (-not $NoStart) {
    Write-Host "[*] Launching '$VmName' in VirtualBox..." -ForegroundColor Cyan
    & $VBoxManage startvm $VmName
}


