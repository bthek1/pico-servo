#pragma once
#include "pico/stdlib.h"

void led_init(uint gpio);
void led_on(uint gpio);
void led_off(uint gpio);
void led_toggle(uint gpio);
