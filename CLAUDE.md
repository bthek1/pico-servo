# pico-servo — Claude Code Project Context

## Project

Servo motor control firmware for the **Raspberry Pi Pico** (standard RP2040), written in C/C++ with the official Pico SDK.

## Board

- **Board**: Raspberry Pi Pico (standard) — `PICO_BOARD=pico`
- **MCU**: RP2040, dual-core Arm Cortex-M0+, 264 KB RAM, 2 MB flash
- **No Wi-Fi / Bluetooth** — do **not** suggest `cyw43`, `lwIP`, `pico_cyw43_arch_*`, or any wireless APIs

## Host Machine

The Pico is connected to a **Raspberry Pi** (the host/build machine).

- SSH access: `ssh pi`
- All build, flash, and serial commands run **on the Raspberry Pi over SSH**
- Flash mount point: `/media/pi/RPI-RP2/`

When giving instructions that involve the terminal, assume the user is either already SSH'd in or needs to run `ssh pi` first.

## Build & Flash

```bash
# Run on the Raspberry Pi (ssh pi)
./compile.sh            # build
./compile.sh --clean    # clean build
./flash.sh              # flash (Pico must be in BOOTSEL mode)
picocom -b 115200 /dev/ttyACM0   # serial monitor
```

Build output is in `build/` (gitignored). The flashable binary is `build/main/pico_servo.uf2`.

Manual flash (if `flash.sh` is unavailable):

```bash
cp build/main/pico_servo.uf2 /media/pi/RPI-RP2/
```

## SDK & CMake Conventions

- SDK lives at `lib/pico-sdk` (git submodule)
- `PICO_SDK_PATH` is set in `CMakeLists.txt`; do **not** rely on env vars
- Always call `pico_sdk_init()` after setting board variables
- `pico_add_extra_outputs(target)` is required to produce `.uf2`
- Stdio is routed via USB (`pico_enable_stdio_usb 1`); UART stdio is off

## Servo PWM

Standard hobby servos need 50 Hz PWM with 1–2 ms pulse widths:

```c
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// 125 MHz / (100 * 25000) = 50 Hz
pwm_set_clkdiv(slice, 100.0f);
pwm_set_wrap(slice, 24999);
// level = pulse_us * 25000 / 20000  (e.g. 1500 µs → 1875)
pwm_set_gpio_level(gpio, level);
pwm_set_enabled(slice, true);
```

Link `hardware_pwm` in `target_link_libraries`.

## What to Avoid

- `pico/cyw43_arch.h`, `cyw43_*` APIs, `pico_cyw43_arch_*` link libraries
- `PICO_BOARD=pico_w` or `PICO_CYW43_SUPPORTED`
- Wi-Fi, Bluetooth, MQTT, or lwIP anything
- Busy-wait loops — use `sleep_ms()` / `sleep_us()` or absolute-time alarms
