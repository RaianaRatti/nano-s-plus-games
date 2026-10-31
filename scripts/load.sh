#!/usr/bin/env bash
# Installs an app on a Nano S Plus connected by USB, or removes it: Badge Arcade, or with "pet" or
# "gallery" first, the pet or the gallery. Run it on the Mac itself, not in Docker: Docker on macOS
# cannot reach USB devices.
#
#   ./scripts/load.sh                          install Badge Arcade (builds first if bin/ is empty)
#   ./scripts/load.sh pet | gallery            install the pet or the gallery
#   ./scripts/load.sh [pet | gallery] remove   uninstall
#
# Before: plug the Nano S Plus in, enter the PIN, stay on the dashboard, quit Ledger Live.
# The device then asks you to allow the install; see README.md, "Put it on your Nano S Plus".
set -euo pipefail
cd "$(dirname "$0")/.."

TARGET_ID=0x33100004  # Nano S Plus

APP_DIR=.
BUILD_ARGS=()
case "${1:-}" in
  pet | gallery)
    APP_DIR=$1
    BUILD_ARGS=("$1")
    shift
    ;;
esac
# The name on the dashboard, from the app's Makefile.
APP_NAME="$(sed -n 's/^APPNAME = "\(.*\)"$/\1/p' "$APP_DIR/Makefile")"

# ledgerblue, Ledger's loader, in a virtualenv of its own.
if [ ! -x .venv/bin/python ]; then
  python3 -m venv .venv
  .venv/bin/pip install -q --upgrade pip
  .venv/bin/pip install -q ledgerblue
fi

if [ "${1:-}" = "remove" ]; then
  echo "Removing $APP_NAME. Confirm on the device."
  .venv/bin/python -m ledgerblue.deleteApp --targetId "$TARGET_ID" --appName "$APP_NAME"
  exit 0
fi

[ -f "$APP_DIR/bin/app.apdu" ] || ./scripts/build.sh "${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"}"

echo "Installing $APP_NAME. Watch the device: allow the install, then confirm."
.venv/bin/python -m ledgerblue.runScript --scp --fileName "$APP_DIR/bin/app.apdu" --elfFile "$APP_DIR/bin/app.elf"
echo "Done: $APP_NAME is on the dashboard."
