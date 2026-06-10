# ESC Support — pico-servo

Electronic Speed Controllers use the same 50 Hz PWM signal as servos (1000–2000 µs) but have a different lifecycle: they must be **armed** before accepting throttle commands and will cut power on signal loss.

## ESC vs Servo

| | Servo | ESC |
|---|---|---|
| Signal | 50 Hz PWM, 1000–2000 µs | 50 Hz PWM, 1000–2000 µs |
| Neutral | 1500 µs (center) | 1000 µs (off) or 1500 µs (bidirectional stop) |
| Startup | Send pulse → moves | Must arm first (hold min throttle for 2–5 s) |
| Signal loss | Holds last position | Firmware failsafe must cut throttle |

## ESC Types

**Unidirectional** (drones, RC cars forward-only):

| Pulse | Meaning |
|---|---|
| 1000 µs | Off / brake |
| 1000–2000 µs | 0–100% throttle |
| 2000 µs | Full throttle |

**Bidirectional** (boats, crawlers, brushed controllers):

| Pulse | Meaning |
|---|---|
| 1000 µs | Full reverse |
| ~1475–1525 µs | Deadband (stop) |
| 1500 µs | Stop |
| 2000 µs | Full forward |

## `lib/esc` API

```c
#include "esc.h"

// Presets
extern const esc_config_t ESC_STANDARD;  // unidirectional, 1000–2000 µs, 3 s arm
extern const esc_config_t ESC_BIDIR;     // bidirectional, 1000–2000 µs, 1500 stop, ±25 µs deadband, 3 s arm

void esc_init(uint gpio, const esc_config_t *cfg);

// State machine — esc_update() must be called every main loop iteration
void        esc_arm(uint gpio);           // DISARMED → ARMING; holds min_us for arm_ms
void        esc_disarm(uint gpio);        // → DISARMED; sends min_us
esc_state_t esc_get_state(uint gpio);     // ESC_STATE_DISARMED | ARMING | ARMED
void        esc_update(uint gpio);        // drives arm timer + 500 ms failsafe

// All no-ops unless ARMED
void esc_set_throttle(uint gpio, float throttle);  // unidirectional: 0.0–1.0
void esc_set_speed(uint gpio, float speed);         // bidirectional: -1.0–+1.0
void esc_brake(uint gpio);                          // sends neutral_us
void esc_set_us(uint gpio, uint16_t pulse_us);      // raw; clamped to [min_us, max_us]
```

Link: `esc` (pulls in `pico_stdlib`, `hardware_pwm`, `hardware_clocks`)

**State machine:**
```
DISARMED ──esc_arm()──► ARMING ──arm_ms elapsed──► ARMED
ARMED / ARMING ──esc_disarm()──► DISARMED
```

**Failsafe:** If no command received for >500 ms while ARMED, `esc_update()` sends `neutral_us`. Reset by any throttle call.

**Deadband:** `esc_set_speed()` snaps to `neutral_us` if computed pulse falls within `neutral_us ± deadband_us`.

## Usage Example

```c
#include "esc.h"

#define ESC_GPIO 1

int main(void) {
    serial_init();
    esc_init(ESC_GPIO, &ESC_STANDARD);
    esc_arm(ESC_GPIO);  // starts non-blocking arm sequence

    while (true) {
        esc_update(ESC_GPIO);  // drives arm timer and failsafe

        if (esc_get_state(ESC_GPIO) == ESC_STATE_ARMED) {
            esc_set_throttle(ESC_GPIO, 0.5f);  // 50% throttle
        }
    }
}
```

CMakeLists.txt for a target:
```cmake
target_link_libraries(mytarget esc serial)
```

## Arming Sequence

The `lib/esc` state machine handles arming automatically — `esc_arm()` sends `min_us` for `arm_ms` (default 3000 ms) then transitions to `ARMED`. No manual power cycling required for standard arming.

**Throttle-range calibration** (one-time, optional — teaches the ESC your pulse range):
1. Power off the ESC
2. Hold 2000 µs before powering on — ESC beeps acknowledging max
3. Drop to 1000 µs — ESC beeps acknowledging min
4. Calibration saved to ESC EEPROM

## Safety Rules

- Never arm with a propeller, gear, or load attached during development
- `esc_set_us` clamps all pulses to `[min_us, max_us]`
- Throttle functions are silent no-ops unless state is `ARMED`
- Firmware failsafe brakes at 500 ms if no command received

## PWM Protocols (scope)

This library implements **standard PWM only** (50 Hz, 1000–2000 µs). Other protocols are out of scope:

| Protocol | Range | Update rate | Notes |
|---|---|---|---|
| **Standard PWM** ← implemented | 1000–2000 µs | 50 Hz | Works with any ESC |
| OneShot125 | 125–250 µs | ~2 kHz | Faster; same hardware |
| DSHOT300/600 | Digital | 4–12 kHz | No calibration; requires PIO |

DSHOT on the Pico would use PIO state machines — each frame is 16 bits (11 throttle, 1 telemetry, 4 CRC), T0H ≈ 37.5% duty, T1H ≈ 75% duty.

## Website Target Integration

The `website` target exposes ESC control via:

- **CGI `/esc`** — params: `arm`, `disarm`, `throttle=<0–100>`, `speed=<-100–100>`, `brake`
- **Dynamic file `/esc_info.txt`** — served via `fs_open_custom`, returns current state:
  ```
  state=disarmed
  type=unidirectional
  min_us=1000
  max_us=2000
  neutral_us=1000
  ```
- **ESC tab in UI** — hold-to-arm countdown, throttle slider, brake button, unidirectional/bidirectional toggle
