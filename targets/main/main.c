#include "wifi.h"
#include "serial.h"
#include "secrets.h"
#include "pico/stdlib.h"

#define BLINK_MS   100
#define REPORT_MS  5000

int main() {
    serial_init();

    serial_println("connecting to %s ...", WIFI_SSID);
    wifi_result_t res = wifi_connect(WIFI_SSID, WIFI_PASSWORD);
    if (res == WIFI_OK) {
        serial_println("connected: %s", wifi_get_ip());
    } else {
        serial_println("wifi connect failed");
    }

    absolute_time_t next_blink  = make_timeout_time_ms(BLINK_MS);
    absolute_time_t next_report = make_timeout_time_ms(REPORT_MS);
    uint32_t uptime_s = 0;

    while (true) {
        wifi_poll();

        if (time_reached(next_blink)) {
            wifi_led_toggle();
            next_blink = make_timeout_time_ms(BLINK_MS);
        }

        if (time_reached(next_report)) {
            uptime_s += REPORT_MS / 1000;
            serial_println("uptime: %lu s | ip: %s", uptime_s, wifi_get_ip());
            next_report = make_timeout_time_ms(REPORT_MS);
        }
    }
}
