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


Good coverage. Let me pull the full feature comparison table.Great, got everything I need. Let me put together a comprehensive breakdown.Here's the full rundown on ESC firmware:

---

**What ESC firmware actually does**

The firmware is the software running on the ESC's microcontroller. It determines how throttle commands from the flight controller get converted into three-phase motor drive signals — affecting throttle linearity, timing, motor noise, telemetry, and efficiency. Choosing the wrong firmware (or running outdated firmware) can cause desyncs, motor stutters, jitter, or thermal shutdown.

---

**The three main options**

**BLHeli_32** was the undisputed king for years. It was the gold standard — closed-source but extremely reliable — until the developer discontinued it in 2024, meaning no new features or bug fixes will ever be released. Existing BLHeli_32 ESCs still work perfectly and will continue to do so indefinitely, but you can't flash it to new hardware anymore.

**Bluejay** is the go-to upgrade for anyone with BLHeli_S hardware (the older 8-bit EFM8-based ESCs). It's BLHeli_S based firmware capable of bidirectional DShot, so a great choice if you want to run RPM filtering, and it also includes a startup sound editor. Bluejay is favoured for its superior performance, robust feature set, and consistent updates over stock BLHeli_S. The one ceiling is hardware — the 8-bit EFM8 MCU struggles with very high RPM motors (50k+) or 8K PID loops with simultaneous telemetry.

**AM32** is where everything is heading. It's an open-source firmware for brushless ESCs, born from a desire to break free from the limitations of closed-source alternatives like BLHeli_32. Its name comes from its support for ARM-based MCUs and its design target for 32-bit processors. It implements advanced Field-Oriented Control (FOC) techniques, providing exceptionally smooth operation especially at low throttle — crucial for cinematic drones — along with high RPM stability and reduced motor heating. AM32 also supports bidirectional DShot, allowing the ESC to send real-time telemetry (RPM, temperature, current) back to the flight controller over the same signal wire — critical for RPM filtering.

---

**Protocols you need to know**

DShot300/600 has become the FPV mainstream, while DShot1200/2400 on high-end ESCs can provide faster and cleaner throttle signals. Bidirectional DShot is the key feature — it enables **RPM filtering** in Betaflight, where each ESC sends its motor's exact RPM to the flight controller, which calculates the fundamental motor frequency and its harmonics, then applies narrow notch filters that track those frequencies in real time, removing only motor noise while filter latency drops dramatically. Note that bidirectional DShot requires DShot300 or DShot600 — DShot150 does not leave enough time in the frame for telemetry data to return.

AM32 also supports **Extended DShot Telemetry (EDT)**, which interleaves temperature, voltage, and current data into the eRPM telemetry frames, enabling ESC health monitoring through the same signal wire without requiring a separate serial telemetry connection.

---

**The bottom line in 2026**

If you're buying new ESCs, target AM32 or Bluejay — both are actively maintained and support RPM filtering. BLHeli_32 ESCs still work great, but the firmware won't receive updates.

If you already have BLHeli_32 ESCs, there's no immediate need to switch. Don't rush into flashing AM32 because there's no real benefit right now, and once you flash AM32 there's no going back — it's better to wait and make an informed decision.