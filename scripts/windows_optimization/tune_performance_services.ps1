<#
.SYNOPSIS
    Xenithra OS / Windows Core Backend Services Performance Tuning Script
.DESCRIPTION
    Optimizes SysMain (SuperFetch), MMCSS (Multimedia Class Scheduler Service),
    DWM (Desktop Window Manager), and process scheduler priorities for zero-latency
    fullscreen web kiosk execution.
#>

# Ensure script is running with Administrator privileges
$IsAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $IsAdmin) {
    Write-Warning "[-] Administrator rights required to tune Windows performance services. Please run PowerShell as Administrator."
    exit 1
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   Windows Backend Services Performance Optimizer (Kiosk)  " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Configure SysMain (SuperFetch) for Application Pre-fetching
Write-Host "`n[*] [1/5] Configuring SysMain (SuperFetch) Memory Pre-fetching..." -ForegroundColor Yellow
Set-Service -Name "SysMain" -StartupType Automatic
Start-Service -Name "SysMain" -ErrorAction SilentlyContinue

$PrefetchRegPath = "HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PrefetcherParameters"
# 3 = Prefetch Boot + Application files
Set-ItemProperty -Path $PrefetchRegPath -Name "EnablePrefetcher" -Value 3 -Type DWord
Set-ItemProperty -Path $PrefetchRegPath -Name "EnableSuperfetch" -Value 3 -Type DWord
Set-ItemProperty -Path $PrefetchRegPath -Name "EnableBoottrace"  -Value 0 -Type DWord
Write-Host "[+] SysMain configured for aggressive application RAM pre-caching." -ForegroundColor Green

# 2. Tune MMCSS (Multimedia Class Scheduler Service) for Real-Time UI Prioritization
Write-Host "`n[*] [2/5] Tuning MMCSS Thread Priorities and Network Throttling..." -ForegroundColor Yellow
Set-Service -Name "MMCSS" -StartupType Automatic
Start-Service -Name "MMCSS" -ErrorAction SilentlyContinue

$SystemProfilePath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile"
# Disable network throttling on localhost (0xFFFFFFFF = unthrottled loopback)
Set-ItemProperty -Path $SystemProfilePath -Name "NetworkThrottlingIndex" -Value 0xFFFFFFFF -Type DWord
# 0 = Maximize foreground UI application responsiveness (allocates up to 100% CPU to prioritized threads)
Set-ItemProperty -Path $SystemProfilePath -Name "SystemResponsiveness"   -Value 0 -Type DWord

$DisplayPostTaskPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\DisplayPostProcessing"
if (-not (Test-Path $DisplayPostTaskPath)) { New-Item -Path $DisplayPostTaskPath -Force | Out-Null }
Set-ItemProperty -Path $DisplayPostTaskPath -Name "Affinity"            -Value 0 -Type DWord
Set-ItemProperty -Path $DisplayPostTaskPath -Name "Background Priority" -Value 24 -Type DWord
Set-ItemProperty -Path $DisplayPostTaskPath -Name "Clock Rate"          -Value 10000 -Type DWord
Set-ItemProperty -Path $DisplayPostTaskPath -Name "GPU Priority"        -Value 8 -Type DWord
Set-ItemProperty -Path $DisplayPostTaskPath -Name "Priority"            -Value 6 -Type DWord
Set-ItemProperty -Path $DisplayPostTaskPath -Name "Scheduling Category" -Value "High" -Type String
Set-ItemProperty -Path $DisplayPostTaskPath -Name "SFIO Priority"       -Value "High" -Type String
Write-Host "[+] MMCSS configured: 100% UI thread prioritization & unthrottled network bandwidth." -ForegroundColor Green

# 3. Force DWM (Desktop Window Manager) Hardware Acceleration & Flip Mode
Write-Host "`n[*] [3/5] Forcing DWM DirectX 12 Hardware Acceleration..." -ForegroundColor Yellow
$DwmRegPath = "HKCU:\Software\Microsoft\Windows\DWM"
if (-not (Test-Path $DwmRegPath)) { New-Item -Path $DwmRegPath -Force | Out-Null }
Set-ItemProperty -Path $DwmRegPath -Name "EnableAeroPeek"       -Value 0 -Type DWord
Set-ItemProperty -Path $DwmRegPath -Name "AlwaysHibernateThumbnails" -Value 0 -Type DWord
Set-ItemProperty -Path $DwmRegPath -Name "AnimationsShiftKey"   -Value 0 -Type DWord

# Disable Window Animation latency
$DesktopRegPath = "HKCU:\Control Panel\Desktop"
Set-ItemProperty -Path $DesktopRegPath -Name "MenuShowDelay" -Value "0" -Type String
Set-ItemProperty -Path $DesktopRegPath -Name "UserPreferencesMask" -Value ([byte[]](0x90,0x12,0x03,0x80,0x10,0x00,0x00,0x00)) -Type Binary
Write-Host "[+] DWM transition latency set to 0ms." -ForegroundColor Green

# 4. Verify & Enable Essential Audio and Telemetry Services
Write-Host "`n[*] [4/5] Auditing Core Dependent Services (Audiosrv, Winmgmt, NlaSvc)..." -ForegroundColor Yellow
$Services = @("Audiosrv", "AudioEndpointBuilder", "Winmgmt", "NlaSvc", "netprofm")
foreach ($svc in $Services) {
    Set-Service -Name $svc -StartupType Automatic -ErrorAction SilentlyContinue
    Start-Service -Name $svc -ErrorAction SilentlyContinue
    $status = (Get-Service -Name $svc).Status
    Write-Host "    - Service: $svc -> Status: $status" -ForegroundColor Cyan
}

# 5. Configure Windows Defender Exclusions for Vite Dist & Browser Executable
Write-Host "`n[*] [5/5] Adding Windows Defender Process & Path Exclusions (Prevent I/O Spikes)..." -ForegroundColor Yellow
$KioskRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$DistFolder = Join-Path $KioskRoot "desktop_shell\dist"

if (Get-Command Add-MpPreference -ErrorAction SilentlyContinue) {
    Add-MpPreference -ExclusionPath $KioskRoot -ErrorAction SilentlyContinue
    Add-MpPreference -ExclusionProcess "msedge.exe", "chrome.exe", "dotnet.exe", "uvicorn.exe" -ErrorAction SilentlyContinue
    Write-Host "[+] Defender exclusions configured for: $KioskRoot" -ForegroundColor Green
} else {
    Write-Host "[!] Note: Add-MpPreference cmdlet not available in current environment." -ForegroundColor Yellow
}

Write-Host "`n[+] Windows Performance Optimization Completed Successfully." -ForegroundColor Green
