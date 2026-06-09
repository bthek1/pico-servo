# Wiring: DC Supply + Pico W + DF9GMS Servo

## Connections

```
  Raspberry Pi                        DC Supply (5V)
  ┌──────────┐                        ┌──────────────┐
  │          │                        │  (+)    (−)  │
  │  USB port│                        └───┬──────┬───┘
  └────┬─────┘                            │      │
       │ USB (power + programming)        │      │
       │                                  │      │
  ┌────┴─────────────────┐               │      │
  │   Raspberry Pi Pico W│               │      │
  │                      │               │      │    ┌─────────────────┐
  │  GND  [pin 38] ──────┼───────────────┼──────┘    │  DF9GMS Servo   │
  │                      │               │            │                 │
  │  GP0  [pin  1] ──────┼───────────────┼───────────►│  Signal (orange)│
  │                      │               │            │  GND   (brown)  │◄─┐
  └──────────────────────┘               │            │  VCC   (red)    │◄─┼─┐
                                         │            └─────────────────┘  │ │
                                         └───────────────────────────────────┘ │
                                                       shared GND              │
                                         (+)──────────────────────────────────┘
                                               servo VCC
```

## Pin Reference

| Connection | Pico W Pin | Servo Wire | Notes |
|------------|-----------|------------|-------|
| Power in   | VSYS (39) | —          | 1.8–5.5V accepted |
| GND        | GND (38)  | Brown/Black | Shared ground — required |
| PWM Signal | GP0 (1)   | Orange      | 3.3V logic, 50 Hz |
| VCC        | —         | Red         | Direct from DC supply |

> **Shared ground is critical.** The DC supply negative, servo GND, and Pico GND must all connect together.

## Servo Specs (DF9GMS)

- Operating voltage: 4.8V – 6V
- Signal: standard PWM, 50 Hz, 1000–2000 µs pulse width
- Do not power from Pico's 3.3V pin — servo draws too much current

## Code

```c
#include "servo.h"

servo_init(0);              // GP0
servo_set_deg(0, 0.0f);     // full left
servo_set_deg(0, 90.0f);    // centre
servo_set_deg(0, 180.0f);   // full right
servo_set_us(0, 1500);      // centre (raw µs)
```

Link `servo` in `CMakeLists.txt` — pulls in `hardware_pwm` and `hardware_clocks` automatically.

## Notes

- Pico is powered and programmed via USB from the Raspberry Pi — no VSYS wire needed
- DC supply powers the servo only; its `(−)` must still connect to Pico GND (shared reference)
- DC supply `GND` terminal (earth/chassis) — leave unconnected
- To reflash: hold BOOTSEL, replug USB, copy `.uf2` to the mount point (or use `just deploy`)
