#include "led.h"

void led_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
}

void led_on(uint gpio)     { gpio_put(gpio, 1); }
void led_off(uint gpio)    { gpio_put(gpio, 0); }
void led_toggle(uint gpio) { gpio_put(gpio, !gpio_get(gpio)); }
