For a microcontroller dashboard/config page, I'd go with:

**htmx + Pico.css**

That's basically it. Here's why it's the right fit:

- **htmx** handles all your dynamic updates (poll sensor data, submit config forms, WebSocket support) with just HTML attributes — zero JS to write
- **Pico.css** gives you a clean, responsive UI with semantic HTML, no classes needed
- Both load from CDN, using zero flash storage
- The MCU just serves simple HTML fragments — no JSON parsing, no JS state management

For example, polling a sensor every 2 seconds is just:
```html
<div hx-get="/temperature" hx-trigger="every 2s">24°C</div>
```

The only reason to add anything else is if you need **charts**, in which case add **uPlot** — it's the most MCU-friendly charting lib (fast, small, handles time-series perfectly).

If you find yourself needing more interactivity (forms with live validation, toggles, etc.), sprinkle in **Alpine.js** — it plays nicely alongside htmx.

So the full "heavy" version would be:

| Layer | Library |
|---|---|
| Styling | Pico.css |
| Interactivity | htmx |
| Reactive UI (if needed) | Alpine.js |
| Charts (if needed) | uPlot |

But genuinely start with just htmx + Pico.css and only add when you hit a wall.