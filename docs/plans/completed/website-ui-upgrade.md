# Plan: Website UI Upgrade (Tailwind CSS + htmx + Alpine.js)

Replace the custom vanilla-JS / hand-rolled CSS dashboard in `targets/website/content/index.html`
with the project-standard stack: **Tailwind CSS + htmx + Alpine.js**.  
No new C endpoints needed; existing CGI handlers and `fs_open_custom` files stay the same
except for one small change to `/recv.txt`.

**Status: completed.**

---

## What changed

| Concern | Before | After |
|---|---|---|
| Styling | Inline custom CSS, CSS variables | Tailwind CSS utility classes (play CDN) |
| Tab switching | Vanilla JS (`showTab`) | Alpine.js `x-data` / `x-show` |
| Terminal polling | `setInterval` + `fetch('/recv.txt')` | `hx-get="/recv.txt" hx-trigger="every 200ms" hx-swap="beforeend"` |
| Terminal send | `fetch` + DOM append | Alpine `@submit.prevent` + `fetch` |
| LED control | `fetch` + DOM mutation | Alpine `ledPanel()` component |
| LED indicator | JS class toggling | Alpine `:class` binding + CSS animation |
| Servo panel | `innerHTML` DOM build from key=value | Alpine `servoPanel()` component |
| Servo control | `fetch` with debounce | Alpine `debounceSend` + `fetch` |
| Servo stop | `navigator.sendBeacon` on `beforeunload` | Same, wired in Alpine `init()` |

---

## C changes (`website.c`)

- `s_recv_snapshot` buffer widened from `SERIAL_BUF_SZ` to `SERIAL_BUF_SZ + 16`
- `fs_open_custom /recv.txt` now wraps output in `<span>…</span>` for htmx `beforeend` swap

## Content changes

- `content/empty.txt` added (zero bytes)
- `content/index.html` fully rewritten with Tailwind + htmx + Alpine.js
- `CMakeLists.txt` — `empty.txt` added to `pico_set_lwip_httpd_content`
