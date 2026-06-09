#!/bin/bash
set -e

SECRETS="${BASH_SOURCE[0]%/*}/secrets.h"
_DEFAULT=$([ -f "$SECRETS" ] && sed -n 's/^#define DEFAULT_TARGET[[:space:]]*"\([^"]*\)".*/\1/p' "$SECRETS" || echo "main")
TARGET="${1:-$_DEFAULT}"
MOUNT="${PICO_MOUNT:-/media/$(whoami)/RPI-RP2}"
BUILD_DIR="build/targets/$TARGET"
UF2=$(find "$BUILD_DIR" -maxdepth 1 -name "*.uf2" 2>/dev/null | head -1)

if [ -z "$UF2" ]; then
    echo "Error: no .uf2 found in $BUILD_DIR"
    exit 1
fi

# If Pico is running firmware, reboot it into BOOTSEL via USB
if [ ! -d "$MOUNT" ]; then
    echo "Rebooting Pico into BOOTSEL mode..."
    # 1200-baud magic: Pico SDK USB CDC reboots to BOOTSEL on 1200-baud open
    if [ -e /dev/ttyACM0 ]; then
        stty -F /dev/ttyACM0 1200 2>/dev/null || true
    fi
    # Also try picotool in case device isn't on ttyACM0
    if command -v picotool &>/dev/null; then
        picotool reboot -f -u 2>/dev/null || true
    fi
    for i in $(seq 1 10); do
        sleep 1
        [ -d "$MOUNT" ] && break
    done
fi

if [ ! -d "$MOUNT" ]; then
    echo "Error: $MOUNT not found — hold BOOTSEL, then plug the Pico into USB"
    exit 1
fi

echo "Flashing $UF2 -> $MOUNT"
cp "$UF2" "$MOUNT"
echo "Done."
