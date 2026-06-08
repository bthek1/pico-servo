# Pico W Migration Plan

Migrate the project from the standard Raspberry Pi Pico (`PICO_BOARD=pico`) to the
Raspberry Pi Pico W (`PICO_BOARD=pico_w`).

---

## Key difference: the onboard LED

On the standard Pico, the LED is a plain GPIO output on pin 25 (`PICO_DEFAULT_LED_PIN = 25`).

On the Pico W, **`PICO_DEFAULT_LED_PIN` is not defined**. The LED is wired to the CYW43
wireless chip's GPIO (`CYW43_WL_GPIO_LED_PIN = 0`). Driving it requires:

1. Linking `pico_cyw43_arch_none` (initialises the chip without enabling networking)
2. Calling `cyw43_arch_init()` at startup
3. Using `cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, value)` instead of `gpio_put`

The `lib/led` library will absorb this difference with conditional compilation so all
targets keep the same `led_*` API.

---

## Files to change

### 1 — `compile.sh`

Change the CMake board flag:

```diff
- cmake .. -DPICO_BOARD=pico
+ cmake .. -DPICO_BOARD=pico_w
```

### 2 — `lib/led/CMakeLists.txt`

Add `pico_cyw43_arch_none` so the CYW43 headers and init are available to `led.c`.
This propagates transitively to every target that links `led`:

```cmake
add_library(led INTERFACE)

target_sources(led INTERFACE led.c)
target_include_directories(led INTERFACE .)
target_link_libraries(led INTERFACE
    pico_stdlib
    pico_cyw43_arch_none   # ← new: needed for CYW43 LED on Pico W
)
```

### 3 — `lib/led/led.h`

No API change — public header stays identical. All targets continue to call:

```c
led_init(LED_PIN);
led_toggle(LED_PIN);
```

### 4 — `lib/led/led.c`

Add conditional compilation. When built for Pico W (`PICO_CYW43_SUPPORTED` is defined
by the SDK), route through the CYW43 driver; otherwise use plain GPIO:

```c
#include "led.h"
#include "pico/stdlib.h"

#ifdef PICO_CYW43_SUPPORTED
#include "pico/cyw43_arch.h"

void led_init(uint gpio) {
    (void)gpio;
    cyw43_arch_init();
}

void led_on(uint gpio)     { (void)gpio; cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); }
void led_off(uint gpio)    { (void)gpio; cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); }
void led_toggle(uint gpio) {
    (void)gpio;
    static bool state = false;
    state = !state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
}

#else

void led_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
}

void led_on(uint gpio)     { gpio_put(gpio, 1); }
void led_off(uint gpio)    { gpio_put(gpio, 0); }
void led_toggle(uint gpio) { gpio_put(gpio, !gpio_get(gpio)); }

#endif
```

> `gpio` is ignored on Pico W — the only onboard LED is always `CYW43_WL_GPIO_LED_PIN`.
> Callers can still pass any value (e.g. `LED_PIN 25`) and it compiles cleanly.

### 5 — Target `main.c` files

No code changes required — `led_init` / `led_toggle` / etc. calls are unchanged.

`LED_PIN 25` defined in each target is passed in but ignored at runtime on Pico W, so
no rename is needed. Optionally tidy to:

```c
#define LED_PIN 0   // CYW43_WL_GPIO_LED_PIN on Pico W; GPIO 25 on standard Pico
```

But this is cosmetic only.

### 6 — `CLAUDE.md`

- Change board section: `PICO_BOARD=pico_w`, Pico W, CYW43 chip present
- Remove the blanket "do not suggest cyw43" restriction — replace with:
  "use `pico_cyw43_arch_none` only (no networking); do not use `pico_cyw43_arch_lwip_*`
  or Wi-Fi / MQTT APIs unless explicitly requested"
- Update "What to Avoid" accordingly
- Add note that `stdio` is still via USB, not Wi-Fi

### 7 — `docs/SETUP.md` and `docs/blink-led.md`

- Update board table to Pico W
- Note the CYW43 LED difference
- Update flash mount: still `/media/bthek1/RPI-RP2` (same)
- `SETUP.md` section 11 (pico-examples): remove instruction to add to CMake —
  it's already a browse-only submodule

---

## What does NOT change

| Thing | Reason |
|---|---|
| Servo PWM code | Uses `hardware_pwm` — unaffected by board change |
| Serial / USB stdio | `pico_enable_stdio_usb` works the same on Pico W |
| `flash.sh` / `justfile` | Board flag is only in `compile.sh` |
| Target `CMakeLists.txt` files | `pico_cyw43_arch_none` comes in transitively via `led` |
| `lib/serial` | No change |
| `lib/servo` | No change |

---

## Steps (in order)

- [ ] `compile.sh` — change `PICO_BOARD` to `pico_w`
- [ ] `lib/led/CMakeLists.txt` — add `pico_cyw43_arch_none`
- [ ] `lib/led/led.c` — add `#ifdef PICO_CYW43_SUPPORTED` branches
- [ ] `CLAUDE.md` — update board section and "What to Avoid"
- [ ] `docs/SETUP.md` — update board table and notes
- [ ] `docs/blink-led.md` — update LED pin notes
- [ ] Clean build and flash to verify

---

## Future Wi-Fi

When Wi-Fi is needed, swap `pico_cyw43_arch_none` → `pico_cyw43_arch_lwip_threadsafe_background`
(or poll variant) in the relevant target's `CMakeLists.txt`. The `lib/led` library stays on
`none` since the LED driver does not need networking.
