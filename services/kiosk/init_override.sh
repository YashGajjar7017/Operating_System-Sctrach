#!/bin/bash
# ==============================================================================
# Xenithra OS - Kernel/Init Boot Pipeline Override
# 1. Bypasses legacy window manager / GDI GUI
# 2. Boots ASP.NET Core or Django System Daemon
# 3. Executes dependency readiness health-check
# 4. Launches Fullscreen Hardware-Accelerated Kiosk Shell
# ==============================================================================

set -e

BACKEND_TYPE="${1:-aspnet}" # "aspnet" or "django"
TARGET_PORT=5000
BACKEND_HOST="127.0.0.1"

echo "=========================================================="
echo "    Xenithra OS - Kiosk Shell & Daemon Boot Orchestrator   "
echo "=========================================================="

# 1. Kill legacy display managers (LightDM / GDM / SDDM / Custom GDI Window Manager)
echo "[*] Step 1: Disabling legacy GUI display managers..."
if command -v systemctl >/dev/null 2>&1; then
    systemctl stop display-manager 2>/dev/null || true
    systemctl stop lightdm 2>/dev/null || true
    systemctl stop gdm3 2>/dev/null || true
fi

# 2. Launch Selected System Backend Service in Background
echo "[*] Step 2: Starting Local System Controller Daemon (${BACKEND_TYPE})..."

if [ "$BACKEND_TYPE" = "aspnet" ]; then
    TARGET_PORT=5000
    export ASPNETCORE_URLS="http://${BACKEND_HOST}:${TARGET_PORT}"
    
    if [ -f "/opt/xenithra/daemon/SystemDaemon.dll" ]; then
        dotnet /opt/xenithra/daemon/SystemDaemon.dll &
    else
        echo "[!] Binary not found at /opt/xenithra/daemon/, running via local dotnet..."
        dotnet run --project /opt/xenithra/services/aspnet_core/SystemDaemon.csproj &
    fi
    BACKEND_PID=$!

elif [ "$BACKEND_TYPE" = "django" ]; then
    TARGET_PORT=8000
    echo "[*] Launching Django ASGI server via Uvicorn on port ${TARGET_PORT}..."
    uvicorn services.django_daemon.asgi:application --host ${BACKEND_HOST} --port ${TARGET_PORT} --workers 2 &
    BACKEND_PID=$!
fi

# 3. Wait for Local Service Readiness (Health Check Loop with /dev/tcp)
echo "[*] Step 3: Awaiting service readiness on http://${BACKEND_HOST}:${TARGET_PORT}..."

MAX_ATTEMPTS=30
ATTEMPT=0
READY=0

while [ $ATTEMPT -lt $MAX_ATTEMPTS ]; do
    if (echo > /dev/tcp/${BACKEND_HOST}/${TARGET_PORT}) >/dev/null 2>&1; then
        READY=1
        break
    fi
    ATTEMPT=$((ATTEMPT + 1))
    sleep 0.2
done

if [ $READY -ne 1 ]; then
    echo "[-] Error: Backend service failed to bind to port ${TARGET_PORT} within timeout!"
    exit 1
fi

echo "[+] Backend service ready on http://${BACKEND_HOST}:${TARGET_PORT} (PID: ${BACKEND_PID})."

# 4. Start Lightweight DRM/Wayland Cage or Minimal X11 Kiosk
echo "[*] Step 4: Initializing Fullscreen Stealth Desktop Shell..."

if command -v cage >/dev/null 2>&1; then
    # Wayland direct DRM/KMS kiosk (Best performance, zero X11 overhead)
    exec cage -- /opt/xenithra/services/kiosk/kiosk_run.sh "http://${BACKEND_HOST}:${TARGET_PORT}"
elif command -v xinit >/dev/null 2>&1; then
    # Minimal direct X11 single-window kiosk
    exec xinit /opt/xenithra/services/kiosk/kiosk_run.sh "http://${BACKEND_HOST}:${TARGET_PORT}" -- :0 -nocursor -nolisten tcp vt7
else
    # Direct execution if X / Wayland is already running
    exec /opt/xenithra/services/kiosk/kiosk_run.sh "http://${BACKEND_HOST}:${TARGET_PORT}"
fi
