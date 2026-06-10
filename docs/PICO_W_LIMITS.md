# Raspberry Pi Pico W — Compute & Storage Limits

## MCU

| Property | Value |
|---|---|
| Chip | RP2040 |
| Cores | 2× Arm Cortex-M0+ |
| Clock | 125 MHz (default) |
| FPU | None — float is software-emulated (~10–20× slower than int) |

## Memory

| Resource | Size |
|---|---|
| SRAM | 264 KB (6 banks) |
| Flash | 2 MB external QSPI |

## RAM Budget (Wi-Fi + lwIP httpd) — total: 264 KB

| Component | Approx usage | % of 264 KB |
|---|---|---|
| lwIP stack + buffers | ~40–60 KB | ~19% |
| CYW43 driver | ~30 KB | ~11% |
| httpd + CGI/SSI | ~8–16 KB | ~5% |
| Stack (2 cores) | ~8 KB each | ~6% |
| Application headroom | ~100–130 KB | ~44% |

Avoid large stack allocations, heap-heavy patterns, or big static arrays.

## Flash Budget — total: 2 MB

| Component | Approx size | % of 2 MB |
|---|---|---|
| SDK + CYW43 firmware | ~300–400 KB | ~17% |
| lwIP | ~100–200 KB | ~7% |
| Application headroom | ~1.2–1.5 MB | ~66% |

HTML/JS embedded via `pico_set_lwip_httpd_content` lives in rodata — keep content files small.

## Compute Constraints

- **No hardware FPU** — avoid float in hot paths (CGI handlers, main loop).
- **No OS** — bare metal, single-threaded poll loop. Any blocking call stalls Wi-Fi, HTTP, and serial.
- **`wifi_poll()` must be called every loop iteration** — gaps longer than a few ms cause TCP timeouts or dropped packets.
- **125 MHz** is sufficient for simple HTTP serving + servo control, but not signal processing or heavy math.

## lwIP httpd Limits

- CGI handlers return a **filename** (served from rodata); they cannot write a response body directly.
- Use `fs_open_custom` for runtime-generated content (dynamic sensor data, status).
- No persistent connections or WebSockets without significant custom work.
- SSI buffer sizes are set in `lwipopts.h`; keep dynamic responses short.
- HTTP response bodies must fit in lwIP pbuf chains.

## Design Rules

- Keep HTML/JS small — it is rodata baked into the binary.
- Dynamic data → `fs_open_custom` or SSI in `.shtml` files, not CGI body generation.
- No heap allocation in CGI/SSI handlers — use static or stack buffers.
- Never block the main loop for more than a few ms (no `sleep_ms` in loop body).
