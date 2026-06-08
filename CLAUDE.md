# pico-servo — Claude Code Project Context

## Project

Servo motor control firmware for the **Raspberry Pi Pico** (standard RP2040), written in C/C++ with the official Pico SDK.

## Board

- **Board**: Raspberry Pi Pico W — `PICO_BOARD=pico_w`
- **MCU**: RP2040, dual-core Arm Cortex-M0+, 264 KB RAM, 2 MB flash
- **Wireless chip**: CYW43439 (Wi-Fi + Bluetooth) — present but networking is **not** in use
- **Onboard LED**: wired to CYW43 chip GPIO (`CYW43_WL_GPIO_LED_PIN = 0`), **not** GPIO 25

## Host Machine

The Pico is connected to a **Raspberry Pi** (the host/build machine).

- SSH access: `ssh pi` (username: `bthek1`)
- Repo path on Pi: `~/Documents/pico/pico-servo`
- All build, flash, and serial commands run **on the Raspberry Pi over SSH**
- Flash mount point: `/media/bthek1/RPI-RP2/`

When giving instructions that involve the terminal, assume the user is either already SSH'd in or needs to run `ssh pi` first.

## Project Structure

```
pico-servo/
├── CMakeLists.txt          ← root: SDK init + add_subdirectory for each lib and target
├── compile.sh              ← build all, or a single target: ./compile.sh [target]
├── flash.sh                ← flash a target (default: main): ./flash.sh [target]
├── justfile                ← just commands: deploy, compile, flash, monitor
├── docs/
│   ├── SETUP.md            ← environment setup guide
│   └── plans/              ← active design/migration plans
│       └── completed/      ← plans moved here once fully implemented
├── lib/
│   ├── pico-sdk/           ← SDK submodule
│   ├── pico-examples/      ← reference submodule (not built, browse only)
│   ├── led/                ← GPIO LED library
│   ├── serial/             ← USB serial library
│   └── servo/              ← servo PWM library
└── targets/
    ├── main/               ← primary target (default for flash/deploy)
    ├── blink/              ← LED blink demo
    └── sweep/              ← servo sweep demo
```

## Build & Flash

```bash
# Run on the Raspberry Pi (ssh pi)
./compile.sh              # build all targets
./compile.sh sweep        # build one target
./compile.sh --clean      # clean build (all)
./flash.sh                # flash default target (main)
./flash.sh sweep          # flash a specific target
picocom -b 115200 /dev/ttyACM0   # serial monitor
```

Or via justfile (run locally, SSH to Pi automatically):

```bash
just deploy           # pull + compile + flash main
just deploy sweep     # pull + compile + flash sweep
just compile sweep    # compile one target on Pi
just flash sweep      # flash one target on Pi
just monitor          # open serial monitor on Pi
```

Build output is in `build/` (gitignored). UF2 files land at `build/targets/<target>/<target>.uf2`.

The default target is **`main`** (`build/targets/main/main.uf2`).

Manual flash:

```bash
cp build/targets/main/main_fw.uf2 /media/bthek1/RPI-RP2/
```

## SDK & CMake Conventions

- SDK lives at `lib/pico-sdk` (git submodule)
- `PICO_SDK_PATH` is set in `CMakeLists.txt`; do **not** rely on env vars
- `pico_sdk_init()` is called once in the root `CMakeLists.txt` only
- `pico_add_extra_outputs(target)` is required to produce `.uf2`
- Stdio is routed via USB (`pico_enable_stdio_usb 1`); UART stdio is off
- Each new program goes in its own subdirectory under `targets/`
- Shared peripheral code goes in a library under `lib/`
- Each target's `CMakeLists.txt` only lists deps that target actually uses

## Shared Libraries

### `lib/led` — LED (CYW43-aware)

```c
#include "led.h"

void led_init(uint gpio);
void led_on(uint gpio);
void led_off(uint gpio);
void led_toggle(uint gpio);
```

Link: `led` (pulls in `pico_stdlib` + `pico_cyw43_arch_none`)

On Pico W the `gpio` argument is ignored — the onboard LED is always `CYW43_WL_GPIO_LED_PIN`.
Pass any value (e.g. `25`) and it compiles cleanly. `led_init` calls `cyw43_arch_init()` internally.

Note: `PICO_CYW43_SUPPORTED` is a CMake variable only — it is **not** a C preprocessor define.
Do not use `#ifdef PICO_CYW43_SUPPORTED` in C source; the CYW43 path in `lib/led` is unconditional.

### `lib/serial` — USB Serial

```c
#include "serial.h"

void serial_init(void);                       // replaces stdio_init_all()
void serial_print(const char *fmt, ...);
void serial_println(const char *fmt, ...);
```

Link: `serial` (pulls in `pico_stdlib`)

Use `getchar_timeout_us(0)` for non-blocking read; returns `PICO_ERROR_TIMEOUT` if no char available.

### `lib/servo` — Servo PWM

```c
#include "servo.h"

void servo_init(uint gpio);
void servo_set_us(uint gpio, uint16_t pulse_us);   // 1000–2000 µs
void servo_set_deg(uint gpio, float degrees);       // 0–180°
```

Link: `servo` (pulls in `pico_stdlib`, `hardware_pwm`, `hardware_clocks`)

PWM: 125 MHz / (100 × 25000) = 50 Hz. Level = pulse_us × 25000 / 20000.

## Timing

Prefer `absolute_time_t` over `sleep_ms` when multiple independent intervals are needed:

```c
absolute_time_t next = make_timeout_time_ms(500);
if (time_reached(next)) {
    // do work
    next = make_timeout_time_ms(500);
}
```

This keeps the loop non-blocking so multiple tasks (blink, serial report, echo) can run concurrently.

## What to Avoid

- `pico_cyw43_arch_lwip_*` link libraries — networking is not in use; use `pico_cyw43_arch_none` only
- Wi-Fi, Bluetooth, MQTT, lwIP, or TCP/IP anything — unless explicitly requested
- `PICO_BOARD=pico` — board is Pico W; always use `pico_w`
- Busy-wait loops — use `sleep_ms()` / `sleep_us()` or `absolute_time_t` alarms
- Calling `pico_sdk_init()` more than once
- Calling `stdio_init_all()` directly — use `serial_init()` instead
- Calling `cyw43_arch_init()` directly — `led_init()` handles it
- Adding `lib/pico-examples` to CMake — it is browse-only reference, not built
