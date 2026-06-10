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
SECRETS="${BASH_SOURCE[0]%/*}/secrets.h"
_DEFAULT=$([ -f "$SECRETS" ] && sed -n 's/^#define DEFAULT_TARGET[[:space:]]*"\([^"]*\)".*/\1/p' "$SECRETS" || echo "website")
TARGET=""
REMOTE=0
PI_HOST="${PICO_PI_HOST:-pi}"
PI_MOUNT="${PICO_PI_MOUNT:-/media/bthek1/RPI-RP2}"
PI_FLASH_DIR="~/pico-flash"

for arg in "$@"; do
    case "$arg" in
        --remote) REMOTE=1 ;;
        *)        TARGET="$arg" ;;
    esac
done

TARGET="${TARGET:-$_DEFAULT}"
BUILD_DIR="build/targets/$TARGET"
UF2=$(find "$BUILD_DIR" -maxdepth 1 -name "*.uf2" 2>/dev/null | head -1)

MODE="${REMOTE:+remote → $PI_HOST}"
MODE="${MODE:-local}"

# ── header ─────────────────────────────────────────────────────────────────
echo
printf "${YELLOW}${BOLD}  PICO FLASH${RESET}  ${GRAY}→  ${RESET}${BOLD}%s${RESET}  ${GRAY}[%s]${RESET}\n" "$TARGET" "$MODE"
rule

# ── preflight ──────────────────────────────────────────────────────────────
step "Preflight"
[ -n "$UF2" ] || die "no .uf2 found in $BUILD_DIR — run compile.sh first"

SIZE=$(du -h "$UF2" | cut -f1)
info "UF2:  $UF2  ${YELLOW}(${SIZE})${RESET}"

# ── remote flash ───────────────────────────────────────────────────────────
if [ "$REMOTE" -eq 1 ]; then
    step "Upload  ${GRAY}→  ${PI_HOST}${RESET}"
    ssh "$PI_HOST" "mkdir -p $PI_FLASH_DIR"
    scp "$UF2" "$PI_HOST:$PI_FLASH_DIR/${TARGET}.uf2" \
        | sed "s/^/${GRAY}       /" \
        | sed "s/$/${RESET}/"
    ok "Uploaded  ${GRAY}${TARGET}.uf2${RESET}"

    step "Flash on ${PI_HOST}"
    ssh "$PI_HOST" "
        CYAN='\e[1;36m'; GREEN='\e[1;32m'; YELLOW='\e[1;33m'; RED='\e[1;31m'; GRAY='\e[0;37m'; RESET='\e[0m'; BOLD='\e[1m'
        info()  { printf \"  ${GRAY}·  %s${RESET}\n\" \"\$*\"; }
        ok()    { printf \"  ${GREEN}✓  ${RESET}%s\n\" \"\$*\"; }
        warn()  { printf \"  ${YELLOW}⚠  ${RESET}%s\n\" \"\$*\"; }
        die()   { printf \"  ${RED}✗  ERROR: %s${RESET}\n\" \"\$*\" >&2; exit 1; }

        MOUNT='$PI_MOUNT'
        if [ ! -d \"\$MOUNT\" ] && command -v picotool &>/dev/null; then
            info 'Rebooting Pico into BOOTSEL mode...'
            picotool reboot -f -u 2>/dev/null || true
            for i in 1 2 3 4 5; do
                sleep 1
                [ -d \"\$MOUNT\" ] && break
                info 'Waiting for mount... ('\$i'/5)'
            done
        fi
        [ -d \"\$MOUNT\" ] || die \"\$MOUNT not found — hold BOOTSEL then plug in\"
        info 'Copying ${TARGET}.uf2 → '\"\$MOUNT\"
        cp \$HOME/pico-flash/${TARGET}.uf2 \"\$MOUNT/\"
        ok 'Flashed — Pico rebooting'
    "

# ── local flash ────────────────────────────────────────────────────────────
else
    MOUNT="${PICO_MOUNT:-/media/$(whoami)/RPI-RP2}"
    step "Mount  ${GRAY}${MOUNT}${RESET}"

    if [ ! -d "$MOUNT" ]; then
        if command -v picotool &>/dev/null; then
            info "Rebooting Pico into BOOTSEL mode..."
            picotool reboot -f -u 2>/dev/null || true
            for i in $(seq 1 5); do
                sleep 1
                [ -d "$MOUNT" ] && break
                info "Waiting for mount... ($i/5)"
            done
        fi
    fi

    [ -d "$MOUNT" ] || die "$MOUNT not found — hold BOOTSEL then plug in"
    ok "Mount ready  ${GRAY}${MOUNT}${RESET}"

    step "Flash"
    info "Copying $(basename "$UF2") → $MOUNT"
    cp "$UF2" "$MOUNT"
    ok "Flashed — Pico rebooting"
fi

# ── done ───────────────────────────────────────────────────────────────────
rule
printf "${GREEN}${BOLD}  Done.${RESET}\n\n"
