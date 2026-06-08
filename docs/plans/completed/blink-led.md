# Blink Onboard LED

**Target**: `targets/blink/`  
**Board**: Raspberry Pi Pico (standard) — GPIO 25  
**Libraries**: `lib/led`, `lib/serial`

---

## How the Onboard LED Works

On the standard Pico, the onboard LED is wired directly to **GPIO 25** on the RP2040. The `lib/led` library wraps GPIO init and control into simple calls.

---

## Source

`targets/blink/main.c`:

```c
#include "led.h"
#include "serial.h"

#define LED_PIN  25
#define BLINK_MS 500

int main() {
    serial_init();
    led_init(LED_PIN);

    serial_println("blink start");

    while (true) {
        led_toggle(LED_PIN);
        sleep_ms(BLINK_MS);
    }
}
```

`targets/blink/CMakeLists.txt`:

```cmake
add_executable(blink main.c)
target_link_libraries(blink led serial)
pico_add_extra_outputs(blink)
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)
```

---

## Build & Flash

```bash
just compile blink
just flash blink
# or in one step:
just deploy blink
```

Output: `build/targets/blink/blink.uf2`

---

## LED API (`lib/led`)

| Function | Description |
|---|---|
| `led_init(gpio)` | Configure GPIO as output |
| `led_on(gpio)` | Drive high |
| `led_off(gpio)` | Drive low |
| `led_toggle(gpio)` | Flip state |

## Notes

- `PICO_DEFAULT_LED_PIN` is defined as `25` for the standard Pico. Using the raw constant `25` is fine for this board-locked project.
- On the Pico W, GPIO 25 is **not** the LED — the LED is behind the CYW43 chip. This project targets the standard Pico only.
