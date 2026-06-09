#include "wifi.h"
#include "serial.h"
#include "servo.h"
#include "secrets.h"
#include "pico/stdlib.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVO_GPIO    0
#define SERIAL_BUF_SZ 512

static const servo_config_t *s_servo_model   = &SERVO_SER0006;
static int16_t               s_servo_trim_us = 0;

static char s_serial_buf[SERIAL_BUF_SZ];
static int  s_serial_len = 0;

static char s_recv_snapshot[SERIAL_BUF_SZ];
static int  s_recv_snapshot_len = 0;

static char s_servo_info_buf[64];
static int  s_servo_info_len = 0;

// --- LED state machine ---------------------------------------------------

typedef enum { LED_OFF, LED_ON, LED_BLINK } led_mode_t;

static led_mode_t      s_led_mode  = LED_BLINK;
static uint32_t        s_blink_ms  = 100;
static absolute_time_t s_next_blink;

// --- URL decode ----------------------------------------------------------

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

// --- CGI: /send?data=<text> ----------------------------------------------

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

// --- CGI: /led?state=on|off|blink&ms=<n> ---------------------------------

static const char *cgi_led(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    (void)iIndex;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "state") == 0) {
            if      (strcmp(pcValue[i], "on")    == 0) s_led_mode = LED_ON;
            else if (strcmp(pcValue[i], "off")   == 0) s_led_mode = LED_OFF;
            else if (strcmp(pcValue[i], "blink") == 0) s_led_mode = LED_BLINK;
        } else if (strcmp(pcParam[i], "ms") == 0) {
            int ms = atoi(pcValue[i]);
            if (ms >= 50 && ms <= 5000) s_blink_ms = (uint32_t)ms;
        }
    }
    if (s_led_mode == LED_ON)  wifi_led_on();
    if (s_led_mode == LED_OFF) wifi_led_off();
    return "/ok.txt";
}

// --- CGI: /servo?pulse=<µs> | ?deg=<0-180> | ?speed=<-100..100> | ?trim=<-50..50> | ?stop

static const char *cgi_servo(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    (void)iIndex;
    bool do_stop = false;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "pulse") == 0) {
            int pulse = atoi(pcValue[i]);
            if (pulse > 0) servo_set_us(SERVO_GPIO, (uint16_t)pulse);
        } else if (strcmp(pcParam[i], "deg") == 0) {
            float deg = (float)atof(pcValue[i]);
            if (deg < 0.0f)   deg = 0.0f;
            if (deg > 180.0f) deg = 180.0f;
            servo_set_deg(SERVO_GPIO, deg);
        } else if (strcmp(pcParam[i], "speed") == 0) {
            float speed = (float)atof(pcValue[i]) / 100.0f;
            servo_set_speed(SERVO_GPIO, speed);
        } else if (strcmp(pcParam[i], "trim") == 0) {
            int trim = atoi(pcValue[i]);
            if (trim < -50) trim = -50;
            if (trim >  50) trim =  50;
            s_servo_trim_us = (int16_t)trim;
            servo_set_stop_us(SERVO_GPIO, (uint16_t)((int)s_servo_model->stop_us + trim));
        } else if (strcmp(pcParam[i], "stop") == 0) {
            do_stop = true;
        }
    }
    if (do_stop) servo_safe_stop(SERVO_GPIO);
    return "/ok.txt";
}

static const tCGI cgi_handlers[] = {
    { "/send",  cgi_send  },
    { "/led",   cgi_led   },
    { "/servo", cgi_servo },
};

// --- Custom fs: /recv.txt ------------------------------------------------

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
    if (strcmp(name, "/servo_info.txt") == 0) {
        s_servo_info_len = snprintf(s_servo_info_buf, sizeof(s_servo_info_buf),
            "type=%s\nmin_us=%d\nmax_us=%d\nstop_us=%d\n",
            s_servo_model->type == SERVO_CONTINUOUS ? "continuous" : "positional",
            (int)s_servo_model->min_us,
            (int)s_servo_model->max_us,
            (int)s_servo_model->stop_us + s_servo_trim_us);
        file->data  = s_servo_info_buf;
        file->len   = s_servo_info_len;
        file->index = s_servo_info_len;
        file->flags = 0;
        return 1;
    }
    return 0;
}

void fs_close_custom(struct fs_file *file) { (void)file; }

// --- Main ----------------------------------------------------------------

int main() {
    serial_init();
    servo_init_config(SERVO_GPIO, s_servo_model);
    servo_safe_stop(SERVO_GPIO);

    serial_println("connecting to %s ...", WIFI_SSID);
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) != WIFI_OK) {
        serial_println("wifi failed");
        return 1;
    }
    wifi_set_static_ip("192.168.2.55", "255.255.255.0", "192.168.2.1");
    serial_println("ip: %s", wifi_get_ip());

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, 3);
    serial_println("http ready at http://%s/", wifi_get_ip());

    s_next_blink = make_timeout_time_ms(s_blink_ms);

    char cmd_buf[64];
    int  cmd_len = 0;

    while (true) {
        wifi_poll();

        if (s_led_mode == LED_BLINK && time_reached(s_next_blink)) {
            wifi_led_toggle();
            s_next_blink = make_timeout_time_ms(s_blink_ms);
        }

        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) continue;

        if (c == '\r' || c == '\n') {
            if (cmd_len > 0) {
                cmd_buf[cmd_len] = '\0';
                if (cmd_len == 2 &&
                    (cmd_buf[0] == 'i' || cmd_buf[0] == 'I') &&
                    (cmd_buf[1] == 'p' || cmd_buf[1] == 'P')) {
                    serial_println("ip: %s", wifi_get_ip());
                } else {
                    for (int i = 0; i < cmd_len && s_serial_len < SERIAL_BUF_SZ - 1; i++)
                        s_serial_buf[s_serial_len++] = cmd_buf[i];
                    if (s_serial_len < SERIAL_BUF_SZ - 1)
                        s_serial_buf[s_serial_len++] = '\n';
                }
                cmd_len = 0;
            }
        } else if (cmd_len < (int)sizeof(cmd_buf) - 1) {
            cmd_buf[cmd_len++] = (char)c;
        }
    }
}
