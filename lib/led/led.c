#include "led.h"
#include "pico/stdlib.h"

#ifdef PICO_CYW43_SUPPORTED
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

#else

void led_init(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
}

void led_on(uint gpio)     { gpio_put(gpio, 1); }
void led_off(uint gpio)    { gpio_put(gpio, 0); }
void led_toggle(uint gpio) { gpio_put(gpio, !gpio_get(gpio)); }

#endif
