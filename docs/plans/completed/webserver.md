# Plan: Serve HTML/JS Page from Pico W

## Status: complete ✓

## Goal

Add a `webserver` target that connects to Wi-Fi and serves a static HTML/JS page using
lwIP's built-in HTTP server (`pico_lwip_http`). The page is embedded directly in the
firmware binary — no SD card or filesystem required.

## Approach

Use `lwip/apps/httpd` (already in the SDK) with the `pico_set_lwip_httpd_content()` CMake
helper, which compiles content files into a `fsdata.c` struct at build time. The existing
`wifi` library is unchanged; the webserver is a separate target.

## File Layout (implemented)

```
targets/webserver/
├── CMakeLists.txt          ✓
├── webserver.c             ✓  wifi connect + httpd_init + CGI + poll loop
└── content/
    ├── index.html          ✓  polls /status every 2 s via JS fetch
    └── status.txt          ✓  served by CGI handler at GET /status
```

## Steps

- [x] **Step 1** — `lib/wifi/lwipopts.h`: added HTTPD options (`LWIP_HTTPD`, `LWIP_HTTPD_DYNAMIC_HEADERS`, `LWIP_HTTPD_CGI`, `LWIP_HTTPD_SSI`, tag lengths)
- [x] **Step 2** — `targets/webserver/content/index.html`: self-contained page; JS polls `/status` every 2 s via `fetch` + `setInterval`
- [x] **Step 3** — `targets/webserver/content/status.txt`: static response body for `/status` CGI endpoint
- [x] **Step 4** — `targets/webserver/webserver.c`: wifi connect → `httpd_init()` → CGI handler → `wifi_poll()` loop; logs IP to serial on boot
- [x] **Step 5** — `targets/webserver/CMakeLists.txt`: links `wifi` + `serial` + `pico_lwip_http` + `webserver_content`; uses `pico_set_lwip_httpd_content()` for `index.html` and `status.txt`
- [x] **Step 6** — root `CMakeLists.txt`: `add_subdirectory(targets/webserver)` added
- [x] **Step 7** — build & flash: `just deploy webserver`; verified — `index.html` served at `/`, `/status` returns `Pico W online`
- [x] **Fix** — added `#define HTTPD_FSDATA_FILE "pico_fsdata.inc"` to `lwipopts.h`; without it lwIP ignores the generated fsdata and falls back to its built-in demo page

## Deploy

```bash
just deploy webserver
# open http://<ip-shown-in-serial>/ in a browser
```

## Extending Later

| Feature | What to add |
|---------|-------------|
| Servo control from page | CGI handler that calls `servo_set_deg()`; JS button POSTs to `/servo.cgi` |
| Dynamic status (uptime/IP) | Set `LWIP_HTTPD_SSI 1`; rename to `.shtml`; register `http_set_ssi_handler` |
| mDNS (`pico.local`) | Link `pico_lwip_mdns`; call `mdns_resp_init()` after `httpd_init()` |
| Multiple pages | Add more files to `pico_set_lwip_httpd_content()` |
