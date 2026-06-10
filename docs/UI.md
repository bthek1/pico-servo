For a microcontroller dashboard/config page, use:

**Tailwind CSS + htmx + Alpine.js**

All load from CDN — zero flash storage cost.

- **Tailwind CSS** (play CDN) handles all styling with utility classes. Full control over dark UI, no semantic-HTML constraints, no framework opinions.
- **htmx** handles all dynamic updates (poll sensor data, submit config forms) with just HTML attributes — zero JS to write for server interactions.
- **Alpine.js** handles reactive client state: tab switching, button toggles, sliders, local display values — lightweight, no build step.

**CDN links:**
```html
<script src="https://cdn.tailwindcss.com"></script>
<script src="https://unpkg.com/htmx.org@2/dist/htmx.min.js"></script>
<script src="https://unpkg.com/alpinejs@3/dist/cdn.min.js" defer></script>
```

**Polling example** (sensor data every 2 s):
```html
<div hx-get="/temperature" hx-trigger="every 2s" hx-swap="innerHTML">24°C</div>
```

The MCU endpoint returns a plain text or HTML fragment for htmx swaps; `fs_open_custom` is the right pattern for those dynamic responses (see CLAUDE.md).

For **charts**, add **uPlot** — fast, small, handles time-series well:
```html
<script src="https://unpkg.com/uplot@1/dist/uPlot.iife.min.js"></script>
```

| Layer | Library |
|---|---|
| Styling | Tailwind CSS |
| Interactivity | htmx |
| Reactive UI | Alpine.js |
| Charts (if needed) | uPlot |
