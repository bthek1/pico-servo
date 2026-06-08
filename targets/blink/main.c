#include "led.h"
#include "serial.h"

#define LED_PIN  25
#define BLINK_MS 500

int main() {
    serial_init();
    led_init(LED_PIN);

    serial_println("blink start");

    while (true) {
        led_toggle(LED_PIN);
        sleep_ms(BLINK_MS);
    }
}
