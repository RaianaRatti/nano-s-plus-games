#!/usr/bin/env bash
# Builds an app for the Nano S Plus inside Ledger's official builder image: Badge Arcade, or with
# "pet" or "gallery" first, the pet or the gallery. Output: bin/ (or pet/bin/, gallery/bin/): app.elf,
# app.hex, app.apdu (what load.sh installs).
#
#   ./scripts/build.sh                  Badge Arcade, for the newest firmware (API level 27)
#   ./scripts/build.sh pet              the pet
#   ./scripts/build.sh gallery          the gallery (its works.c is made by tools/make_gallery.py)
#   API_LEVEL=26 ./scripts/build.sh     for Nano S Plus firmware 1.6.0 (the SDK branch of API 26)
#   ./scripts/build.sh [pet] clean      remove the build
#
# The app must be built for the API level of the firmware on the device, or the device refuses
# it. Older levels (25 and before) no longer build with the image's compiler: update the device.
set -euo pipefail
cd "$(dirname "$0")/.."

APP_DIR=.
case "${1:-}" in
  pet | gallery)
    APP_DIR=$1
    shift
    ;;
esac

IMAGE=ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
API_LEVEL="${API_LEVEL:-}"

# A build for another API level starts from scratch.
mkdir -p "$APP_DIR/build"
if [ "$(cat "$APP_DIR/build/.api_level" 2>/dev/null || true)" != "$API_LEVEL" ]; then
  rm -rf "$APP_DIR/build/nanos2" "$APP_DIR/bin" "$APP_DIR/debug"
fi
echo "$API_LEVEL" > "$APP_DIR/build/.api_level"

docker run --rm -v "$PWD:/app" -w "/app/$APP_DIR" -e API_LEVEL="$API_LEVEL" "$IMAGE" bash -c '
  set -e
  mkdir -p glyphs
  [ -f /app/icons/arcade_14px.gif ] && [ -f /app/pet/icons/pet_14px.gif ] && [ -f /app/gallery/icons/gallery_14px.gif ] ||
    python3 /app/tools/make_icon.py
  SDK=$NANOSP_SDK
  if [ -n "$API_LEVEL" ]; then
    cp -r /opt/ledger-secure-sdk /tmp/sdk
    git -C /tmp/sdk checkout -q "origin/API_LEVEL_$API_LEVEL"
    SDK=/tmp/sdk
  fi
  make BOLOS_SDK=$SDK TARGET=nanos2 "$@"
' _ "$@"
