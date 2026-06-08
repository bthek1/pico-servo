#include "servo.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// 125 MHz / (100 * 25000) = 50 Hz
#define SERVO_CLKDIV   100.0f
#define SERVO_WRAP     24999
#define SERVO_PERIOD   20000  // µs

void servo_init(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice, SERVO_CLKDIV);
    pwm_set_wrap(slice, SERVO_WRAP);
    pwm_set_enabled(slice, true);
}

void servo_set_us(uint gpio, uint16_t pulse_us) {
    uint16_t level = (uint16_t)((uint32_t)pulse_us * (SERVO_WRAP + 1) / SERVO_PERIOD);
    pwm_set_gpio_level(gpio, level);
}

void servo_set_deg(uint gpio, float degrees) {
    if (degrees < 0.0f) degrees = 0.0f;
    if (degrees > 180.0f) degrees = 180.0f;
    uint16_t pulse_us = (uint16_t)(1000.0f + degrees / 180.0f * 1000.0f);
    servo_set_us(gpio, pulse_us);
}
