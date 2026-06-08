#!/bin/bash
set -e

BUILD_DIR="build"
TARGET=""

for arg in "$@"; do
    case "$arg" in
        --clean) rm -rf "$BUILD_DIR" ;;
        *)       TARGET="$arg" ;;
    esac
done

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DPICO_BOARD=pico

if [ -n "$TARGET" ]; then
    make -j$(nproc) "$TARGET"
else
    make -j$(nproc)
fi
