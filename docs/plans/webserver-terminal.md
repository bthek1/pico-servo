# Plan: Webserver Terminal — Bidirectional Serial Bridge

## Goal

Update `targets/webserver` to:
- Blink LED at 100 ms
- Serve a terminal UI in the browser
- Forward browser input → USB serial (print to serial monitor)
- Forward USB serial input → browser (display in terminal)
- Use `serial_println` for all logging

## Architecture

```
Browser                     Pico W
  |                            |
  |  GET /recv.txt (poll 200ms)|
  |<---------------------------|  ring buffer → HTTP response
  |                            |
  |  GET /send?data=hello      |
  |--------------------------->|  URL-decode → putchar to serial
  |<-- ok.txt (static "OK") --|
```

**Serial → browser**: main loop reads USB serial with `getchar_timeout_us(0)` into a
512-byte ring buffer. JS polls `GET /recv.txt` every 200 ms; the response is the buffered
content, which is then cleared. Served via `fs_open_custom` (lwIP custom file hook).

**Browser → serial**: CGI handler for `/send` reads the `data` query param (URL-decoded)
and calls `putchar()` for each character. Returns `/ok.txt` (static empty response).

**No SSI, no WebSockets** — plain HTTP polling is sufficient and fits lwIP poll mode.

## Files Changed

| File | Change |
|------|--------|
| `lib/wifi/lwipopts.h` | add `LWIP_HTTPD_CUSTOM_FILES 1` |
| `targets/webserver/webserver.c` | full rewrite |
| `targets/webserver/content/index.html` | terminal UI |
| `targets/webserver/content/ok.txt` | static empty response for `/send` CGI |
| `targets/webserver/CMakeLists.txt` | add `ok.txt` to content list |

## Step 1 — `lib/wifi/lwipopts.h`

Add under the httpd block:

```c
#define LWIP_HTTPD_CUSTOM_FILES         1
```

This makes lwIP call `fs_open_custom(file, name)` before falling back to fsdata.
`FS_FILE_FLAGS_CUSTOM` is set automatically by `fs_open` when our hook returns 1.

## Step 2 — `targets/webserver/content/index.html`

Self-contained terminal (inline CSS + JS, no external deps):

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Pico W Terminal</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { background: #1e1e1e; color: #d4d4d4; font-family: monospace;
           display: flex; flex-direction: column; height: 100vh; padding: 1rem; gap: .5rem; }
    h2 { color: #c62828; font-size: 1rem; }
    #output { flex: 1; background: #111; border: 1px solid #333; padding: .5rem;
              overflow-y: auto; white-space: pre-wrap; font-size: .85rem; }
    #input-row { display: flex; gap: .5rem; }
    #input { flex: 1; background: #111; color: #d4d4d4; border: 1px solid #444;
             padding: .4rem .6rem; font-family: monospace; font-size: .85rem; }
    button { background: #c62828; color: #fff; border: none; padding: .4rem .8rem; cursor: pointer; }
  </style>
</head>
<body>
  <h2>Pico W Terminal</h2>
  <div id="output"></div>
  <div id="input-row">
    <input id="input" type="text" placeholder="type and press Enter…" autocomplete="off">
    <button onclick="send()">Send</button>
  </div>
  <script>
    const out = document.getElementById('output');
    const inp = document.getElementById('input');

    function append(text) {
      out.textContent += text;
      out.scrollTop = out.scrollHeight;
    }

    function send() {
      const text = inp.value;
      if (!text) return;
      inp.value = '';
      append('> ' + text + '\n');
      fetch('/send?data=' + encodeURIComponent(text))
        .catch(() => append('[send failed]\n'));
    }

    inp.addEventListener('keydown', e => { if (e.key === 'Enter') send(); });

    // poll serial output every 200 ms
    setInterval(() => {
      fetch('/recv.txt')
        .then(r => r.text())
        .then(t => { if (t) append(t); })
        .catch(() => {});
    }, 200);
  </script>
</body>
</html>
```

## Step 3 — `targets/webserver/content/ok.txt`

Empty file — the body of the `/send` CGI response. The browser ignores it.

```
(empty)
```

## Step 4 — `targets/webserver/webserver.c`

```c
#include "wifi.h"
#include "serial.h"
#include "secrets.h"
#include "pico/stdlib.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include <string.h>
#include <stdio.h>

#define BLINK_MS       100
#define SERIAL_BUF_SZ  512

// Ring buffer for USB serial → browser
static char  s_serial_buf[SERIAL_BUF_SZ];
static int   s_serial_len = 0;

// Snapshot buffer used during an active /recv.txt HTTP response
static char  s_recv_snapshot[SERIAL_BUF_SZ];
static int   s_recv_snapshot_len = 0;

// --- URL decode -------------------------------------------------------
static char url_decode_char(const char *s) {
    if (*s == '+') return ' ';
    if (*s == '%' && s[1] && s[2]) {
        char hi = s[1], lo = s[2];
        hi = (hi >= 'a') ? hi - 'a' + 10 : (hi >= 'A') ? hi - 'A' + 10 : hi - '0';
        lo = (lo >= 'a') ? lo - 'a' + 10 : (lo >= 'A') ? lo - 'A' + 10 : lo - '0';
        return (char)((hi << 4) | lo);
    }
    return *s;
}

static void url_decode(const char *src, char *dst, size_t dst_sz) {
    size_t i = 0;
    while (*src && i + 1 < dst_sz) {
        dst[i++] = url_decode_char(src);
        src += (*src == '%' && src[1] && src[2]) ? 3 : 1;
    }
    dst[i] = '\0';
}

// --- CGI: /send?data=<text> -------------------------------------------
static const char *cgi_send(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    (void)iIndex;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "data") == 0) {
            char decoded[256];
            url_decode(pcValue[i], decoded, sizeof(decoded));
            printf("%s\n", decoded);   // echo to USB serial
            break;
        }
    }
    return "/ok.txt";
}

static const tCGI cgi_handlers[] = {
    { "/send", cgi_send },
};

// --- Custom fs: /recv.txt returns buffered serial data ----------------
int fs_open_custom(struct fs_file *file, const char *name) {
    if (strcmp(name, "/recv.txt") == 0) {
        s_recv_snapshot_len = s_serial_len;
        memcpy(s_recv_snapshot, s_serial_buf, s_serial_len);
        s_serial_len = 0;               // clear ring buffer

        file->data  = s_recv_snapshot;
        file->len   = s_recv_snapshot_len;
        file->index = s_recv_snapshot_len;
        file->flags = 0;                // httpd generates Content-Type: text/plain
        return 1;
    }
    return 0;
}

void fs_close_custom(struct fs_file *file) { (void)file; }

// --- Main -------------------------------------------------------------
int main() {
    serial_init();

    serial_println("connecting to %s ...", WIFI_SSID);
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) != WIFI_OK) {
        serial_println("wifi failed");
        return 1;
    }
    serial_println("ip: %s", wifi_get_ip());

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, 1);
    serial_println("http ready at http://%s/", wifi_get_ip());

    absolute_time_t next_blink = make_timeout_time_ms(BLINK_MS);

    while (true) {
        wifi_poll();

        // LED blink
        if (time_reached(next_blink)) {
            wifi_led_toggle();
            next_blink = make_timeout_time_ms(BLINK_MS);
        }

        // USB serial → browser buffer
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT && s_serial_len < SERIAL_BUF_SZ - 1) {
            s_serial_buf[s_serial_len++] = (char)c;
        }
    }
}
```

> `printf` writes to USB serial (same path as `serial_println`). `serial_println` is used
> for logging (connection status, IP). `printf` is used for the terminal echo because it
> avoids the `[log]` prefix — keep them semantically separate if desired.

## Step 5 — `targets/webserver/CMakeLists.txt`

Add `ok.txt` to the content list:

```cmake
pico_set_lwip_httpd_content(webserver_content INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/content/index.html
    ${CMAKE_CURRENT_LIST_DIR}/content/ok.txt
)
```

Remove `status.txt` (no longer needed).

## Step 6 — Build & Flash

```bash
just deploy webserver
# open http://<ip>/ — type in the input box and press Enter
# chars typed in serial monitor appear in the browser terminal
```

## Notes & Constraints

- `s_recv_snapshot` is a static buffer — safe for lwIP to read after `fs_open_custom` returns
  because lwIP reads it synchronously in poll mode before any other request can preempt.
- If the browser polls while serial is being written, the worst case is one poll cycle of data loss —
  acceptable for a debug terminal.
- The 512-byte buffer will drop chars if the serial monitor floods faster than the browser polls.
  Increase `SERIAL_BUF_SZ` if needed.
- `file->index = file->len` on open tells lwIP the file is pre-loaded (no dynamic reads needed).

## Extending Later

| Feature | What to add |
|---------|-------------|
| Line-buffered input | Accumulate chars until `\n`, then add to browser buffer as a complete line |
| Newline in browser input | Send `\r\n` for terminals that expect it |
| Multiple concurrent clients | Add a mutex around `s_serial_buf` (not needed in single-core poll mode) |
| Servo control endpoint | Add second CGI handler `/servo?deg=90` that calls `servo_set_deg()` |
