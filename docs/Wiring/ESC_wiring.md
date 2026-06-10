# Wiring: Battery + Pico W + ESC + Brushless Motor

## Connections

```
  Raspberry Pi                        LiPo / DC Battery
  ┌──────────┐                        ┌──────────────────┐
  │          │                        │   (+)      (−)   │
  │  USB port│                        └────┬─────────┬───┘
  └────┬─────┘                             │         │
       │ USB (power + programming)         │         │
       │                                   │         │
  ┌────┴─────────────────┐                │         │
  │   Raspberry Pi Pico W│                │         │
  │                      │                │         │
  │  GND  [pin 38] ───────────────────────┼─────────┘
  │                      │                │                ┌──────────────────┐
  │  GP1  [pin  2] ───────────────────────┼───────────────►│  ESC signal      │
  │                      │                │                │                  │
  └──────────────────────┘                │                │  ESC GND ────────┼── to shared GND
                                          │                │                  │
                                          └───────────────►│  ESC battery (+) │
                                                           │                  │
                                                           │  Phase A ────────┼───┐
                                                           │  Phase B ────────┼───┤── Brushless Motor
                                                           │  Phase C ────────┼───┘
                                                           └──────────────────┘
```

## Pin Reference

| Connection | Pico W Pin | ESC Wire | Notes |
|---|---|---|---|
| GND | GND (38) | GND (black) | Shared ground — required |
| PWM Signal | GP1 (2) | Signal (white/yellow/orange) | 3.3V logic, 50 Hz |
| Battery (+) | — | Battery + (red thick) | Direct from battery positive |
| Battery (−) | — | Battery − (black thick) | To shared GND |

> **Shared ground is critical.** Battery negative, ESC signal GND, and Pico GND must all connect together. Floating signal ground causes missed pulses and erratic behaviour.

> **Do not power the ESC signal wire's 5V BEC from the Pico's 3V3/VSYS pins** — use it to power other peripherals only if you know the BEC current rating.

## ESC Signal Wire Colours

ESC signal connectors are typically a 3-pin servo-style header:

| Pin | Colour (common) | Function |
|---|---|---|
| 1 | White / Yellow / Orange | PWM signal |
| 2 | Red | BEC 5V (optional — often left unconnected) |
| 3 | Black / Brown | Signal GND |

## Arming Sequence

Standard ESCs require an arming sequence before accepting throttle. `lib/esc` handles this automatically:

1. Call `esc_arm()` — library holds `min_us` (1000 µs) for the arm duration (default 3 s)
2. ESC acknowledges with a beep/LED sequence
3. State transitions to `ARMED` — throttle commands now accepted

The ESC will **not** respond to throttle commands until armed.

## Calibration (one-time, optional)

Teaches the ESC your pulse range — only needed once per ESC:

1. Power **off** the ESC
2. Send 2000 µs (max) from the Pico and power **on** the ESC — ESC beeps high
3. Drop to 1000 µs (min) — ESC beeps confirmation
4. Calibration saved to ESC EEPROM

## Code

```c
#include "esc.h"

#define ESC_GPIO 1   // GP1

int main(void) {
    serial_init();
    esc_init(ESC_GPIO, &ESC_STANDARD);
    esc_arm(ESC_GPIO);  // starts non-blocking arm sequence

    while (true) {
        esc_update(ESC_GPIO);  // must be called every loop — drives arm timer + failsafe

        if (esc_get_state(ESC_GPIO) == ESC_STATE_ARMED) {
            esc_set_throttle(ESC_GPIO, 0.5f);  // 50% throttle
        }
    }
}
```

For bidirectional ESCs (boats, crawlers):

```c
esc_init(ESC_GPIO, &ESC_BIDIR);
// ...
esc_set_speed(ESC_GPIO,  1.0f);   // full forward
esc_set_speed(ESC_GPIO,  0.0f);   // stop (deadband applied)
esc_set_speed(ESC_GPIO, -0.5f);   // half reverse
esc_brake(ESC_GPIO);              // sends neutral_us immediately
```

Link `esc` in `CMakeLists.txt` — pulls in `hardware_pwm` and `hardware_clocks` automatically.

## Safety Notes

- **Never arm with a propeller or drivetrain attached** during development
- Firmware failsafe: if no throttle command received for >500 ms while armed, `esc_update()` sends `neutral_us` automatically
- `esc_set_us()` clamps all pulses to `[min_us, max_us]`
- All throttle functions are silent no-ops unless state is `ARMED`
- To reflash: hold BOOTSEL, replug USB, copy `.uf2` to the mount point (or use `just deploy`)
