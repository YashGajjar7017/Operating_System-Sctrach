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

# 2. Ensure ISO exists
if (-not (Test-Path $IsoPath)) {
    Write-Host "[*] build/xenithra.iso not found. Building now..." -ForegroundColor Yellow
    & powershell -File (Join-Path $RootDir "build.ps1")
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

# 5. Storage Controller & Attach ISO
Write-Host "[*] Attaching ISO ($IsoPath) as Bootable DVD..." -ForegroundColor White

# Ensure SATA AHCI controller exists
$CtlCheck = & $VBoxManage showvminfo $VmName
if ($CtlCheck -notmatch "AHCI") {
    & $VBoxManage storagectl $VmName --name "AHCI" --add sata --controller IntelAHCI --portcount 2 --bootable on
}

# Attach ISO to Port 0
& $VBoxManage storageattach $VmName `
    --storagectl "AHCI" `
    --port 0 `
    --device 0 `
    --type dvddrive `
    --medium $IsoPath

Write-Host "[+] VirtualBox VM '$VmName' successfully configured!" -ForegroundColor Green

# 6. Launch the VM
if (-not $NoStart) {
    Write-Host "[*] Launching '$VmName' in VirtualBox..." -ForegroundColor Cyan
    & $VBoxManage startvm $VmName
}
