#!/usr/bin/env bash
# Installs Badge Arcade on a Nano S Plus connected by USB, or removes it.
# Run it on the Mac itself, not in Docker: Docker on macOS cannot reach USB devices.
#
#   ./scripts/load.sh            install (builds first if bin/ is empty)
#   ./scripts/load.sh remove     uninstall
#
# Before: plug the Nano S Plus in, enter the PIN, stay on the dashboard, quit Ledger Live.
# The device then asks you to allow the install; see README.md, "Put it on your Nano S Plus".
set -euo pipefail
cd "$(dirname "$0")/.."

TARGET_ID=0x33100004  # Nano S Plus
APP_NAME="Badge Arcade"

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

[ -f bin/app.apdu ] || ./scripts/build.sh

echo "Installing $APP_NAME. Watch the device: allow the install, then confirm."
.venv/bin/python -m ledgerblue.runScript --scp --fileName bin/app.apdu --elfFile bin/app.elf
echo "Done: Badge Arcade is on the dashboard."
