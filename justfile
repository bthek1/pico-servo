default:
    @just --list

# ── Workflow ──────────────────────────────────────────────────────────────────

# pull, compile, and flash a target (default: blink)
[group('workflow')]
deploy target='blink': pull (compile target) (flash target)

# ── Git ───────────────────────────────────────────────────────────────────────

# git pull on the Pi
[group('git')]
pull:
    ssh pi "cd ~/Documents/pico/pico-servo && git pull"

# ── Build ─────────────────────────────────────────────────────────────────────

# compile on the Pi (optional target, e.g: just compile sweep)
[group('build')]
compile target='':
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh {{target}}"

# clean then compile on the Pi
[group('build')]
compile-clean target='':
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh --clean {{target}}"

# ── Device ────────────────────────────────────────────────────────────────────

# flash a target (default: blink)
[group('device')]
flash target='blink':
    ssh pi "cd ~/Documents/pico/pico-servo && ./flash.sh {{target}}"

# open serial monitor
[group('device')]
monitor:
    ssh pi "picocom -b 115200 /dev/ttyACM0"
