#include "wifi.h"
#include "serial.h"
#include "secrets.h"
#include "pico/stdlib.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include <string.h>
#include <stdio.h>

#define BLINK_MS      100
#define SERIAL_BUF_SZ 512

static char s_serial_buf[SERIAL_BUF_SZ];
static int  s_serial_len = 0;

static char s_recv_snapshot[SERIAL_BUF_SZ];
static int  s_recv_snapshot_len = 0;

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
            printf("%s\n", decoded);
            break;
        }
    }
    return "/ok.txt";
}

static const tCGI cgi_handlers[] = {
    { "/send", cgi_send },
};

// --- Custom fs: /recv.txt ---------------------------------------------

int fs_open_custom(struct fs_file *file, const char *name) {
    if (strcmp(name, "/recv.txt") == 0) {
        s_recv_snapshot_len = s_serial_len;
        memcpy(s_recv_snapshot, s_serial_buf, s_serial_len);
        s_serial_len = 0;

        file->data  = s_recv_snapshot;
        file->len   = s_recv_snapshot_len;
        file->index = s_recv_snapshot_len;
        file->flags = 0;
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

        if (time_reached(next_blink)) {
            wifi_led_toggle();
            next_blink = make_timeout_time_ms(BLINK_MS);
        }

        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT && s_serial_len < SERIAL_BUF_SZ - 1) {
            s_serial_buf[s_serial_len++] = (char)c;
        }
    }
}
