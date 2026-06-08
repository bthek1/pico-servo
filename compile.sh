#!/bin/bash
set -e

BUILD_DIR="build"

if [ "$1" = "--clean" ]; then
    echo "Cleaning build..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DPICO_BOARD=pico
make -j$(nproc)
