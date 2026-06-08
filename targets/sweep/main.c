#include "led.h"
#include "serial.h"
#include "servo.h"

#define SERVO_PIN  15
#define LED_PIN    25
#define STEP_DEG   1.0f
#define STEP_MS    10

int main() {
    serial_init();
    led_init(LED_PIN);
    servo_init(SERVO_PIN);

    serial_println("sweep start");
    led_on(LED_PIN);

    float deg = 0.0f;
    float dir = 1.0f;

    while (true) {
        servo_set_deg(SERVO_PIN, deg);
        sleep_ms(STEP_MS);

        deg += dir * STEP_DEG;
        if (deg >= 180.0f) { deg = 180.0f; dir = -1.0f; }
        if (deg <= 0.0f)   { deg = 0.0f;   dir =  1.0f; }
    }
}
