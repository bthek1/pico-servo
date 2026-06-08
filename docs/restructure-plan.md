# Project Restructure Plan

Current structure has a single `main/` target wired directly to the root `CMakeLists.txt`.
Goal: support multiple independent targets, a shared library, and reference examples.

---

## Target layout

```
pico-servo/
├── CMakeLists.txt              ← root: SDK init + add_subdirectory per target
├── compile.sh                  ← updated: accepts optional TARGET arg
├── flash.sh                    ← updated: accepts optional TARGET arg
├── justfile                    ← updated: flash/deploy accept target param
├── lib/
│   ├── pico-sdk/               ← existing submodule (unchanged)
│   ├── pico-examples/          ← new submodule (reference only, not built)
│   └── servo/                  ← new shared library
│       ├── CMakeLists.txt
│       ├── servo.h
│       └── servo.c
└── targets/
    ├── blink/                  ← existing main/ renamed + moved here
    │   ├── CMakeLists.txt
    │   └── main.c
    └── sweep/                  ← new: continuous servo sweep demo
        ├── CMakeLists.txt
        └── main.c
```

---

## Steps

### 1 — Move existing target

Rename `main/` → `targets/blink/` to fit the new layout.

```bash
mkdir -p targets/blink
git mv main/main.c targets/blink/main.c
git mv main/CMakeLists.txt targets/blink/CMakeLists.txt
git rm -r main/
```

Update `targets/blink/CMakeLists.txt` — rename executable and link servo lib:

```cmake
add_executable(blink main.c)
target_link_libraries(blink pico_stdlib servo)
pico_add_extra_outputs(blink)
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)
```

### 2 — Create shared servo library

`lib/servo/CMakeLists.txt`:

```cmake
add_library(servo INTERFACE)
target_sources(servo INTERFACE servo.c)
target_include_directories(servo INTERFACE .)
target_link_libraries(servo INTERFACE
    pico_stdlib
    hardware_pwm
    hardware_clocks
)
```

`lib/servo/servo.h` — public API:

```c
#pragma once
#include <stdint.h>

void servo_init(uint gpio);
void servo_set_us(uint gpio, uint16_t pulse_us);   // 1000–2000
void servo_set_deg(uint gpio, float degrees);       // 0–180
```

`lib/servo/servo.c` — implementation using the PWM formula from CLAUDE.md.

### 3 — Update root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_servo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/servo)
add_subdirectory(targets/blink)
add_subdirectory(targets/sweep)
```

> `lib/pico-examples` is **not** added here — it's a reference, browsed locally, not compiled as part of this project.

### 4 — Add sweep target

`targets/sweep/CMakeLists.txt`:

```cmake
add_executable(sweep main.c)
target_link_libraries(sweep servo)
pico_add_extra_outputs(sweep)
pico_enable_stdio_usb(sweep 1)
pico_enable_stdio_uart(sweep 0)
```

`targets/sweep/main.c` — sweeps servo 0°→180°→0° continuously using `servo_set_deg()`.

### 5 — Update compile.sh

Accept an optional `TARGET` to build only one target, or build all if omitted:

```bash
TARGET=${1:-}   # e.g. ./compile.sh sweep

cmake .. -DPICO_BOARD=pico
[ -n "$TARGET" ] && make -j$(nproc) "$TARGET" || make -j$(nproc)
```

### 6 — Update flash.sh

Resolve the `.uf2` from the target name:

```bash
TARGET=${1:-blink}
BUILD_DIR="build/targets/$TARGET"
UF2=$(find "$BUILD_DIR" -maxdepth 1 -name "*.uf2" | head -1)
```

### 7 — Update justfile

```just
# compile on the Pi (optional target, e.g: just compile sweep)
[group('build')]
compile target='':
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh {{target}}"

# flash a specific target (default: blink)
[group('device')]
flash target='blink':
    ssh pi "cd ~/Documents/pico/pico-servo && ./flash.sh {{target}}"

# pull, compile, and flash a target
[group('workflow')]
deploy target='blink': pull (compile target) (flash target)
```

---

## Build output paths

| Target  | UF2 location                        |
|---------|-------------------------------------|
| blink   | `build/targets/blink/blink.uf2`     |
| sweep   | `build/targets/sweep/sweep.uf2`     |

---

## Rules going forward

- Every new program → new subdirectory under `targets/`
- Shared peripheral code (PWM, I2C, SPI helpers) → library under `lib/`
- Each target's `CMakeLists.txt` only lists deps that target actually uses
- `pico_sdk_init()` called once in root `CMakeLists.txt` only
- `lib/pico-examples` is browse-only; copy snippets into `targets/` or `lib/` as needed
