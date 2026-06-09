#pragma once
#include "pico/stdlib.h"

void servo_init(uint gpio);
void servo_set_us(uint gpio, uint16_t pulse_us);  // 500–2500 µs
void servo_set_deg(uint gpio, float degrees);      // 0–180°
