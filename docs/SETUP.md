# Raspberry Pi Pico — Project Setup Guide

A step-by-step guide to setting up a Raspberry Pi Pico C/C++ development environment on Linux, from scratch through to flashing firmware.

---

## Target Board

| | |
|---|---|
| **Board** | Raspberry Pi Pico (standard) |
| **MCU** | RP2040 (dual-core Arm Cortex-M0+) |
| **`PICO_BOARD`** | `pico` |
| **Wi-Fi / Bluetooth** | None — this is **not** the Pico W |

> Do **not** use `PICO_BOARD=pico_w`, `cyw43`, `lwIP`, or any wireless libraries. The standard Pico has no CYW43 chip.

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

The SDK can be pulled as a git submodule (recommended for project portability) or installed system-wide.

### Option A — Git submodule (recommended)

```bash
# Inside your project root
mkdir lib
git submodule add https://github.com/raspberrypi/pico-sdk lib/pico-sdk
git submodule update --init --recursive
```

Then point CMake at it in your root `CMakeLists.txt`:

```cmake
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
```

### Option B — System-wide

```bash
git clone https://github.com/raspberrypi/pico-sdk ~/pico-sdk
cd ~/pico-sdk && git submodule update --init
export PICO_SDK_PATH=~/pico-sdk   # add to ~/.bashrc to persist
```

Then in `CMakeLists.txt`:

```cmake
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
```

---

## 3. Project Structure

A minimal project looks like this:

```
my-project/
├── CMakeLists.txt        # Root CMake config
├── main/
│   ├── CMakeLists.txt    # Target-level CMake config
│   └── main.c            # Application source
└── lib/
    └── pico-sdk/         # SDK submodule
```

---

## 4. Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(my_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
add_subdirectory(main)
```

---

## 5. Target CMakeLists.txt

```cmake
add_executable(my_project main.c)

target_link_libraries(my_project
    pico_stdlib
)

pico_add_extra_outputs(my_project)  # generates .uf2, .hex, .bin, .map

# Enable USB serial, disable UART serial
pico_enable_stdio_usb(my_project 1)
pico_enable_stdio_uart(my_project 0)
```

---

## 6. Minimal main.c

```c
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100); // wait for USB serial

    printf("Hello from Pico!\n");

    while (true) {
        sleep_ms(1000);
    }
}
```

---

## 7. Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Or using the convenience scripts in this repo:

```bash
./compile.sh          # normal build
./compile.sh --clean  # wipe build/ and rebuild
```

A successful build produces `build/main/my_project.uf2`.

---

## 8. Flash to Pico

1. Hold **BOOTSEL** on the Pico and plug it into USB.
2. It mounts as a drive named `RPI-RP2`.
3. Copy the `.uf2` file to the drive:

```bash
cp build/main/my_project.uf2 /media/$USER/RPI-RP2/
```

Or use the `flash.sh` script in this repo:

```bash
./flash.sh              # flashes build/main/ (default)
./flash.sh path/to/dir  # flashes a specific build subdirectory
```

The Pico reboots automatically and starts running the firmware.

---

## 9. Serial Monitor

USB serial output is readable via any serial terminal:

```bash
picocom -b 115200 /dev/ttyACM0
```

Exit picocom with `Ctrl+A` then `Ctrl+X`.

If `/dev/ttyACM0` is not found, try `/dev/ttyUSB0` or check:

```bash
ls /dev/tty*   # before and after plugging in to identify the port
```

---

## 10. VS Code IntelliSense

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
        "PICO_BOARD=pico"
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

And `.vscode/settings.json` to wire up CMake Tools:

```json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

---

## 11. Adding pico-examples (optional)

The official examples are useful as reference or to flash directly:

```bash
git submodule add https://github.com/raspberrypi/pico-examples lib/pico-examples
git submodule update --init --recursive
```

Add to root `CMakeLists.txt`:

```cmake
add_subdirectory(lib/pico-examples)
```

Then build and flash any example:

```bash
make -j$(nproc)
./flash.sh lib/pico-examples/blink
./flash.sh lib/pico-examples/gpio/hello_gpio
```

---

## Quick Reference

| Task | Command |
|---|---|
| Build | `./compile.sh` |
| Clean build | `./compile.sh --clean` |
| Flash default | `./flash.sh` |
| Flash example | `./flash.sh lib/pico-examples/blink` |
| Serial monitor | `make terminal` |
| Init submodules | `git submodule update --init --recursive` |
