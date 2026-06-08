#!/bin/bash
set -e

BUILD_DIR="build"
TARGET=""
SECRETS="${BASH_SOURCE[0]%/*}/secrets.h"

for arg in "$@"; do
    case "$arg" in
        --clean) rm -rf "$BUILD_DIR" ;;
        *)       TARGET="$arg" ;;
    esac
done

if [ -z "$TARGET" ] && [ -f "$SECRETS" ]; then
    TARGET=$(grep -oP '(?<=#define DEFAULT_TARGET\s{1,20}")[^"]+' "$SECRETS")
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DPICO_BOARD=pico_w

if [ -n "$TARGET" ]; then
    make -j$(nproc) "$TARGET"
else
    make -j$(nproc)
fi
