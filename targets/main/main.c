#include "led.h"
#include "serial.h"
#include "pico/stdlib.h"

#define LED_PIN    25
#define BLINK_MS   500
#define REPORT_MS  1000

int main() {
    serial_init();
    led_init(LED_PIN);

    serial_println("ready");

    absolute_time_t next_blink  = make_timeout_time_ms(BLINK_MS);
    absolute_time_t next_report = make_timeout_time_ms(REPORT_MS);
    uint32_t uptime_s = 0;

    while (true) {
        if (time_reached(next_blink)) {
            led_toggle(LED_PIN);
            next_blink = make_timeout_time_ms(BLINK_MS);
        }

        if (time_reached(next_report)) {
            uptime_s++;
            serial_println("uptime: %lu s", uptime_s);
            next_report = make_timeout_time_ms(REPORT_MS);
        }

        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            putchar(c);
        }
    }
}
