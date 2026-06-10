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
