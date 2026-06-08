#include "led.h"
#include "pico/cyw43_arch.h"

void led_init(uint gpio) {
    (void)gpio;
    cyw43_arch_init();
}

void led_on(uint gpio)     { (void)gpio; cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); }
void led_off(uint gpio)    { (void)gpio; cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); }
void led_toggle(uint gpio) {
    (void)gpio;
    static bool state = false;
    state = !state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
}
