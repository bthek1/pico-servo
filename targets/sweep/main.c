#include "pico/stdlib.h"
#include "servo.h"

#define SERVO_PIN  15
#define STEP_DEG   1.0f
#define STEP_MS    10

int main() {
    stdio_init_all();
    servo_init(SERVO_PIN);

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
