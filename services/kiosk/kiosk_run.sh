#!/bin/bash
# ==============================================================================
# Xenithra OS - Production Stealth Kiosk Browser Startup Script
# Completely disables all browser chrome, dialogs, crash bubbles, and key escapes
# ==============================================================================

set -e

TARGET_URL="${1:-http://127.0.0.1:5000}"
USER_DATA_DIR="/tmp/xenithra_kiosk_profile_$(id -u)"

# 1. Clean previous crash state to prevent "Restore Session" dialogs
rm -rf "${USER_DATA_DIR}/Default/Preferences"
mkdir -p "${USER_DATA_DIR}/Default"

cat <<EOF > "${USER_DATA_DIR}/Default/Preferences"
{
  "profile": {
    "exit_type": "Normal",
    "exited_cleanly": true
  },
  "session": {
    "restore_on_startup": 4
  }
}
EOF

# 2. Hide mouse cursor when inactive (after 1 second)
if command -v unclutter >/dev/null 2>&1; then
    unclutter -idle 1 -root &
elif command -v unclutter-xfixes >/dev/null 2>&1; then
    unclutter-xfixes --timeout 1 --hide-on-touch &
fi

# 3. Disable screensaver & display power management signaling (DPMS)
if [ -n "$DISPLAY" ]; then
    xset s off 2>/dev/null || true
    xset -dpms 2>/dev/null || true
    xset s noblank 2>/dev/null || true
fi

# 4. Launch Hardware-Accelerated Stealth Chromium / WebKit Kiosk
# Note: Every parameter is tuned to hide browser identity and guarantee 60fps compositor fluidness
exec chromium-browser \
    --app="${TARGET_URL}" \
    --kiosk \
    --start-fullscreen \
    --window-position=0,0 \
    --no-first-run \
    --noerrdialogs \
    --disable-infobars \
    --disable-session-crashed-bubble \
    --disable-features=TranslateUI,InterestFeedContentSuggestions,CalculateNativeWinOcclusion \
    --disable-component-update \
    --disable-popup-blocking \
    --disable-background-networking \
    --disable-sync \
    --disable-default-apps \
    --disable-pinch \
    --hide-scrollbars \
    --incognito \
    --user-data-dir="${USER_DATA_DIR}" \
    --check-for-update-interval=31536000 \
    --enable-gpu-rasterization \
    --enable-zero-copy \
    --ignore-gpu-blocklist \
    --autoplay-policy=no-user-gesture-required \
    --password-store=basic
