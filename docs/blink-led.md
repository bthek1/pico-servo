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

1. **Add the GPIO init** — call `gpio_init(25)` and `gpio_set_dir(25, GPIO_OUT)` once at startup, before the loop.
2. **Toggle in the loop** — `gpio_put(25, 1)` turns the LED on; `gpio_put(25, 0)` turns it off.
3. **SSH into the Raspberry Pi** (the build/flash host):
   ```bash
   ssh pi
   ```
4. **Build** on the Pi:
   ```bash
   ./compile.sh
   ```
5. **Flash** — hold BOOTSEL on the Pico, plug it into the Pi's USB, then:
   ```bash
   ./flash.sh
   ```
   Or manually:
   ```bash
   cp build/main/pico_servo.uf2 /media/pi/RPI-RP2/
   ```
6. **Verify** — the onboard LED should blink at 1 Hz (500 ms on / 500 ms off).

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
