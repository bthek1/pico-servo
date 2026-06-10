# Plan: Webserver UI — Tabs, Terminal, Light Control, Servo Control

## Goal

Upgrade `targets/webserver` with a polished tabbed single-page dashboard:

| Tab | Features |
|-----|----------|
| **Terminal** | existing bidirectional serial bridge (unchanged) |
| **Light** | LED on / off / blink; blink frequency slider + presets |
| **Servo** | angle arc indicator, slider, preset buttons |

## Design System

### Palette

| Token | Value | Use |
|-------|-------|-----|
| `--bg` | `#0d0d0d` | page background |
| `--surface` | `#1a1a1a` | card / panel background |
| `--border` | `#2a2a2a` | card borders, dividers |
| `--accent` | `#e53935` | active tabs, active buttons, slider thumb |
| `--accent-dim` | `#7f1d1d` | hover on inactive accent elements |
| `--text` | `#e0e0e0` | primary text |
| `--text-muted` | `#666` | labels, placeholders |
| `--green` | `#43a047` | LED-on indicator |
| `--mono` | `'Courier New', monospace` | terminal output/input |

### Typography & Spacing

- Base font: `system-ui, sans-serif`
- Base size: `14px`
- Card padding: `1.25rem`
- Gap between elements: `0.75rem`
- Border radius: `6px` for cards, `4px` for buttons

### Shared Button Style

```css
.btn {
    padding: .45rem .9rem;
    border: 1px solid var(--border);
    border-radius: 4px;
    background: var(--surface);
    color: var(--text);
    cursor: pointer;
    font-size: .85rem;
    transition: background .15s, border-color .15s;
}
.btn:hover   { background: #222; border-color: #444; }
.btn.active  { background: var(--accent); border-color: var(--accent); color: #fff; }
```

---

## Layout

```
┌────────────────────────────────────────────────────────┐
│  ● Pico W Control Panel          192.168.2.x  ↑ 120 s  │  ← header bar
├────────────────────────────────────────────────────────┤
│  [Terminal]  [Light]  [Servo]                          │  ← tab bar
├────────────────────────────────────────────────────────┤
│                                                        │
│   ┌────────────────────────────────────────────────┐   │
│   │  (active panel — card with padding)            │   │
│   └────────────────────────────────────────────────┘   │
│                                                        │
└────────────────────────────────────────────────────────┘
```

Header polls `/recv.txt` every 2 s for uptime; shows IP from page load.
Connection dot pulses green; turns red if `/recv.txt` fails 3× in a row.

---

## Panel Designs

### Terminal

```
┌─────────────────────────────────────────┐
│                                         │
│  > hello                                │
│  Pico W online                          │
│  > world                                │
│                                         │
│  ░░░░░░░░░░░░░░░░░░░░ (scrollable)      │
│                                         │
├─────────────────────────────────────────┤
│  [input field ...................] [→]  │
└─────────────────────────────────────────┘
```

- Monospace output, dark background, auto-scroll
- Enter key or → button sends
- Polls `/recv.txt` every 200 ms (independent of header poll)

### Light

```
┌─────────────────────────────────────────┐
│  LED Status                             │
│                                         │
│           ◉  BLINKING                  │  ← animated dot + label
│                                         │
│  Mode                                   │
│  [ON]  [BLINK]  [OFF]                  │  ← button group, active = red
│                                         │
│  Blink Speed                            │
│  50ms ●━━━━━━━━━━━━━━━━━━━━━━━ 5000ms  │  ← range slider
│            500 ms                       │  ← current value centred
│                                         │
│  Presets                                │
│  [50ms] [100ms] [250ms] [500ms] [1s]   │
└─────────────────────────────────────────┘
```

- Animated dot: CSS `@keyframes pulse` when in BLINK mode, solid green ON, solid grey OFF
- Slider sends `GET /led?state=blink&ms=<val>` on `change` (not `input`, to avoid flooding)
- Preset buttons set both the slider value and fire the request

### Servo

```
┌─────────────────────────────────────────┐
│  Servo Position                         │
│                                         │
│         ╭──────────────╮               │
│       ╱                  ╲             │  ← SVG arc (180° sweep)
│     ╱         90°          ╲           │  ← needle + angle label
│   ╱___________________________╲        │
│                                         │
│  0° ●━━━━━━━━━━━━━━━━━━━━━━━━ 180°    │  ← range slider
│                                         │
│  Presets                                │
│  [0°]  [45°]  [90°]  [135°]  [180°]   │
└─────────────────────────────────────────┘
```

- SVG arc: fixed semicircle track; needle rotates with CSS `transform: rotate()` based on angle
- Slider debounced 100 ms — sends `GET /servo?deg=<val>`
- Preset buttons snap slider and send immediately
- Large angle display (`3rem`, bold) centred above slider

---

## HTML Structure

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <!-- charset, viewport, title -->
  <style>/* all CSS inline — no external deps */</style>
</head>
<body>
  <header>
    <span class="brand"><span id="dot">●</span> Pico W Control Panel</span>
    <span class="meta"><span id="ip"></span> &nbsp; ↑ <span id="uptime">--</span></span>
  </header>

  <nav>
    <button class="tab active" data-tab="terminal" onclick="showTab(this)">Terminal</button>
    <button class="tab"        data-tab="light"    onclick="showTab(this)">Light</button>
    <button class="tab"        data-tab="servo"    onclick="showTab(this)">Servo</button>
  </nav>

  <main>
    <div id="panel-terminal" class="panel card">…</div>
    <div id="panel-light"    class="panel card" hidden>…</div>
    <div id="panel-servo"    class="panel card" hidden>…</div>
  </main>

  <script>/* all JS inline */</script>
</body>
</html>
```

---

## SVG Arc (Servo indicator)

A static 180° semicircle track with a rotating needle line:

```html
<svg id="arc" viewBox="0 0 200 110" width="200" height="110">
  <!-- track -->
  <path d="M10,100 A90,90 0 0,1 190,100"
        fill="none" stroke="#2a2a2a" stroke-width="6" stroke-linecap="round"/>
  <!-- needle — rotates around (100,100) -->
  <line id="needle" x1="100" y1="100" x2="100" y2="15"
        stroke="#e53935" stroke-width="3" stroke-linecap="round"
        transform="rotate(-90, 100, 100)"/>
  <!-- centre dot -->
  <circle cx="100" cy="100" r="5" fill="#e53935"/>
</svg>
```

JS: `needle.setAttribute('transform', \`rotate(${deg - 90}, 100, 100)\`)`
Maps 0°→ rotate(-90°), 90°→ rotate(0°), 180°→ rotate(90°).

---

## JS Architecture (single file, no framework)

```
state = { ledMode, blinkMs, servoDeg, uptime, errorCount }

showTab(btn)          — switch active panel + tab highlight
cmd(url)              — fetch wrapper; increments errorCount on fail
setLed(state, ms?)    — updates state + button highlights + dot animation + sends /led
setServo(deg)         — updates state + needle + slider + sends /servo
pollRecv()            — every 200 ms; appends to terminal; also updates uptime/dot
debounce(fn, ms)      — wraps slider input handlers
```

No `setInterval` for control — only for `pollRecv`. All control is event-driven.

---

## Backend Changes (same as before, unchanged from original plan)

### `webserver.c` additions

- LED state machine: `LED_OFF / LED_ON / LED_BLINK`, runtime `s_blink_ms`
- CGI `/led?state=on|off|blink&ms=<n>`
- CGI `/servo?deg=<0-180>`
- `servo_init(SERVO_GPIO)` + `servo_set_deg(SERVO_GPIO, 90.0f)` on boot
- CGI table updated to 3 handlers: `http_set_cgi_handlers(cgi_handlers, 3)`

### `CMakeLists.txt`

Add `servo` to link libraries.

### `SERVO_GPIO`

```c
#define SERVO_GPIO 2   // ← change to match physical wiring
```

---

## Files Changed

| File | Change |
|------|--------|
| `targets/webserver/webserver.c` | LED state machine; 2 new CGI handlers; servo init |
| `targets/webserver/content/index.html` | full redesign — header, tabs, 3 panels, SVG arc |
| `targets/webserver/CMakeLists.txt` | add `servo` |

---

## Build & Flash

```bash
just deploy webserver
# open http://<ip>/
```

---

## Constraints

- HTML + CSS + JS must stay self-contained (no CDN); current target ~8–12 KB compiled
- `atof` / software float is fine — only called on user input events
- Slider sends on `change` not `input` to avoid request floods while dragging
- CGI handler count must be `3` in `http_set_cgi_handlers`
- `SERVO_GPIO` must match wiring before flash

---

## Extending Later

| Feature | What to add |
|---------|-------------|
| Servo speed ramp | Track target vs current deg in C; step each loop iteration |
| Multiple servos | `/servo?ch=0&deg=90`; channel→GPIO map array |
| Status tab | Card grid: IP, uptime, LED mode, servo position — polled via `/recv.txt` |
| Theme toggle | CSS custom properties already use variables — add a light-mode override class |
