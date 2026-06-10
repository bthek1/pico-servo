#pragma once
#include "pico/stdlib.h"

typedef enum {
    ESC_UNIDIRECTIONAL,   // 1000 = off, 2000 = full throttle
    ESC_BIDIRECTIONAL,    // 1000 = full reverse, 1500 = stop, 2000 = full forward
} esc_type_t;

typedef enum {
    ESC_STATE_DISARMED,
    ESC_STATE_ARMING,     // holding min throttle during arm window
    ESC_STATE_ARMED,
    ESC_STATE_CAL_HIGH,   // calibration: holding max_us, waiting for user power-cycle
    ESC_STATE_CAL_LOW,    // calibration: holding min_us for 2 s, then → DISARMED
} esc_state_t;

typedef struct {
    esc_type_t type;
    uint16_t   min_us;       // 1000
    uint16_t   max_us;       // 2000
    uint16_t   neutral_us;   // 1000 (unidirectional) or 1500 (bidirectional)
    uint16_t   deadband_us;  // µs each side of neutral (bidirectional), typically 25
    uint32_t   arm_ms;       // arm hold duration, typically 3000
} esc_config_t;

extern const esc_config_t ESC_STANDARD;  // unidirectional, 1000–2000 µs, 3 s arm
extern const esc_config_t ESC_BIDIR;     // bidirectional, 1000–2000 µs, 1500 stop, 25 µs deadband

void esc_init(uint gpio, const esc_config_t *cfg);

// Non-blocking — call esc_update() each main loop iteration
void        esc_arm(uint gpio);
void        esc_disarm(uint gpio);
esc_state_t esc_get_state(uint gpio);

// Drives arm timer, 500 ms failsafe, and calibration completion; call every loop
void esc_update(uint gpio);

// No-op unless state is ARMED
void esc_set_throttle(uint gpio, float throttle);  // unidirectional: 0.0–1.0
void esc_set_speed(uint gpio, float speed);         // bidirectional: -1.0–+1.0
void esc_brake(uint gpio);                          // sends neutral_us
void esc_set_us(uint gpio, uint16_t pulse_us);      // raw; clamped to [min_us, max_us]
uint16_t esc_get_us(uint gpio);                     // last pulse actually sent (µs), including failsafe

// Throttle-range calibration (only from DISARMED)
// Step 1: holds max_us — power-cycle the ESC, wait for beeps, then call step 2
void esc_calibrate_start(uint gpio);
// Step 2: holds min_us for 2 s, then auto-returns to DISARMED
void esc_calibrate_low(uint gpio);
// Cancel calibration mid-sequence (returns to DISARMED, sends min_us)
void esc_calibrate_cancel(uint gpio);
