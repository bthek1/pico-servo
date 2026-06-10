#!/bin/bash
set -e

# ── colours ────────────────────────────────────────────────────────────────
BOLD=$'\e[1m'
DIM=$'\e[2m'
RESET=$'\e[0m'
CYAN=$'\e[1;36m'
GREEN=$'\e[1;32m'
YELLOW=$'\e[1;33m'
RED=$'\e[1;31m'
BLUE=$'\e[1;34m'
GRAY=$'\e[0;37m'

step()    { printf "\n${CYAN}  ──  ${BOLD}%s${RESET}\n" "$*"; }
ok()      { printf "${GREEN}  ✓  ${RESET}%s\n" "$*"; }
info()    { printf "${BLUE}  ·  ${RESET}${GRAY}%s${RESET}\n" "$*"; }
warn()    { printf "${YELLOW}  ⚠  ${RESET}%s\n" "$*"; }
die()     { printf "\n${RED}  ✗  ERROR: %s${RESET}\n\n" "$*" >&2; exit 1; }
rule()    { printf "${DIM}  %s${RESET}\n" "$(printf '─%.0s' {1..54})"; }

# ── parse args ─────────────────────────────────────────────────────────────
BUILD_DIR="build"
TARGET=""
CLEAN=0
SECRETS="${BASH_SOURCE[0]%/*}/secrets.h"

for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=1 ;;
        *)       TARGET="$arg" ;;
    esac
done

if [ -z "$TARGET" ] && [ -f "$SECRETS" ]; then
    TARGET=$(sed -n 's/^#define DEFAULT_TARGET[[:space:]]*"\([^"]*\)".*/\1/p' "$SECRETS")
fi

LABEL="${TARGET:-all targets}"

# ── header ─────────────────────────────────────────────────────────────────
echo
printf "${CYAN}${BOLD}  PICO BUILD${RESET}  ${GRAY}→  ${RESET}${BOLD}%s${RESET}\n" "$LABEL"
rule

# ── clean ──────────────────────────────────────────────────────────────────
if [ "$CLEAN" -eq 1 ]; then
    step "Clean"
    rm -rf "$BUILD_DIR"
    ok "Removed $BUILD_DIR/"
fi

# ── cmake ──────────────────────────────────────────────────────────────────
step "CMake configure"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

CMAKE_START=$SECONDS
cmake .. -DPICO_BOARD=pico_w 2>&1 \
    | sed "s/^/${GRAY}       /" \
    | sed "s/$/${RESET}/"
ok "Configured  ${GRAY}($((SECONDS - CMAKE_START)) s)${RESET}"

# ── make ───────────────────────────────────────────────────────────────────
JOBS=$(nproc)
step "Make  ${GRAY}(-j${JOBS})${RESET}"

MAKE_START=$SECONDS
if [ -n "$TARGET" ]; then
    make -j"$JOBS" "$TARGET" 2>&1 \
        | grep -v "^$" \
        | sed "s/^\(\[.*\]\)/${GRAY}\1${RESET}/" \
        | sed "s/^[^[]/${GRAY}     &${RESET}/"
else
    make -j"$JOBS" 2>&1 \
        | grep -v "^$" \
        | sed "s/^\(\[.*\]\)/${GRAY}\1${RESET}/" \
        | sed "s/^[^[]/${GRAY}     &${RESET}/"
fi
ELAPSED=$((SECONDS - MAKE_START))
ok "Build complete  ${GRAY}(${ELAPSED} s)${RESET}"

# ── artefacts ──────────────────────────────────────────────────────────────
step "Artefacts"
cd ..
if [ -n "$TARGET" ]; then
    UF2=$(find "build/targets/$TARGET" -maxdepth 1 -name "*.uf2" 2>/dev/null | head -1)
    if [ -n "$UF2" ]; then
        SIZE=$(du -h "$UF2" | cut -f1)
        info "${UF2}  ${YELLOW}(${SIZE})${RESET}"
    else
        warn "No .uf2 found for $TARGET"
    fi
else
    while IFS= read -r uf2; do
        SIZE=$(du -h "$uf2" | cut -f1)
        info "${uf2}  ${YELLOW}(${SIZE})${RESET}"
    done < <(find build/targets -maxdepth 2 -name "*.uf2" 2>/dev/null | sort)
fi

# ── done ───────────────────────────────────────────────────────────────────
rule
printf "${GREEN}${BOLD}  Done.${RESET}\n\n"
