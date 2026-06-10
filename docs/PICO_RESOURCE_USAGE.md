# Pico W — Target Resource Usage

## `targets/webserver`

### Flash (embedded content — rodata)

| File | Size | % of 2 MB | % of headroom (~1.35 MB) |
|---|---|---|---|
| `content/index.html` | 18.8 KB (19,272 B) | 0.90% | 1.36% |
| `content/ok.txt` | 1 B | <0.01% | <0.01% |
| `content/status.txt` | 14 B | <0.01% | <0.01% |
| **Total embedded content** | **~18.8 KB** | **0.90%** | **1.36%** |

The HTML file dominates; it includes inline CSS (~3 KB) and inline JS (~6 KB) for the terminal, LED, and servo tabs.

### Static RAM (BSS — application globals)

| Variable | Size | % of 264 KB | % of headroom (~115 KB) |
|---|---|---|---|
| `s_serial_buf[512]` | 512 B | 0.19% | 0.43% |
| `s_recv_snapshot[512]` | 512 B | 0.19% | 0.43% |
| `s_servo_info_buf[64]` | 64 B | 0.02% | 0.05% |
| `s_next_blink` (`absolute_time_t`) | 8 B | <0.01% | <0.01% |
| `s_blink_ms`, `s_led_mode`, lengths, trim | ~24 B | <0.01% | 0.02% |
| `cgi_handlers[3]` (3 × `tCGI`) | ~24 B | <0.01% | 0.02% |
| **Total application globals** | **~1.1 KB** | **0.42%** | **0.96%** |

### Stack

| Context | Peak locals | % of 264 KB | Notes |
|---|---|---|---|
| `main` loop | `cmd_buf[64]` = 64 B | 0.02% | Serial line-editor buffer |
| `cgi_send` (lwIP call) | `decoded[256]` = 256 B | 0.09% | URL-decode scratch buffer |
| `cgi_led` / `cgi_servo` | < 32 B each | <0.01% | Only ints/floats |
| `fs_open_custom` | 0 B | 0% | Pure pointer/memcpy ops |

Deepest call chain: `wifi_poll` → httpd → `cgi_send` → `url_decode`.  
Peak application stack at that point: ~350 B (~0.13% of 264 KB) above the lwIP frame.

### HTTP Activity (steady-state browser open)

| Endpoint | Trigger | Frequency | Payload |
|---|---|---|---|
| `GET /recv.txt` | `setInterval` | every 200 ms | 0–512 B (serial data) |
| `GET /servo_info.txt` | page load | once | ~64 B |
| `GET /send?data=…` | user input | on-demand | `ok.txt` (1 B) |
| `GET /led?…` | button click | on-demand | `ok.txt` (1 B) |
| `GET /servo?…` | slider drag (100 ms debounce) | on-demand | `ok.txt` (1 B) |

The 200 ms `/recv.txt` poll is the highest-frequency HTTP activity; each call does a `memcpy` of up to 512 bytes and resets `s_serial_len`.

### Float Usage

`cgi_servo` uses `atof` and float arithmetic for `deg` and `speed` parameters. These are software-emulated on RP2040. The paths are infrequent (user-driven), so the overhead is acceptable.

---

## `targets/website`

Measured from `build/targets/website/website.elf` (`arm-none-eabi-size` / `nm` on the linked binary), not estimated. The website target is the default flash/deploy target and adds ESC control on top of the webserver's LED + servo + serial terminal.

### Total footprint

| Resource | Used | Budget | % used |
|---|---|---|---|
| **Flash** (`.bin`) | ~392 KB (401,040 B) | 2 MB | **19.1%** |
| **Static RAM** (`.data` + `.bss` + vectors + heap + stack) | ~61.3 KB (62,784 B) | 264 KB | **23.8%** |
| of which `.bss` | 51.2 KB (52,472 B) | — | — |
| of which `.data` | 5.85 KB (5,992 B) | — | — |
| reserved heap / stack | 2 KB / 2 KB | — | — |

`text/data/bss` from `arm-none-eabi-size`: `405136 / 0 / 52696`. Roughly **80% of flash and 76% of RAM remain free.**

### Flash breakdown (where the ~392 KB goes)

| Region | Size | % of 2 MB | Notes |
|---|---|---|---|
| CYW43439 Wi-Fi firmware blob (`w43439A0_..._combined`) | 220 KB (225,240 B) | 10.7% | Fixed cost of Wi-Fi; embedded in `.rodata` |
| `.text` (all code: lwIP, httpd, SDK, libc, app) | 111 KB (113,904 B) | 5.4% | |
| Embedded HTTP content (fsdata) | 40.2 KB (41,162 B) | 2.0% | See per-file table below |
| Other `.rodata` (lwIP tables, libc `_ctype_`, Wi-Fi NVRAM) | ~14.1 KB | 0.7% | |
| `.boot2` + `.binary_info` + `.data` (flash LMA copy) | ~6.1 KB | 0.3% | |

The single CYW43 firmware blob is **more than half the entire binary** — any non-Wi-Fi target is dramatically smaller.

### Embedded content (fsdata — flash rodata)

Sizes are the linked symbols (`makefsdata.py` prepends an HTTP response header to each file, so they exceed the raw byte count):

| Symbol | Linked size | Raw file | % of 2 MB |
|---|---|---|---|
| `data_index_html` | 36.8 KB (37,706 B) | 36.7 KB (37,563 B) | 1.80% |
| `data_logo_svg` | 3.10 KB (3,179 B) | 2.96 KB (3,033 B) | 0.15% |
| `data_empty_txt` | 140 B | 0 B | <0.01% |
| `data_ok_txt` | 137 B | 1 B | <0.01% |
| **Total** | **~40.2 KB (41,162 B)** | — | **2.0%** |

`index.html` dominates (inline CSS + JS for the terminal, LED, servo, and ESC tabs). Note `content/status.txt` exists on disk but is **not** listed in [CMakeLists.txt](../targets/website/CMakeLists.txt) — it is not compiled into the firmware.

### Static RAM breakdown (`.bss`, the 51 KB)

| Symbol | Size | % of 264 KB | Owner |
|---|---|---|---|
| `memp_memory_PBUF_POOL_base` | 35.9 KB (36,771 B) | 13.6% | lwIP packet-buffer pool |
| `cyw43_state` | 2.39 KB (2,448 B) | 0.90% | Wi-Fi driver state |
| `s_slots` | 1.64 KB (1,680 B) | 0.62% | SDK alarm/timer slots |
| `memp_memory_TCP_PCB_base` | 823 B | 0.31% | lwIP TCP control blocks |
| `dns_table` | 1,088 B | 0.41% | lwIP DNS |
| `httpd_req_buf` | 1,024 B | 0.39% | httpd request buffer |
| `hw_endpoints` | 1,024 B | 0.39% | TinyUSB |
| `memp_memory_TCP_SEG_base` | 515 B | 0.19% | lwIP TCP segments |
| **App globals (`website.c`)** | **~1.3 KB** | **0.49%** | see below |

lwIP + CYW43 together account for ~42 KB of the 51 KB `.bss` — the `PBUF_POOL` alone is the largest single allocation on the chip. Application data is a rounding error by comparison.

### Application globals (`website.c` — BSS)

| Variable | Size | % of 264 KB |
|---|---|---|
| `s_recv_snapshot[512+16]` | 528 B | 0.20% |
| `s_serial_buf[512]` | 512 B | 0.19% |
| `s_esc_info_buf[128]` | 128 B | 0.05% |
| `s_servo_info_buf[64]` | 64 B | 0.02% |
| `s_servo_pulse_buf[16]` / `s_esc_pulse_buf[16]` | 32 B | 0.01% |
| `cgi_handlers[4]` (`.rodata`, in flash) | 32 B | <0.01% |
| scalars (`s_led_mode`, `s_blink_ms`, `s_next_blink`, `s_servo_trim_us`, lengths, model ptrs) | ~40 B | 0.02% |
| **Total** | **~1.3 KB** | **0.49%** |

### Stack

| Context | Peak locals | % of 264 KB | Notes |
|---|---|---|---|
| `main` loop | `cmd_buf[64]` = 64 B | 0.02% | Serial line-editor buffer |
| `cgi_send` (lwIP call) | `decoded[256]` = 256 B | 0.10% | URL-decode scratch buffer |
| `cgi_led` / `cgi_servo` / `cgi_esc` | < 32 B each | <0.01% | Ints/floats only |
| `fs_open_custom` | `snprintf` into static bufs | <0.01% | No large locals |

Deepest application chain: `wifi_poll` → httpd → `cgi_send` → `url_decode`, peaking ~350 B above the lwIP frame. The reserved stack is 2 KB; the dominant deep-stack consumer is the lwIP/cyw43 receive path, not application code.

### HTTP activity (steady-state, browser open)

| Endpoint | Trigger | Payload |
|---|---|---|
| `GET /recv.txt` | poll (terminal) | 0–528 B serial snapshot; resets `s_serial_len` |
| `GET /servo_pulse.txt` / `/esc_pulse.txt` | poll (live pulse readout) | ~16 B each |
| `GET /servo_info.txt` / `/esc_info.txt` | page load | 64 B / 128 B |
| `GET /send?data=…` | terminal input | `ok.txt` |
| `GET /led?…` / `/servo?…` / `/esc?…` | button/slider | `ok.txt` |

Each poll regenerates its response into a static buffer via `snprintf` inside `fs_open_custom` — no heap allocation, no per-request flash access.

### Headroom

With ~1.6 MB flash and ~200 KB RAM free, the binding constraints are not application data but the fixed lwIP/CYW43 overhead. Growth budget: embedded HTML/JS can expand by an order of magnitude in flash, and the lwIP `PBUF_POOL` (35.9 KB) is the lever to pull if RAM ever tightens — it is sized by `lwipopts.h`, not by application code.
