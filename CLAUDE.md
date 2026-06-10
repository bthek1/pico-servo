# pico-servo — Claude Code Project Context

## Project

Servo motor control and Wi-Fi firmware for the **Raspberry Pi Pico W** (RP2040 + CYW43439), written in C/C++ with the official Pico SDK.

## Board

- **Board**: Raspberry Pi Pico W — `PICO_BOARD=pico_w`
- **MCU**: RP2040, dual-core Arm Cortex-M0+, 264 KB RAM, 2 MB flash
- **Wireless chip**: CYW43439 (Wi-Fi + Bluetooth) — Wi-Fi available via `lib/wifi`
- **Onboard LED**: wired to CYW43 chip GPIO (`CYW43_WL_GPIO_LED_PIN = 0`), **not** GPIO 25

## Host Machine

The Pico is connected to a **Raspberry Pi** (the flash/monitor machine).

- SSH access: `ssh pi` (username: `bthek1`)
- Flash mount point on Pi: `/media/bthek1/RPI-RP2/`
- **Compile runs locally** (this machine); flash and serial monitor go via `ssh pi`
- The Pi does **not** have a copy of this repo — it only needs the Pico plugged in

All terminal commands run locally unless noted otherwise.

## Project Structure

```
pico-servo/
├── CMakeLists.txt          ← root: SDK init + add_subdirectory for each lib and target
├── compile.sh              ← build all, or a single target: ./compile.sh [target]
├── flash.sh                ← flash a target (default: website): ./flash.sh [target]
├── justfile                ← just commands: deploy, compile, flash, monitor
├── secrets.h               ← Wi-Fi credentials (gitignored, copy from secrets.h.example)
├── secrets.h.example       ← credential template (committed)
├── docs/
│   ├── SETUP.md            ← environment setup guide
│   └── plans/              ← active design/migration plans
│       └── completed/      ← plans moved here once fully implemented
├── lib/
│   ├── pico-sdk/           ← SDK submodule
│   ├── pico-examples/      ← reference submodule (not built, browse only)
│   ├── led/                ← GPIO LED library
│   ├── serial/             ← USB serial library
│   ├── servo/              ← servo PWM library
│   └── wifi/               ← Wi-Fi connection library (lwip, poll mode)
└── targets/
    ├── main/               ← legacy primary target
    ├── blink/              ← LED blink demo
    ├── sweep/              ← servo sweep demo
    ├── webserver/          ← Wi-Fi HTTP server (original)
    └── website/            ← Wi-Fi HTTP server; default for flash/deploy; serves HTML/JS at http://<ip>/
```

## Build & Flash

```bash
# Run locally
./compile.sh              # build all targets
./compile.sh sweep        # build one target
./compile.sh --clean      # clean build (all)
./flash.sh --remote       # scp .uf2 to Pi, then flash default target (website)
./flash.sh --remote sweep # flash a specific target
```

Via justfile (all run locally):

```bash
just deploy           # compile + flash website
just deploy sweep     # compile + flash sweep
just compile sweep    # compile one target
just flash sweep      # flash one target via Pi
just monitor          # open serial monitor (ssh pi)
```

Build output is in `build/` (gitignored). UF2 files land at `build/targets/<target>/<target>.uf2`.

The default target is **`website`** (`build/targets/website/website.uf2`).

Manual flash (from this machine):

```bash
scp build/targets/website/website.uf2 pi:~/pico-flash/website.uf2
ssh pi "cp ~/pico-flash/website.uf2 /media/bthek1/RPI-RP2/"
```

## SDK & CMake Conventions

- SDK lives at `lib/pico-sdk` (git submodule)
- `PICO_SDK_PATH` is set in `CMakeLists.txt`; do **not** rely on env vars
- `pico_sdk_init()` is called once in the root `CMakeLists.txt` only
- `pico_add_extra_outputs(target)` is required to produce `.uf2`
- Stdio is routed via USB (`pico_enable_stdio_usb 1`); UART stdio is off
- Each new program goes in its own subdirectory under `targets/`
- Shared peripheral code goes in a library under `lib/`
- Each target's `CMakeLists.txt` only lists deps that target actually uses

## Shared Libraries

### `lib/led` — LED (CYW43-aware)

```c
#include "led.h"

void led_init(uint gpio);
void led_on(uint gpio);
void led_off(uint gpio);
void led_toggle(uint gpio);
```

Link: `led` (pulls in `pico_stdlib` + `pico_cyw43_arch_none`)

On Pico W the `gpio` argument is ignored — the onboard LED is always `CYW43_WL_GPIO_LED_PIN`.
Pass any value (e.g. `25`) and it compiles cleanly. `led_init` calls `cyw43_arch_init()` internally.

Note: `PICO_CYW43_SUPPORTED` is a CMake variable only — it is **not** a C preprocessor define.
Do not use `#ifdef PICO_CYW43_SUPPORTED` in C source; the CYW43 path in `lib/led` is unconditional.

**CYW43 arch rule:** exactly one `pico_cyw43_arch_*` variant per binary. `led` and `wifi` carry different arch variants — do **not** link both in the same target. Wifi targets use `wifi_led_*` for LED control instead of `led`.

### `lib/serial` — USB Serial

```c
#include "serial.h"

void serial_init(void);                       // replaces stdio_init_all()
void serial_print(const char *fmt, ...);
void serial_println(const char *fmt, ...);
```

Link: `serial` (pulls in `pico_stdlib`)

Use `getchar_timeout_us(0)` for non-blocking read; returns `PICO_ERROR_TIMEOUT` if no char available.

### `lib/wifi` — Wi-Fi (lwip, poll mode)

```c
#include "wifi.h"

wifi_result_t wifi_connect(const char *ssid, const char *password);
bool          wifi_is_connected(void);
const char   *wifi_get_ip(void);
void          wifi_poll(void);       // call each main-loop iteration

void          wifi_led_on(void);
void          wifi_led_off(void);
void          wifi_led_toggle(void);
```

Link: `wifi` (pulls in `pico_stdlib` + `pico_cyw43_arch_lwip_poll`)

`wifi_connect()` handles `cyw43_arch_init()` internally; do not also link `led` in the same target.
`wifi_connect()` uses `CYW43_AUTH_WPA2_AES_PSK` with a 10 s timeout.
`wifi_poll()` drives the lwip stack; must be called in the main loop when using poll mode.

### lwIP HTTP Server (`pico_lwip_http`)

Use the SDK's built-in lwIP httpd to serve HTML/JS pages embedded in the firmware binary.

**CMake pattern:**

```cmake
pico_add_library(mylib_content NOFLAG)
pico_set_lwip_httpd_content(mylib_content INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/content/index.html
    ${CMAKE_CURRENT_LIST_DIR}/content/status.txt
)
target_link_libraries(mytarget wifi serial pico_lwip_http mylib_content)
target_include_directories(mytarget PRIVATE ${PICO_LWIP_CONTRIB_PATH}/apps/httpd)
```

`pico_set_lwip_httpd_content()` runs `makefsdata.py` at build time and generates
`pico_fsdata.inc` in the build dir. The include dir is added automatically.

**Critical:** `lwipopts.h` must define:

```c
#define HTTPD_FSDATA_FILE "pico_fsdata.inc"
```

Without this, lwIP ignores the generated fsdata and falls back to its own built-in demo page.
This define is already present in `lib/wifi/lwipopts.h`.

**CGI handlers** (browser → Pico, query param input):

```c
#include "lwip/apps/httpd.h"

static const char *cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    // pcParam[i] / pcValue[i] are URL-encoded — decode before use
    return "/ok.txt";   // file must exist in fsdata content
}
static const tCGI cgi_handlers[] = { { "/send", cgi_handler } };
httpd_init();
http_set_cgi_handlers(cgi_handlers, 1);
```

**Dynamic responses with `fs_open_custom`** (Pico → browser, runtime data):

Set `LWIP_HTTPD_CUSTOM_FILES 1` in `lwipopts.h` (already set). Then implement:

```c
#include "lwip/apps/fs.h"

static char response_buf[512];
static int  response_len = 0;

int fs_open_custom(struct fs_file *file, const char *name) {
    if (strcmp(name, "/data.txt") == 0) {
        // fill response_buf / response_len from your data source
        file->data  = response_buf;
        file->len   = response_len;
        file->index = response_len;  // pre-loaded: no fs_read needed
        file->flags = 0;             // httpd generates Content-Type from extension
        return 1;                    // 0 = fall through to fsdata
    }
    return 0;
}
void fs_close_custom(struct fs_file *file) { (void)file; }
```

`fs_open_custom` is called before fsdata lookup. `file->index = file->len` signals that
the data is already in memory (lwIP reads it directly without calling `fs_read_custom`).
`response_buf` must remain valid for the lifetime of the HTTP response.

See `targets/webserver/` for a working bidirectional serial terminal using both patterns.

### Frontend Tech Stack (webserver UI)

For dashboard and config pages served from the Pico, use **Tailwind CSS + htmx + Alpine.js**. All load from CDN — zero flash storage cost.

| Layer | Library | When to add |
|---|---|---|
| Styling | Tailwind CSS (play CDN) | Always |
| Interactivity | htmx | Always |
| Reactive UI | Alpine.js | Always (tab switching, sliders, local state) |
| Charts | uPlot | When you need time-series plots |

**Why this stack:**
- The Pico only serves simple HTML fragments — no JSON parsing, no JS state management on the MCU
- htmx handles polling, form submission, and partial updates with HTML attributes alone
- Tailwind utility classes give full control over the dark UI without any custom CSS framework overhead
- Alpine.js handles reactive state (tabs, buttons, sliders) with minimal JS

**Polling example** (sensor data every 2 s):
```html
<div hx-get="/temperature" hx-trigger="every 2s">24°C</div>
```

**CDN links** (put in `<head>`):
```html
<script src="https://cdn.tailwindcss.com"></script>
<script src="https://unpkg.com/htmx.org@2/dist/htmx.min.js"></script>
<script src="https://unpkg.com/alpinejs@3/dist/cdn.min.js" defer></script>
<!-- Add only if needed: -->
<script src="https://unpkg.com/uplot@1/dist/uPlot.iife.min.js"></script>
```

The MCU endpoint returns an HTML fragment (not a full page) for htmx swaps; `fs_open_custom` is the right pattern for those dynamic responses.

### `lib/servo` — Servo PWM

```c
#include "servo.h"

// Model presets
extern const servo_config_t SERVO_SER0006;  // DFRobot 9g positional, 500–2500 µs
extern const servo_config_t SERVO_SG92R;    // TowerPro 9g positional, 500–2500 µs
extern const servo_config_t SERVO_MG996R;   // TowerPro/Olimex continuous rotation, 1000–2000 µs

void servo_init_config(uint gpio, const servo_config_t *cfg); // attach model config
void servo_init(uint gpio);                                    // backward-compat: uses SERVO_SER0006

void servo_set_us(uint gpio, uint16_t pulse_us);   // clamped to cfg.min_us–cfg.max_us
void servo_set_deg(uint gpio, float degrees);       // positional only: 0–180°; no-op on continuous
void servo_set_speed(uint gpio, float speed);       // continuous only: -1.0–+1.0; no-op on positional
void servo_set_stop_us(uint gpio, uint16_t stop_us); // continuous only: update trim/stop point
void servo_safe_stop(uint gpio);                    // sends stop_us regardless of type
```

Link: `servo` (pulls in `pico_stdlib`, `hardware_pwm`, `hardware_clocks`)

PWM: 125 MHz / (100 × 25000) = 50 Hz. Level = pulse_us × 25000 / 20000.

**Servo models:**
- `SERVO_SER0006` and `SERVO_SG92R`: positional, 500–2500 µs. SER0006 nominal spec is 1000–2000 µs but that only yields ~90° travel — 500–2500 µs is empirically correct.
- `SERVO_MG996R`: continuous rotation (TowerPro/Olimex MS-R-9.5-55-MG), 1000–2000 µs, 1500 µs = stop.

**Safety:** `servo_set_us` clamps all pulses to `[min_us, max_us]`. Type-mismatch calls (`set_deg` on continuous, `set_speed` on positional) are silent no-ops. `servo_safe_stop` always works.

### `lib/esc` — ESC (Electronic Speed Controller) PWM

```c
#include "esc.h"

// Presets
extern const esc_config_t ESC_STANDARD;  // unidirectional, 1000–2000 µs, 1000 µs off, 3 s arm
extern const esc_config_t ESC_BIDIR;     // bidirectional, 1000–2000 µs, 1500 µs stop, ±25 µs deadband, 3 s arm

void esc_init(uint gpio, const esc_config_t *cfg);

// State machine — call esc_update() every main loop iteration
void        esc_arm(uint gpio);           // DISARMED → ARMING; holds min_us for cfg.arm_ms
void        esc_disarm(uint gpio);        // → DISARMED; sends min_us
esc_state_t esc_get_state(uint gpio);     // ESC_STATE_DISARMED | ARMING | ARMED

void esc_update(uint gpio);               // drives arm timer + 500 ms failsafe; call every loop

// All no-ops unless ARMED
void esc_set_throttle(uint gpio, float throttle);  // unidirectional: 0.0–1.0
void esc_set_speed(uint gpio, float speed);         // bidirectional: -1.0–+1.0 (deadband applied)
void esc_brake(uint gpio);                          // sends neutral_us; resets failsafe timer
void esc_set_us(uint gpio, uint16_t pulse_us);      // raw; clamped to [min_us, max_us]
```

Link: `esc` (pulls in `pico_stdlib`, `hardware_pwm`, `hardware_clocks`)

PWM: same 50 Hz timing as `lib/servo` — 125 MHz / (100 × 25000).

**State machine:**
```
DISARMED ──esc_arm()──► ARMING ──arm_ms elapsed──► ARMED
ARMED / ARMING ──esc_disarm()──► DISARMED
```
While `ARMING`, `esc_update()` holds `min_us` each loop. All throttle functions are silent no-ops unless `ARMED`.

**Failsafe:** If no throttle command is received for >500 ms while `ARMED`, `esc_update()` sends `neutral_us`. Reset by any call to `esc_set_throttle`, `esc_set_speed`, `esc_set_us`, or `esc_brake`.

**Deadband (bidirectional):** `esc_set_speed()` clamps to `neutral_us` if the computed pulse falls within `neutral_us ± deadband_us`, preventing unintended slow creep.

**CYW43 arch rule:** `esc` links only `hardware_pwm` + `hardware_clocks` — no arch conflict with `wifi`.

## Secrets

Wi-Fi credentials and other secrets live in `secrets.h` at the repo root (gitignored). The committed `secrets.h.example` shows the required defines:

```c
#define WIFI_SSID     "your_ssid_here"
#define WIFI_PASSWORD "your_password_here"
```

To use in a target, add `target_include_directories(<target> PRIVATE ${CMAKE_SOURCE_DIR})` in its `CMakeLists.txt`, then `#include "secrets.h"` in source.

`secrets.h` is never synced via `git pull` — use `just push-secrets` to copy it to the Pi. `just deploy` and `just deploy-clean` run `push-secrets` automatically.

## Timing

Prefer `absolute_time_t` over `sleep_ms` when multiple independent intervals are needed:

```c
absolute_time_t next = make_timeout_time_ms(500);
if (time_reached(next)) {
    // do work
    next = make_timeout_time_ms(500);
}
```

This keeps the loop non-blocking so multiple tasks (blink, serial report, echo) can run concurrently.

## What to Avoid

- `pico_cyw43_arch_lwip_threadsafe_background` — use poll mode (`pico_cyw43_arch_lwip_poll`) instead
- Linking both `pico_cyw43_arch_none` and `pico_cyw43_arch_lwip_poll` in the same binary — pick one
- Bluetooth, MQTT, or TCP/IP beyond lwip — unless explicitly requested
- `PICO_BOARD=pico` — board is Pico W; always use `pico_w`
- Busy-wait loops — use `sleep_ms()` / `sleep_us()` or `absolute_time_t` alarms
- Calling `pico_sdk_init()` more than once
- Calling `stdio_init_all()` directly — use `serial_init()` instead
- Calling `cyw43_arch_init()` directly — `led_init()` handles it for non-wifi targets; `wifi_connect()` handles it for wifi targets
- Adding `lib/pico-examples` to CMake — it is browse-only reference, not built
