default:
    @just --list

# ── Workflow ──────────────────────────────────────────────────────────────────

# pull, compile, and flash a target (default: main)
[group('workflow')]
deploy target='main': pull (compile target) (flash target)

# pull, clean compile, and flash a target (default: main)
[group('workflow')]
deploy-clean target='main': pull (compile-clean target) (flash target)

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
flash target='main':
    ssh pi "cd ~/Documents/pico/pico-servo && ./flash.sh {{target}}"

# open serial monitor
[group('device')]
monitor:
    ssh pi "picocom -b 115200 /dev/ttyACM0"
