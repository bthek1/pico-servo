#pragma once
#include "pico/stdlib.h"

typedef enum {
    SERVO_POSITIONAL,   // angle control 0–180°
    SERVO_CONTINUOUS,   // speed control; stop_us = stop point
} servo_type_t;

typedef struct {
    servo_type_t type;
    uint16_t     min_us;   // pulse for 0° or full-reverse
    uint16_t     max_us;   // pulse for 180° or full-forward
    uint16_t     stop_us;  // safe neutral (positional: centre, continuous: stop)
} servo_config_t;

// Pre-defined model presets
extern const servo_config_t SERVO_SER0006;  // DFRobot 9g positional, 500–2500 µs
extern const servo_config_t SERVO_SG92R;    // TowerPro 9g positional, 500–2500 µs
extern const servo_config_t SERVO_MG996R;   // TowerPro/Olimex continuous rotation, 1000–2000 µs

// Initialise PWM and attach a model config to this GPIO slot
void servo_init_config(uint gpio, const servo_config_t *cfg);
// Backward-compat shim: uses SERVO_SER0006
void servo_init(uint gpio);

// Raw pulse — clamped to [cfg.min_us, cfg.max_us]
void servo_set_us(uint gpio, uint16_t pulse_us);
// Positional only: 0–180°; no-op on SERVO_CONTINUOUS
void servo_set_deg(uint gpio, float degrees);
// Continuous only: -1.0 = full reverse, 0.0 = stop, +1.0 = full forward; no-op on SERVO_POSITIONAL
void servo_set_speed(uint gpio, float speed);
// Continuous only: update the stop pulse for trim calibration
void servo_set_stop_us(uint gpio, uint16_t stop_us);
// Safe stop: sends stop_us regardless of type
void servo_safe_stop(uint gpio);
// Returns the last pulse width actually sent (µs), or 0 if uninitialised
uint16_t servo_get_us(uint gpio);
