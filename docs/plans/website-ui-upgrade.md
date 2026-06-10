# Plan: Website UI Upgrade (htmx + Pico.css + Alpine.js)

Replace the custom vanilla-JS / hand-rolled CSS dashboard in `targets/website/content/index.html`
with the project-standard stack: **htmx + Pico.css + Alpine.js**.  
No new C endpoints needed; existing CGI handlers and `fs_open_custom` files stay the same
except for one small change to `/recv.txt`.

---

## Current state

| Concern | How it works today |
|---|---|
| Styling | Inline custom CSS, dark theme, CSS variables |
| Tab switching | Vanilla JS (`showTab`) |
| Terminal polling | `setInterval` + `fetch('/recv.txt')` every 200 ms, appends to `<pre>` |
| Terminal send | `fetch('/send?data=…')` |
| LED control | `fetch('/led?state=…')` + DOM mutation |
| LED indicator | JS class toggling on a `<div class="led-dot">` |
| Servo panel | JS fetches `/servo_info.txt`, parses key=value, builds DOM with `innerHTML` |
| Servo control | `fetch('/servo?pulse=…')` with 100 ms debounce |
| Servo stop | `navigator.sendBeacon('/servo?stop')` on `beforeunload` |

All dynamic behaviour lives in ~300 lines of inline `<script>`.

---

## Target state

| Concern | New approach |
|---|---|
| Styling | Pico.css CDN (`data-theme="dark"`) |
| Tab switching | Alpine.js `x-data` / `x-show` |
| Terminal polling | `hx-get="/recv.txt" hx-trigger="every 200ms" hx-swap="beforeend"` |
| Terminal send | `hx-post`-style form or Alpine `fetch` (see note below) |
| LED control | htmx `hx-get` on button click |
| LED indicator | Alpine.js reactive dot (`x-bind:class`) |
| Servo panel | Loaded once via `hx-get="/servo_info.txt" hx-trigger="load"`, then Alpine state |
| Servo control | htmx `hx-get` with `hx-trigger="input delay:100ms"` on range slider |
| Servo stop | Alpine `$el.addEventListener('beforeunload', …)` + `navigator.sendBeacon` |

---

## Changes required

### 1 · C firmware — `fs_open_custom` in `website.c`

**`/recv.txt`**: wrap returned bytes in an HTML fragment so htmx can swap them in.

```c
// Before
file->data = s_recv_snapshot;
file->len  = s_recv_snapshot_len;

// After — prepend nothing, just append a newline inside a <span>
// Simplest: build the fragment in the snapshot buffer
s_recv_snapshot_len = snprintf(s_recv_snapshot, sizeof(s_recv_snapshot),
    "<span>%.*s</span>", s_serial_len, s_serial_buf);
```

The `<span>` is inserted into the terminal `<output>` element via
`hx-swap="beforeend"`.  Empty responses (no new data) must return an empty
`<span></span>` so htmx doesn't render a 404 indicator — handle with an `if`
that sets `len = 0` and returns `0` (fall through to an empty static file)
**or** always return `<span></span>`.

**`/servo_info.txt`**: keep returning key=value; Alpine parses it on initial load
(one-time, not polled). No change needed.

### 2 · C firmware — add `content/empty.txt`

Add a zero-byte file so CGI handlers that currently return `/ok.txt` can
optionally return `/empty.txt` for responses where htmx should swap nothing.
Existing `/ok.txt` (content: `ok`) can stay as-is.

### 3 · `content/index.html` — full rewrite

#### Head

```html
<link rel="stylesheet"
      href="https://cdn.jsdelivr.net/npm/@picocss/pico@2/css/pico.min.css">
<script src="https://unpkg.com/htmx.org@2/dist/htmx.min.js"></script>
<script src="https://unpkg.com/alpinejs@3/dist/cdn.min.js" defer></script>
```

#### Layout

Use Pico's semantic elements:
- `<header>`, `<nav>`, `<main>` — Pico styles these out of the box
- `<article>` / `<section>` — card-style panels
- `<progress>` for a connection indicator (htmx `htmx:responseError` event)

#### Tab switching (Alpine)

```html
<div x-data="{ tab: 'terminal' }">
  <nav>
    <ul>
      <li><button :aria-current="tab==='terminal'" @click="tab='terminal'">Terminal</button></li>
      <li><button :aria-current="tab==='light'"    @click="tab='light'">Light</button></li>
      <li><button :aria-current="tab==='servo'"    @click="tab='servo'">Servo</button></li>
    </ul>
  </nav>
  <section x-show="tab==='terminal'">…</section>
  <section x-show="tab==='light'">…</section>
  <section x-show="tab==='servo'">…</section>
</div>
```

#### Terminal panel (htmx + Alpine)

```html
<section x-show="tab==='terminal'">
  <output id="term-out"
          hx-get="/recv.txt"
          hx-trigger="every 200ms"
          hx-swap="beforeend"
          style="display:block; font-family:monospace; height:60vh; overflow-y:auto">
  </output>
  <form x-data="{msg:''}" @submit.prevent="
        fetch('/send?data='+encodeURIComponent(msg)); msg=''">
    <input x-model="msg" placeholder="type and press Enter…" autocomplete="off">
    <button type="submit">→</button>
  </form>
</section>
```

> Terminal send stays as a small Alpine `fetch` call because URL-encoding the
> query param for a GET request is simpler than an htmx form POST here.

#### Light panel (htmx + Alpine)

```html
<section x-show="tab==='light'"
         x-data="{ mode:'off', ms:100 }">
  <div x-bind:class="{ 'led-on': mode==='on', 'led-blink': mode==='blink' }"
       style="width:1rem;height:1rem;border-radius:50%;background:#888">
  </div>
  <button hx-get="/led?state=on"   hx-swap="none" @click="mode='on'">ON</button>
  <button hx-get="/led?state=blink" hx-include="#blink-ms" hx-swap="none"
          @click="mode='blink'">BLINK</button>
  <button hx-get="/led?state=off"  hx-swap="none" @click="mode='off'">OFF</button>

  <input id="blink-ms" name="ms" type="range" min="50" max="5000"
         x-model="ms"
         @change="if(mode==='blink') htmx.ajax('GET','/led?state=blink&ms='+ms,{swap:'none'})">
  <span x-text="ms+' ms'"></span>
</section>
```

#### Servo panel (htmx + Alpine)

```html
<section x-show="tab==='servo'"
         x-data="servoPanel()"
         x-init="init()">

  <div x-text="pulse+' µs'" style="font-size:2rem;font-weight:700"></div>
  <div x-text="label()"></div>

  <input type="range" :min="range.min" :max="range.max" x-model.number="pulse"
         @input.debounce.100ms="send('/servo?pulse='+pulse)">

  <button @click="setMode('positional')" :aria-pressed="mode==='positional'">Positional</button>
  <button @click="setMode('continuous')" :aria-pressed="mode==='continuous'">Continuous</button>

  <button @click="stop()" x-show="mode==='continuous'">■ STOP</button>
  <button @click="centre()" x-show="mode==='positional'">Centre</button>

  <template x-if="mode==='continuous'">
    <input type="range" min="-50" max="50" x-model.number="trim"
           @change="send('/servo?trim='+trim)">
  </template>
</section>

<script>
function servoPanel() {
  return {
    mode: 'positional', pulse: 1500, trim: 0,
    range: { min: 1000, max: 2000 },
    init() {
      fetch('/servo_info.txt').then(r=>r.text()).then(t=>{
        const p = Object.fromEntries(t.trim().split('\n').map(l=>l.split('=')));
        this.mode = p.type || 'positional';
        this.send('/servo?pulse=1500');
      });
      window.addEventListener('beforeunload', ()=>{
        if (this.mode==='continuous') navigator.sendBeacon('/servo?stop');
      });
    },
    send(url) { fetch(url); },
    stop()   { this.pulse=1500; this.send('/servo?stop'); },
    centre() { this.pulse=1500; this.send('/servo?pulse=1500'); },
    setMode(m) { this.mode=m; },
    label() {
      if (this.mode==='continuous') {
        if (Math.abs(this.pulse-1500)<5) return 'STOP';
        return this.pulse>1500 ? 'FWD' : 'REV';
      }
      return '≈ '+Math.round((this.pulse-this.range.min)/(this.range.max-this.range.min)*180)+'°';
    }
  };
}
</script>
```

---

## What is NOT changing

- All CGI handlers in `website.c` (`/send`, `/led`, `/servo`) — untouched
- `fs_open_custom` for `/servo_info.txt` — untouched
- `CMakeLists.txt` content list — `ok.txt` stays; add `empty.txt` if needed
- `lwipopts.h` — no changes

---

## Implementation order

1. Add `content/empty.txt` (zero bytes)
2. Update `fs_open_custom` `/recv.txt` branch to emit `<span>…</span>` (or `<span></span>` when empty)
3. Rewrite `content/index.html` — Terminal panel first (proves polling works end-to-end)
4. Light panel
5. Servo panel
6. Remove all remaining inline `<script>` blocks; verify no vanilla-JS remains
7. Test: compile + flash + open in browser, exercise all three tabs

---

## Completion

Move this file to `docs/plans/completed/` once step 7 passes.
