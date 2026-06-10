default:
    @just --list

# ── Workflow ──────────────────────────────────────────────────────────────────

# compile locally and flash to Pico via Pi (default: from secrets.h)
[group('workflow')]
deploy target='': (compile target) (flash target) _wait-tty monitor

# clean compile locally and flash to Pico via Pi
[group('workflow')]
deploy-clean target='': (compile-clean target) (flash target) _wait-tty monitor

# ── Build ─────────────────────────────────────────────────────────────────────

# compile locally (default: from secrets.h, e.g: just compile sweep)
[group('build')]
compile target='':
    ./compile.sh {{target}}

# clean then compile locally
[group('build')]
compile-clean target='':
    ./compile.sh --clean {{target}}

# ── Device ────────────────────────────────────────────────────────────────────

# flash a target to the Pico via the Pi (default: from secrets.h)
[group('device')]
flash target='':
    ./flash.sh --remote {{target}}

# open serial monitor
[group('device')]
monitor:
    ssh pi "picocom -b 115200 /dev/ttyACM0"

[private]
_wait-tty:
    @until ssh pi "test -e /dev/ttyACM0"; do sleep 1; done
