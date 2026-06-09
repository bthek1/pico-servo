default:
    @just --list

# ── Workflow ──────────────────────────────────────────────────────────────────

# pull, compile, and flash a target (default: from secrets.h)
[group('workflow')]
deploy target='': pull push-secrets (compile target) (flash target) && just monitor

# pull, clean compile, and flash a target (default: from secrets.h)
[group('workflow')]
deploy-clean target='': pull push-secrets (compile-clean target) (flash target) && just monitor

# ── Secrets ───────────────────────────────────────────────────────────────────

# copy secrets.h to the Pi (never committed, must be synced manually)
[group('secrets')]
push-secrets:
    scp secrets.h pi:~/Documents/pico/pico-servo/secrets.h

# ── Git ───────────────────────────────────────────────────────────────────────

# force pull on the Pi, discarding any local changes
[group('git')]
pull:
    ssh pi "cd ~/Documents/pico/pico-servo && git fetch origin && git reset --hard origin/main"

# ── Build ─────────────────────────────────────────────────────────────────────

# compile on the Pi (default: from secrets.h, e.g: just compile sweep)
[group('build')]
compile target='':
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh {{target}}"

# clean then compile on the Pi (default: from secrets.h)
[group('build')]
compile-clean target='':
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh --clean {{target}}"

# ── Device ────────────────────────────────────────────────────────────────────

# flash a target (default: from secrets.h)
[group('device')]
flash target='':
    ssh pi "cd ~/Documents/pico/pico-servo && ./flash.sh {{target}}"

# open serial monitor
[group('device')]
monitor:
    ssh pi "picocom -b 115200 /dev/ttyACM0"
