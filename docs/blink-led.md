# Plan: Blink the Onboard LED

**Board**: Raspberry Pi Pico (standard) — GPIO 25  
**SDK**: pico-sdk  
**Language**: C

---

## How the Onboard LED Works

On the standard Pico, the onboard LED is wired directly to **GPIO 25** on the RP2040. It is controlled like any other GPIO output — no extra libraries needed beyond `pico_stdlib`.

---

## Changes Required

### 1. No new CMake dependencies

`pico_stdlib` (already linked) includes `hardware_gpio`. Nothing extra needed in `main/CMakeLists.txt`.

### 2. `main.c` — blink loop

```c
#include "pico/stdlib.h"

#define LED_PIN 25
#define BLINK_MS 500

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(BLINK_MS);
        gpio_put(LED_PIN, 0);
        sleep_ms(BLINK_MS);
    }
}
```

---

## Step-by-Step

- [x] **Write firmware** — `main/main.c` toggles GPIO 25 at 1 Hz via `gpio_put` + `sleep_ms`
- [x] **CMake wired up** — root and target `CMakeLists.txt` created; `pico_stdlib` linked
- [x] **Scripts created** — `compile.sh` and `flash.sh` present and executable
- [x] **Pushed to GitHub** — `git@github.com:bthek1/pico-servo.git`
- [x] **Toolchain verified on Pi** — `arm-none-eabi-gcc 12.2.1`, `cmake 3.25.1`
- [x] **Repo cloned on Pi** — `~/Documents/pico/pico-servo`
- [ ] **Init pico-sdk submodule** — `lib/` missing; submodule pull did not complete
- [ ] **Build** — `./compile.sh`
- [ ] **Flash** — hold BOOTSEL, plug Pico into Pi USB, run `./flash.sh`
- [ ] **Verify** — LED blinks at 1 Hz

### Resume from here

```bash
ssh pi
cd ~/Documents/pico/pico-servo
git submodule update --init --recursive   # pulls pico-sdk (~50 MB)
./compile.sh
# hold BOOTSEL, plug Pico in
./flash.sh
```

To monitor serial output:
```bash
picocom -b 115200 /dev/ttyACM0
```

---

## Using `PICO_DEFAULT_LED_PIN` (optional)

The SDK defines `PICO_DEFAULT_LED_PIN` for the board in use. Substituting it for the raw `25` makes the code portable across standard Pico hardware revisions:

```c
#include "pico/stdlib.h"
#include "boards/pico.h"   // defines PICO_DEFAULT_LED_PIN = 25

gpio_init(PICO_DEFAULT_LED_PIN);
gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
gpio_put(PICO_DEFAULT_LED_PIN, value);
```

> **Pico W note**: On the Pico W, `PICO_DEFAULT_LED_PIN` maps to the CYW43 chip GPIO, not RP2040 GPIO 25. The code above will **not** compile for Pico W without `pico_cyw43_arch`. This project targets the standard Pico only.

---

## Quick Reference

| Macro | Value (standard Pico) |
|---|---|
| `PICO_DEFAULT_LED_PIN` | `25` |
| On | `gpio_put(pin, 1)` |
| Off | `gpio_put(pin, 0)` |
| Toggle | `gpio_xor_mask(1u << pin)` |
