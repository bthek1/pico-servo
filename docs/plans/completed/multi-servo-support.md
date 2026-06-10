# Plan: Multi-Servo Type Support

Support positional and continuous rotation servos with per-model configuration,
hardware safety clamping, and a UI that adapts per servo type.

---

## Servo Inventory & Specs

### DFRobot SER0006 / DF9GMS — 9g Micro Servo
| Parameter | Value |
|---|---|
| Type | Positional 180° |
| Pulse range | 500–2500 µs *(empirical — nominal spec says 1000–2000 µs but only yields ~90°)* |
| Center | 1500 µs |
| Dead band | 7 µs |
| Voltage | 4.8–6 V |
| Torque | 1.6 kg·cm @ 4.8V |
| Speed | 0.12 s/60° |
| Gears | Polyamide (nylon) — avoid sustained limit pressure |
| Weight | 9 g |

### TowerPro SG92R — 9g Micro Servo
| Parameter | Value |
|---|---|
| Type | Positional 180° |
| Pulse range | 500–2500 µs |
| Center | 1500 µs |
| Dead band | 5 µs |
| Voltage | 4.8–6.6 V |
| Torque | 2.5 kg·cm @ 4.8V |
| Speed | 0.10 s/60° @ 4.8V |
| Gears | POM with carbon fibre |
| Weight | 9 g |

### TowerPro MG996R / Olimex MS-R-9.5-55-MG — Continuous Rotation Hi-Torque Servo
| Parameter | Value |
|---|---|
| Type | **Continuous rotation** |
| Pulse range | 1000–2000 µs |
| Stop pulse | 1500 µs |
| Dead band | ~10 µs |
| Voltage | 4.8–6 V |
| Torque | 11 kg·cm (1079 mNm / 152.8 oz-in) |
| No-load speed | 60 RPM |
| Gears | Metal (brass + aluminium) |
| Weight | 55 g |

> Pulse mapping: 1000 µs = full reverse, 1500 µs = stop, 2000 µs = full forward.
> Stop point may need trim calibration — exact neutral varies by unit.

---

## 1 — Library: `lib/servo`

### 1.1 Add `servo_config_t` and model presets to `servo.h`

```c
typedef enum {
    SERVO_POSITIONAL,   // angle control 0–180°
    SERVO_CONTINUOUS,   // speed control; 1500 µs = stop
} servo_type_t;

typedef struct {
    servo_type_t type;
    uint16_t     min_us;   // pulse for 0° or full-reverse
    uint16_t     max_us;   // pulse for 180° or full-forward
    uint16_t     stop_us;  // safe neutral (positional = 90°, continuous = stop)
} servo_config_t;

// Pre-defined model presets
extern const servo_config_t SERVO_SER0006;  // DFRobot 9g positional, 500–2500 µs
extern const servo_config_t SERVO_SG92R;    // TowerPro 9g positional, 500–2500 µs
extern const servo_config_t SERVO_MG996R;   // TowerPro/Olimex continuous rotation, 1000–2000 µs
```

### 1.2 Updated API

```c
// Replace servo_init() — attaches a config to the GPIO slot
void servo_init_config(uint gpio, const servo_config_t *config);

// Raw pulse — clamped to [config.min_us, config.max_us], always safe
void servo_set_us(uint gpio, uint16_t pulse_us);

// Positional only — degrees 0–180, clamped; no-op on SERVO_CONTINUOUS
void servo_set_deg(uint gpio, float degrees);

// Continuous only — speed -1.0 (full reverse) to +1.0 (full forward); no-op on SERVO_POSITIONAL
void servo_set_speed(uint gpio, float speed);

// Safe-stop: sends stop_us regardless of type — always works
void servo_safe_stop(uint gpio);
```

Keep old `servo_init(gpio)` as a thin wrapper for `servo_init_config(gpio, &SERVO_SER0006)` so existing targets don't break.

### 1.3 Internal config table

`servo.c` keeps a small static table indexed by GPIO (0–29) holding a `servo_config_t` per slot. `servo_init_config` fills the slot; all other functions look it up for clamping and type guards.

### 1.4 Safety clamping in `servo_set_us`

```c
// Inside servo_set_us:
if (pulse_us < cfg->min_us) pulse_us = cfg->min_us;
if (pulse_us > cfg->max_us) pulse_us = cfg->max_us;
```

This is the final safety net — even a buggy caller cannot send an out-of-range pulse.

### 1.5 Slew rate note

Slew-rate limiting (max µs change per call) would protect metal-gear servos from
shock loads but adds per-GPIO state and a timer. **Deferred** — add only if the
MG996R shows mechanical stress in practice.

---

## 2 — Webserver: `targets/webserver`

### 2.1 Servo config at the top of `webserver.c`

```c
#define SERVO_GPIO   0
static const servo_config_t *SERVO_MODEL = &SERVO_SER0006;
```

Changing the model requires only this one-line edit.

### 2.2 CGI: `/servo`

Extended to handle both types:

| Param | Meaning | Valid range |
|---|---|---|
| `deg=<n>` | Set angle (positional) | 0–180 |
| `speed=<n>` | Set speed (continuous) | -100–100 |
| `trim=<n>` | Adjust stop pulse offset (continuous) | -50–+50 µs |
| `stop` | Safe stop | — |

CGI ignores `deg` on a continuous servo and `speed`/`trim` on a positional servo (matches library no-ops).

### 2.3 CGI: expose servo type and range to the browser

Add `/servo_info.txt` via `fs_open_custom`:

```
type=positional
min=0
max=180
```

or for continuous:

```
type=continuous
stop=1500
```

The page fetches this on load and builds the right control.
`stop` is the configured stop pulse (default 1500 µs); the trim control adjusts this value.

---

## 3 — UI: `targets/webserver/content/index.html`

### 3.1 Fetch servo type on page load

```js
fetch('/servo_info.txt')
  .then(r => r.text())
  .then(buildServoUI);
```

### 3.2 Positional servo UI (current behaviour, refined)

- Slider 0–180° with degree readout
- Sends `/servo?deg=<n>`
- "Centre (90°)" shortcut button

### 3.3 Continuous rotation servo UI

- Slider −100 to +100 (speed %)
- Large **STOP** button always visible, sends `/servo?stop`
- Sends `/servo?speed=<n>` on drag
- **Trim control**: ±50 µs offset around 1500 µs stop point — the Olimex MS-R-9.5-55-MG stop
  pulse varies by unit and may need slight adjustment to hold truly still

### 3.4 Common safety elements

- **Emergency STOP** button present for both types (continuous: prominent; positional: small)
- Input disabled while a request is in flight (prevents pulse flooding)
- On page unload (`beforeunload`), fire `/servo?stop` — servo won't keep running if the browser closes

---

## Implementation Order

1. **`lib/servo`** — add `servo_config_t`, presets, updated API, config table, clamping
2. **`targets/webserver/webserver.c`** — switch to `servo_init_config`, extend CGI, add `servo_info.txt`
3. **`targets/webserver/content/index.html`** — fetch type, build adaptive UI
4. **Existing targets** — `sweep`, `main` compile-check after backward-compat wrapper added
5. **CLAUDE.md** — update `lib/servo` section with new API

---

## Files Changed

| File | Change |
|---|---|
| `lib/servo/servo.h` | Add `servo_config_t`, presets, new API |
| `lib/servo/servo.c` | Config table, clamping, type guards, backward-compat shim |
| `targets/webserver/webserver.c` | `servo_init_config`, extended CGI, `servo_info.txt` |
| `targets/webserver/content/index.html` | Adaptive servo UI, stop-on-unload |
| `CLAUDE.md` | Update servo library section |
