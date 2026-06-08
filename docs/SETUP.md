# Raspberry Pi Pico — Project Setup Guide

A step-by-step guide to setting up a Raspberry Pi Pico C/C++ development environment on Linux, from scratch through to flashing firmware.

---

## Target Board

| | |
|---|---|
| **Board** | Raspberry Pi Pico W |
| **MCU** | RP2040 (dual-core Arm Cortex-M0+) |
| **`PICO_BOARD`** | `pico_w` |
| **Wireless chip** | CYW43439 (Wi-Fi + Bluetooth) — present, networking not in use |
| **Onboard LED** | CYW43 GPIO (`CYW43_WL_GPIO_LED_PIN = 0`) — **not** GPIO 25 |

> The onboard LED requires `pico_cyw43_arch_none` and `cyw43_arch_init()`. The `lib/led` library handles this automatically. Do **not** use networking libraries (`pico_cyw43_arch_lwip_*`, lwIP, MQTT) unless explicitly needed.

---

## 1. Install Dependencies

```bash
sudo apt update
sudo apt install -y \
    cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    build-essential \
    git \
    picocom
```

| Package | Purpose |
|---|---|
| `cmake` | Build system used by the Pico SDK |
| `gcc-arm-none-eabi` | ARM cross-compiler |
| `libnewlib-arm-none-eabi` | C standard library for bare-metal ARM |
| `build-essential` | `make` and other host build tools |
| `git` | For cloning the SDK and managing submodules |
| `picocom` | Serial terminal for USB UART output |

---

## 2. Get the Pico SDK

The SDK is pulled as a git submodule for project portability.

```bash
mkdir lib
git submodule add https://github.com/raspberrypi/pico-sdk lib/pico-sdk
git submodule update --init --recursive
```

Point CMake at it in the root `CMakeLists.txt`:

```cmake
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
```

Do **not** rely on the `PICO_SDK_PATH` environment variable — hardcode the path in CMakeLists.txt.

---

## 3. Project Structure

This project uses a multi-target layout with shared libraries:

```
pico-servo/
├── CMakeLists.txt          ← root: SDK init + add_subdirectory per lib and target
├── compile.sh              ← build script
├── flash.sh                ← flash script
├── justfile                ← task runner
├── secrets.h               ← Wi-Fi credentials (gitignored)
├── secrets.h.example       ← credential template (committed)
├── lib/
│   ├── pico-sdk/           ← SDK submodule
│   ├── pico-examples/      ← reference submodule (browse only, not built)
│   ├── led/                ← shared LED library
│   ├── serial/             ← shared serial library
│   ├── servo/              ← shared servo PWM library
│   └── wifi/               ← Wi-Fi connection library (lwip, poll mode)
└── targets/
    ├── main/               ← primary program (default flash target)
    ├── blink/              ← LED blink demo
    └── sweep/              ← servo sweep demo
```

Each target produces its own `.uf2` at `build/targets/<target>/<target>.uf2`.

---

## 4. Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_servo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/led)
add_subdirectory(lib/serial)
add_subdirectory(lib/servo)
add_subdirectory(lib/wifi)
add_subdirectory(targets/main)
add_subdirectory(targets/blink)
add_subdirectory(targets/sweep)
```

`pico_sdk_init()` must be called exactly once, before any `add_subdirectory`.

---

## 5. Shared Library CMakeLists.txt

Libraries use the `INTERFACE` pattern so include paths and link deps propagate automatically to consumers:

```cmake
add_library(led INTERFACE)
target_sources(led INTERFACE led.c)
target_include_directories(led INTERFACE .)
target_link_libraries(led INTERFACE pico_stdlib)
```

---

## 6. Target CMakeLists.txt

Each target only lists the libraries it actually uses:

```cmake
add_executable(main_fw main.c)

target_link_libraries(main_fw
    led
    serial
    servo
)

pico_add_extra_outputs(main_fw)   # generates .uf2, .hex, .bin, .map
pico_enable_stdio_usb(main_fw 1)
pico_enable_stdio_uart(main_fw 0)
```

---

## 7. Minimal main.c

```c
#include "led.h"
#include "serial.h"

#define LED_PIN 25

int main() {
    serial_init();
    led_init(LED_PIN);

    serial_println("ready");

    while (true) {
        sleep_ms(1000);
    }
}
```

Use `serial_init()` instead of bare `stdio_init_all()`. Use `getchar_timeout_us(0)` for non-blocking serial reads.

---

## 8. Build

```bash
./compile.sh            # build all targets
./compile.sh main_fw    # build one target
./compile.sh --clean    # wipe build/ and rebuild
```

Or via justfile (runs on Pi over SSH):

```bash
just compile            # build all
just compile sweep      # build one target
just compile-clean      # clean build
```

A successful build produces `.uf2` files under `build/targets/`.

---

## 9. Flash to Pico

```bash
./flash.sh              # flash default target (main)
./flash.sh sweep        # flash a specific target
```

`flash.sh` attempts to auto-reboot the Pico into BOOTSEL mode via `picotool`. If that fails, hold **BOOTSEL** on the Pico and plug it into USB manually — it mounts as `RPI-RP2`.

Manual flash:

```bash
cp build/targets/main/main_fw.uf2 /media/$USER/RPI-RP2/
```

Via justfile:

```bash
just flash              # flash main
just flash sweep        # flash sweep
just deploy             # pull + compile + flash main
just deploy sweep       # pull + compile + flash sweep
```

---

## 10. Serial Monitor

```bash
picocom -b 115200 /dev/ttyACM0
# or
just monitor
```

Exit picocom with `Ctrl+A` then `Ctrl+X`.

If `/dev/ttyACM0` is not found:

```bash
ls /dev/tty*   # before and after plugging in to identify the port
```

---

## 11. VS Code IntelliSense

Install the **C/C++** and **CMake Tools** extensions, then create `.vscode/c_cpp_properties.json`:

```json
{
  "configurations": [
    {
      "name": "Pico",
      "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/lib/pico-sdk/src/boards/include"
      ],
      "defines": [
        "PICO_BOARD=pico_w"
      ],
      "compilerPath": "/usr/bin/arm-none-eabi-gcc",
      "cStandard": "c11",
      "cppStandard": "c++17",
      "intelliSenseMode": "gcc-arm"
    }
  ],
  "version": 4
}
```

And `.vscode/settings.json`:

```json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

---

## 12. Secrets (Wi-Fi credentials)

Wi-Fi credentials are stored in `secrets.h` at the repo root. This file is gitignored and must be synced to the Pi manually.

```bash
cp secrets.h.example secrets.h
# edit secrets.h with real SSID and password
just push-secrets    # copy to Pi
```

To use credentials in a target, add the repo root to its include path:

```cmake
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR})
```

Then in source:

```c
#include "secrets.h"
wifi_connect(WIFI_SSID, WIFI_PASSWORD);
```

`just deploy` and `just deploy-clean` run `push-secrets` automatically before compiling.

## 13. Adding a New Target

1. Create `targets/<name>/main.c` and `targets/<name>/CMakeLists.txt`
2. Add `add_subdirectory(targets/<name>)` to root `CMakeLists.txt`
3. Flash with `./flash.sh <name>` or `just flash <name>`

---

## Quick Reference

| Task | Command |
|---|---|
| Build all | `./compile.sh` |
| Build one target | `./compile.sh <target>` |
| Clean build | `./compile.sh --clean` |
| Flash default (main) | `./flash.sh` |
| Flash specific target | `./flash.sh <target>` |
| Deploy (pull+secrets+build+flash) | `just deploy [target]` |
| Copy secrets.h to Pi | `just push-secrets` |
| Serial monitor | `just monitor` |
| Init submodules | `git submodule update --init --recursive` |
