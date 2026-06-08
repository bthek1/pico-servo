#!/bin/bash
set -e

MOUNT="/media/pi/RPI-RP2"
BUILD_DIR="${1:-build/main}"
UF2=$(find "$BUILD_DIR" -maxdepth 1 -name "*.uf2" | head -1)

if [ -z "$UF2" ]; then
    echo "Error: no .uf2 found in $BUILD_DIR"
    exit 1
fi

if [ ! -d "$MOUNT" ]; then
    echo "Error: $MOUNT not found — hold BOOTSEL, then plug the Pico into USB"
    exit 1
fi

echo "Flashing $UF2 -> $MOUNT"
cp "$UF2" "$MOUNT"
echo "Done."
