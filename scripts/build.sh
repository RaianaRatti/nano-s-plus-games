#!/usr/bin/env bash
# Builds the app for the Nano S Plus inside Ledger's official builder image.
# Output: bin/app.elf, bin/app.hex, bin/app.apdu (what scripts/load.sh installs).
#
#   ./scripts/build.sh                  for the newest firmware (the image's SDK: API level 27)
#   API_LEVEL=26 ./scripts/build.sh     for Nano S Plus firmware 1.6.0 (the SDK branch of API 26)
#   ./scripts/build.sh clean            remove the build
#
# The app must be built for the API level of the firmware on the device, or the device refuses
# it. Older levels (25 and before) no longer build with the image's compiler: update the device.
set -euo pipefail
cd "$(dirname "$0")/.."

IMAGE=ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
API_LEVEL="${API_LEVEL:-}"

# A build for another API level starts from scratch.
mkdir -p build
if [ "$(cat build/.api_level 2>/dev/null || true)" != "$API_LEVEL" ]; then
  rm -rf build/nanos2 bin debug
fi
echo "$API_LEVEL" > build/.api_level

docker run --rm -v "$PWD:/app" -w /app -e API_LEVEL="$API_LEVEL" "$IMAGE" bash -c '
  set -e
  mkdir -p glyphs
  [ -f icons/arcade_14px.gif ] || python3 tools/make_icon.py
  SDK=$NANOSP_SDK
  if [ -n "$API_LEVEL" ]; then
    cp -r /opt/ledger-secure-sdk /tmp/sdk
    git -C /tmp/sdk checkout -q "origin/API_LEVEL_$API_LEVEL"
    SDK=/tmp/sdk
  fi
  make BOLOS_SDK=$SDK TARGET=nanos2 "$@"
' _ "$@"
