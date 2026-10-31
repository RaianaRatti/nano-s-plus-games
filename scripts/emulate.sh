#!/usr/bin/env bash
# Runs a built app in Speculos, Ledger's Nano S Plus emulator: Badge Arcade, or with "pet" or
# "gallery" first, the pet or the gallery. No device needed.
#
#   ./scripts/emulate.sh [app]        open http://localhost:5001 in a browser and play
#                                     (the page has the screen and the two buttons; keyboard: arrows)
#   ./scripts/emulate.sh [app] tour   click through every screen and save screenshots/
#   ./scripts/emulate.sh [app] test   run the automated tests
set -euo pipefail
cd "$(dirname "$0")/.."

IMAGE=ghcr.io/ledgerhq/ledger-app-builder/ledger-app-dev-tools:latest
NAME=arcade-speculos

APP_DIR=.
TESTS=test_arcade.py
TOUR=tour.py
BUILD_ARGS=()
case "${1:-}" in
  pet | gallery)
    APP_DIR=$1
    TESTS="test_$1.py"
    TOUR="tour_$1.py"
    BUILD_ARGS=("$1")
    shift
    ;;
esac

[ -f "$APP_DIR/bin/app.elf" ] || ./scripts/build.sh "${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"}"

docker rm -f "$NAME" >/dev/null 2>&1 || true
docker run -d --name "$NAME" -v "$PWD:/app" -p 5001:5000 "$IMAGE" \
  speculos --model nanosp "/app/$APP_DIR/bin/app.elf" --display headless --api-port 5000 >/dev/null

for _ in $(seq 1 30); do
  curl -s localhost:5001/events >/dev/null 2>&1 && break
  sleep 1
done

case "${1:-}" in
  tour)
    docker exec -w /app/tests "$NAME" python3 "$TOUR"
    docker rm -f "$NAME" >/dev/null
    ;;
  test)
    status=0
    docker exec -w /app/tests "$NAME" python3 "$TESTS" || status=$?
    if [ "$APP_DIR" != . ] && [ "$status" != 1 ]; then
      # The pet's and the gallery's tests end by quitting the app, which stops the emulator (and
      # the tests with it).
      sleep 1
      if [ "$(docker inspect -f '{{.State.Running}}' "$NAME" 2>/dev/null)" = false ]; then
        echo "PASS HOLD quits the app"
        status=0
      else
        echo "FAIL HOLD did not quit the app"
        status=1
      fi
    fi
    docker rm -f "$NAME" >/dev/null
    exit "$status"
    ;;
  *)
    echo "Speculos is running: open http://localhost:5001 to play."
    echo "Stop it with: docker rm -f $NAME"
    ;;
esac
