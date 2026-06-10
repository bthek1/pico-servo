# Plan: ESC (Electronic Speed Controller) Support

Add a dedicated ESC library, webserver integration, and adaptive web UI for
controlling brushed/brushless motors via standard RC PWM.

---

## What Is an ESC

An ESC converts a standard RC PWM signal (50 Hz, 1000–2000 µs) into motor drive
current. It shares the same physical signal as a servo but has a different lifecycle:

| | Servo | ESC |
|---|---|---|
| Signal | 50 Hz PWM, 1000–2000 µs | 50 Hz PWM, 1000–2000 µs |
| Neutral | 1500 µs (positional center) | 1000 µs (off) or 1500 µs (bidirectional stop) |
| Startup | Send pulse, servo moves | **Must arm first** — send min throttle for 2–5 s |
| Calibration | Not needed | Throttle-range calibration (optional but recommended) |
| Failsafe | Holds last position | **Must cut throttle on signal loss** |

---

## ESC Types

### Unidirectional (most common — drones, RC cars forward-only)
| Pulse | Meaning |
|---|---|
| 1000 µs | Off / brake |
| 1000–2000 µs | Throttle 0–100% |
| 2000 µs | Full throttle |

### Bidirectional (RC boats, crawlers, some brushed controllers)
| Pulse | Meaning |
|---|---|
| 1000 µs | Full reverse |
| ~1475–1525 µs | Deadband (stop/brake, ~±25 µs around 1500) |
| 1500 µs | Stop |
| 2000 µs | Full forward |

---

## Arming Sequence (required before first throttle command)

1. ESC powered on
2. Firmware sends **1000 µs continuously for 2–5 seconds**
3. ESC acknowledges with beep/LED sequence (hardware side)
4. ESC now accepts throttle commands

> **Safety rule:** Never arm with a propeller or drivetrain load attached during
> development. Always start with motor disconnected from load.

## Calibration Sequence (optional, teaches ESC the throttle range)

1. Power off ESC
2. Firmware holds **2000 µs** (max)
3. Power on ESC — ESC beeps acknowledging max
4. Firmware moves to **1000 µs** (min) — ESC beeps acknowledging min
5. Calibration saved to ESC EEPROM

> Calibration is a one-time physical procedure. The UI will guide through it but
> the user must manually cycle ESC power at the right moment.

---

## ESC Protocols (scope reference)

| Protocol | Pulse range | Update rate | Notes |
|---|---|---|---|
| **Standard PWM** | 1000–2000 µs | 50 Hz | ← **this plan** |
| OneShot125 | 125–250 µs | ~2 kHz | Faster; same hardware |
| DSHOT300/600 | Digital | 4–12 kHz | No calibration; requires bitbanging |

This plan implements **standard PWM only**. DSHOT/OneShot are future additions.

---

## 1 — Library: `lib/esc`

New library alongside `lib/servo`. ESC has its own header and `.c` because the
lifecycle (arm/disarm state machine) is distinct enough to keep separate.

### 1.1 `esc.h`

```c
#include "pico/stdlib.h"

typedef enum {
    ESC_UNIDIRECTIONAL,   // 1000 = off, 2000 = full throttle
    ESC_BIDIRECTIONAL,    // 1000 = full reverse, 1500 = stop, 2000 = full forward
} esc_type_t;

typedef enum {
    ESC_STATE_DISARMED,
    ESC_STATE_ARMING,     // sending min throttle during arm window
    ESC_STATE_ARMED,
} esc_state_t;

typedef struct {
    esc_type_t type;
    uint16_t   min_us;        // 1000
    uint16_t   max_us;        // 2000
    uint16_t   neutral_us;    // 1000 (unidirectional) or 1500 (bidirectional)
    uint16_t   deadband_us;   // µs each side of neutral (bidirectional only), typically 25
    uint32_t   arm_ms;        // arm hold duration, typically 3000
} esc_config_t;

// Pre-defined presets
extern const esc_config_t ESC_STANDARD;       // unidirectional, 1000–2000 µs, 3 s arm
extern const esc_config_t ESC_BIDIRECTIONAL;  // bidirectional, 1000–2000 µs, 1500 stop, 25 µs deadband

void esc_init(uint gpio, const esc_config_t *cfg);

// Non-blocking — call esc_update() in main loop to drive the arm timer
void        esc_arm(uint gpio);
void        esc_disarm(uint gpio);
esc_state_t esc_get_state(uint gpio);

// Must be called each main loop iteration
void esc_update(uint gpio);

// Safe to call only when armed (no-op otherwise)
void esc_set_throttle(uint gpio, float throttle); // unidirectional: 0.0–1.0
void esc_set_speed(uint gpio, float speed);        // bidirectional: -1.0–+1.0
void esc_brake(uint gpio);                         // sends neutral_us
void esc_set_us(uint gpio, uint16_t pulse_us);     // raw; clamped; armed only
```

### 1.2 State machine in `esc_update()`

```
DISARMED ──arm()──► ARMING ──arm_ms elapsed──► ARMED
ARMED    ──disarm()──► DISARMED (sends min pulse)
ARMING   ──disarm()──► DISARMED
```

`esc_update()` drives the arming timer. While `ARMING`, it holds `min_us`.
All throttle-setting functions are no-ops unless state is `ARMED`.

### 1.3 Failsafe

`esc_update()` also tracks last-command time. If no command received for >500 ms
while ARMED, it sends `neutral_us` (brake). Configurable timeout.

### 1.4 Deadband (bidirectional)

`esc_set_speed()` applies the deadband: if |speed| maps to a pulse within
`neutral_us ± deadband_us`, the pulse is clamped to `neutral_us` to prevent
unintended slow creep.

### 1.5 `lib/esc/CMakeLists.txt`

```cmake
add_library(esc INTERFACE)
target_sources(esc INTERFACE esc.c)
target_include_directories(esc INTERFACE .)
target_link_libraries(esc INTERFACE pico_stdlib hardware_pwm hardware_clocks)
```

---

## 2 — Firmware: `targets/website/website.c`

### 2.1 ESC config at top of `website.c`

```c
#define ESC_GPIO  1   // separate GPIO from servo
static const esc_config_t *s_esc_model = &ESC_STANDARD;
```

### 2.2 `esc_update()` in main loop

```c
while (true) {
    wifi_poll();
    esc_update(ESC_GPIO);   // drives arm timer + failsafe
    ...
}
```

### 2.3 New CGI: `/esc`

| Param | Meaning |
|---|---|
| `arm` | Start arm sequence |
| `disarm` | Disarm immediately |
| `throttle=<0–100>` | Set throttle % (unidirectional, armed only) |
| `speed=<-100–100>` | Set speed % (bidirectional, armed only) |
| `brake` | Send neutral_us |

### 2.4 New dynamic file: `/esc_info.txt`

Served via `fs_open_custom` (same pattern as `/servo_info.txt` already in `website.c`):

```
state=disarmed
type=unidirectional
min_us=1000
max_us=2000
neutral_us=1000
```

or when armed:

```
state=armed
type=bidirectional
min_us=1000
max_us=2000
neutral_us=1500
deadband_us=25
```

The browser polls this to sync the ARM/DISARM button state on load and after
transitions.

---

## 3 — UI: ESC tab in `targets/website/content/index.html`

### 3.1 Tab order

The existing tab bar is `[Home] [Terminal] [Light] [Servo]`. Add ESC at the end:

```
[Home] [Terminal] [Light] [Servo] [ESC]
```

Follow the existing Alpine.js tab pattern — add a button with
`@click="tab='esc'"` and `:class` active/inactive binding, then a
`<section x-show="tab==='esc'">` block.

### 3.2 ESC panel — Alpine component `escPanel()`

Add `escPanel()` alongside `ledPanel()` and `servoPanel()` in the `<script>` block.

```js
function escPanel() {
  return {
    state: 'disarmed',   // 'disarmed' | 'arming' | 'armed'
    type:  'unidirectional',
    throttle: 0,
    armCountdown: 0,
    _armTimer: null,
    _pollTimer: null,
    _warned: false,

    init() {
      fetch('/esc_info.txt').then(r => r.text()).then(t => {
        const p = Object.fromEntries(
          t.trim().split('\n').map(l => l.split('=').map(s => s.trim()))
        );
        this.state = p.state || 'disarmed';
        this.type  = p.type  || 'unidirectional';
      }).catch(() => {});
      window.addEventListener('beforeunload', () => {
        if (this.state !== 'disarmed') navigator.sendBeacon('/esc?disarm');
      });
    },

    // Hold-to-arm: user must hold the button for 1 s
    startArm() {
      if (!this._warned) {
        this._warned = true;
        if (!confirm('Ensure the motor has no load attached.\nDisconnect propellers and gearboxes before arming.')) return;
      }
      this.armCountdown = 3;
      this._armTimer = setInterval(() => {
        this.armCountdown--;
        if (this.armCountdown <= 0) {
          clearInterval(this._armTimer);
          send('/esc?arm');
          this._pollState();
        }
      }, 1000);
    },
    cancelArm() {
      clearInterval(this._armTimer);
      this.armCountdown = 0;
    },
    disarm() {
      send('/esc?disarm');
      this.state = 'disarmed';
      this.throttle = 0;
    },

    _pollState() {
      fetch('/esc_info.txt').then(r => r.text()).then(t => {
        const p = Object.fromEntries(
          t.trim().split('\n').map(l => l.split('=').map(s => s.trim()))
        );
        this.state = p.state || 'disarmed';
        if (this.state === 'arming') setTimeout(() => this._pollState(), 500);
      }).catch(() => {});
    },

    setThrottle(v) {
      this.throttle = v;
      const param = this.type === 'bidirectional' ? 'speed' : 'throttle';
      send('/esc?' + param + '=' + v);
    },
    brake() {
      this.throttle = 0;
      send('/esc?brake');
    },
    setType(t) {
      if (this.state === 'armed') this.disarm();
      this.type = t;
    },

    stateLabel() {
      if (this.armCountdown > 0) return 'Arming… ' + this.armCountdown + 's';
      return this.state.toUpperCase();
    },
    throttleLabel() {
      if (this.type === 'bidirectional') {
        if (Math.abs(this.throttle) < 3) return 'BRAKE';
        return (this.throttle > 0 ? 'FWD ' : 'REV ') + Math.abs(this.throttle) + '%';
      }
      return this.throttle + '%';
    },
  };
}
```

### 3.3 ESC panel layout

Use the same card style as the Servo and Light panels
(`bg-zinc-900 border border-zinc-800 rounded-lg p-5 space-y-5`).

```
┌─────────────────────────────────────────┐
│  ● DISARMED                             │  ← status dot + label (red=disarmed, green=armed)
│                                         │
│  [ ARM (hold) ]   [ DISARM ]            │  ← ARM prominent (red); DISARM always visible
│                                         │
│  ── Throttle (disabled until armed) ──  │
│  0% [═══════════════════════════] 100%  │  ← slider disabled + greyed when disarmed
│                                         │
│  [ BRAKE ]                              │
│                                         │
│  ── Type ──                             │
│  [ Unidirectional ]  [ Bidirectional ]  │
└─────────────────────────────────────────┘
```

Bidirectional mode: slider range becomes -100 to +100; show a greyed deadband
region in the middle using a position-absolute overlay on the slider track.
Direction label (`REV 40%` / `BRAKE` / `FWD 75%`) shown below the large pulse readout.

### 3.4 Safety notes in UI

- ARM button shows hold-countdown in its label ("Arming… 3s") using `x-text`
- First arm triggers a `confirm()` modal (simple; no extra markup needed)
- Throttle slider has `:disabled="state !== 'armed'"` and
  `:class="state !== 'armed' ? 'opacity-40 cursor-not-allowed' : ''"
- `beforeunload` sends `/esc?disarm` via `sendBeacon` (same pattern as servo stop)
- Changing type while armed auto-disarms (handled in `setType()`)

---

## 4 — Safety Summary

| Risk | Mitigation |
|---|---|
| Accidental arm | Hold-to-arm countdown (3 s), confirmation modal on first arm |
| Throttle while disarmed | All throttle calls are no-ops in firmware |
| Signal loss (browser close) | `beforeunload` sends `/esc?disarm`; firmware failsafe brakes at 500 ms |
| Over-speed at startup | Arm sequence enforces 0 throttle first |
| Wrong pulse to ESC | `esc_set_us` clamps to `[min_us, max_us]` |

---

## Implementation Order

1. ✅ **`lib/esc`** — `esc.h`, `esc.c`, `CMakeLists.txt`
2. ✅ **Root `CMakeLists.txt`** — `add_subdirectory(lib/esc)`
3. ✅ **`targets/website/website.c`** — ESC init, `esc_update` in loop, `/esc` CGI, `/esc_info.txt` via `fs_open_custom`
4. ✅ **`targets/website/content/index.html`** — ESC tab button + `<section>` + `escPanel()` script
5. ✅ **`CLAUDE.md`** — document `lib/esc` API

---

## Files Changed / Created

| File | Status | Notes |
|---|---|---|
| `lib/esc/esc.h` | ✅ Done | Types, config struct, `ESC_STANDARD` / `ESC_BIDIR` presets, full API |
| `lib/esc/esc.c` | ✅ Done | 50 Hz PWM, DISARMED→ARMING→ARMED state machine, 500 ms failsafe, deadband |
| `lib/esc/CMakeLists.txt` | ✅ Done | INTERFACE library |
| `CMakeLists.txt` | ✅ Done | `add_subdirectory(lib/esc)` after `lib/servo` |
| `targets/website/CMakeLists.txt` | ✅ Done | `esc` added to `target_link_libraries` |
| `targets/website/website.c` | ✅ Done | `ESC_GPIO 1`, `cgi_esc`, `/esc_info.txt`, `esc_init` + `esc_update`, CGI count 4 |
| `targets/website/content/index.html` | ✅ Done | ESC tab, panel with hold-to-arm, throttle slider, brake, type toggle, `escPanel()` |
| `CLAUDE.md` | ✅ Done | `lib/esc` section added after `lib/servo` |
