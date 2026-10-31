#!/usr/bin/env bash
# Runs the built app in Speculos, Ledger's Nano S Plus emulator. No device needed.
#
#   ./scripts/emulate.sh          open http://localhost:5001 in a browser and play
#                                 (the page has the screen and the two buttons; keyboard: arrows)
#   ./scripts/emulate.sh tour     click through every screen and save screenshots/
#   ./scripts/emulate.sh test     run the automated tests
set -euo pipefail
cd "$(dirname "$0")/.."

IMAGE=ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
NAME=arcade-speculos

[ -f bin/app.elf ] || ./scripts/build.sh

docker rm -f "$NAME" >/dev/null 2>&1 || true
docker run -d --name "$NAME" -v "$PWD:/app" -p 5001:5000 "$IMAGE" \
  speculos --model nanosp /app/bin/app.elf --display headless --api-port 5000 >/dev/null

for _ in $(seq 1 30); do
  curl -s localhost:5001/events >/dev/null 2>&1 && break
  sleep 1
done

case "${1:-}" in
  tour)
    docker exec -w /app/tests "$NAME" python3 tour.py
    docker rm -f "$NAME" >/dev/null
    ;;
  test)
    docker exec -w /app/tests "$NAME" python3 test_arcade.py
    docker rm -f "$NAME" >/dev/null
    ;;
  *)
    echo "Speculos is running: open http://localhost:5001 to play."
    echo "Stop it with: docker rm -f $NAME"
    ;;
esac
