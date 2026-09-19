@echo off
:: ============================================================================
:: Custom Shell Setup & Recovery Script — Xenithra OS
:: Requires Administrative Privileges
:: ============================================================================

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [!] Error: This script must be executed as Administrator.
    echo Right-click setup_shell.bat and select "Run as Administrator".
    pause
    exit /b 1
)

echo ============================================================================
echo                      Xenithra OS Shell Setup Utility
echo ============================================================================
echo.

if not exist "C:\CustomShell" (
    mkdir "C:\CustomShell"
    echo [+] Created deployment folder C:\CustomShell
)

echo [*] 1. Creating Registry Backup of current Winlogon Shell state...
reg query "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" /v Shell > C:\CustomShell\backup_winlogon.reg 2>nul
echo [+] Backup saved to C:\CustomShell\backup_winlogon.reg

echo.
echo [*] 2. Registering C:\CustomShell\native_shell_host.exe as active Winlogon Shell...
reg add "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" /v Shell /t REG_SZ /d "C:\CustomShell\native_shell_host.exe" /f

if %errorlevel% equ 0 (
    echo [+] Successfully configured Custom Shell in Windows registry!
) else (
    echo [!] Failed to update registry key.
)

echo.
echo ============================================================================
echo EMERGENCY ROLLBACK COMMAND (Run if stuck or testing):
echo reg add "HKCU\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" /v Shell /t REG_SZ /d "explorer.exe" /f
echo ============================================================================
echo.
pause
