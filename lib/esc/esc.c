#include "esc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// 125 MHz / (100 * 25000) = 50 Hz  (same timing as lib/servo)
#define ESC_CLKDIV   100.0f
#define ESC_WRAP     24999
#define ESC_PERIOD   20000   // µs
#define MAX_GPIO     30
#define FAILSAFE_US  500000  // 500 ms in µs

const esc_config_t ESC_STANDARD = { ESC_UNIDIRECTIONAL, 1000, 2000, 1000,  0, 3000 };
const esc_config_t ESC_BIDIR    = { ESC_BIDIRECTIONAL,  1000, 2000, 1500, 25, 3000 };

typedef struct {
    esc_config_t    cfg;
    esc_state_t     state;
    absolute_time_t arm_deadline;
    absolute_time_t last_cmd;
    uint16_t        last_us;
    bool            inited;
} esc_slot_t;

static esc_slot_t s_slots[MAX_GPIO];

static void raw_pulse(uint gpio, uint16_t pulse_us) {
    uint16_t level = (uint16_t)((uint32_t)pulse_us * (ESC_WRAP + 1) / ESC_PERIOD);
    pwm_set_gpio_level(gpio, level);
    if (gpio < MAX_GPIO) s_slots[gpio].last_us = pulse_us;
}

uint16_t esc_get_us(uint gpio) {
    esc_slot_t *s = get_slot(gpio);
    return s ? s->last_us : 0;
}

void esc_init(uint gpio, const esc_config_t *cfg) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice, ESC_CLKDIV);
    pwm_set_wrap(slice, ESC_WRAP);
    pwm_set_enabled(slice, true);
    if (gpio < MAX_GPIO) {
        s_slots[gpio].cfg    = *cfg;
        s_slots[gpio].state  = ESC_STATE_DISARMED;
        s_slots[gpio].inited = true;
    }
    raw_pulse(gpio, cfg->min_us);
}

static esc_slot_t *get_slot(uint gpio) {
    return (gpio < MAX_GPIO && s_slots[gpio].inited) ? &s_slots[gpio] : NULL;
}

void esc_arm(uint gpio) {
    esc_slot_t *s = get_slot(gpio);
    if (!s || s->state != ESC_STATE_DISARMED) return;
    s->state        = ESC_STATE_ARMING;
    s->arm_deadline = make_timeout_time_ms(s->cfg.arm_ms);
    raw_pulse(gpio, s->cfg.min_us);
}

void esc_disarm(uint gpio) {
    esc_slot_t *s = get_slot(gpio);
    if (!s) return;
    s->state = ESC_STATE_DISARMED;
    raw_pulse(gpio, s->cfg.min_us);
}

esc_state_t esc_get_state(uint gpio) {
    esc_slot_t *s = get_slot(gpio);
    return s ? s->state : ESC_STATE_DISARMED;
}

void esc_update(uint gpio) {
    esc_slot_t *s = get_slot(gpio);
    if (!s) return;

    if (s->state == ESC_STATE_ARMING) {
        raw_pulse(gpio, s->cfg.min_us);
        if (time_reached(s->arm_deadline)) {
            s->state    = ESC_STATE_ARMED;
            s->last_cmd = get_absolute_time();
        }
        return;
    }

    if (s->state == ESC_STATE_ARMED) {
        int64_t elapsed = absolute_time_diff_us(s->last_cmd, get_absolute_time());
        if (elapsed > FAILSAFE_US)
            raw_pulse(gpio, s->cfg.neutral_us);
    }
}

void esc_set_us(uint gpio, uint16_t pulse_us) {
    esc_slot_t *s = get_slot(gpio);
    if (!s || s->state != ESC_STATE_ARMED) return;
    if (pulse_us < s->cfg.min_us) pulse_us = s->cfg.min_us;
    if (pulse_us > s->cfg.max_us) pulse_us = s->cfg.max_us;
    s->last_cmd = get_absolute_time();
    raw_pulse(gpio, pulse_us);
}

void esc_set_throttle(uint gpio, float throttle) {
    esc_slot_t *s = get_slot(gpio);
    if (!s || s->cfg.type != ESC_UNIDIRECTIONAL) return;
    if (throttle < 0.0f) throttle = 0.0f;
    if (throttle > 1.0f) throttle = 1.0f;
    uint16_t pulse = (uint16_t)(s->cfg.min_us + throttle * (float)(s->cfg.max_us - s->cfg.min_us));
    esc_set_us(gpio, pulse);
}

void esc_set_speed(uint gpio, float speed) {
    esc_slot_t *s = get_slot(gpio);
    if (!s || s->cfg.type != ESC_BIDIRECTIONAL) return;
    if (speed < -1.0f) speed = -1.0f;
    if (speed >  1.0f) speed =  1.0f;
    int32_t half  = (int32_t)(s->cfg.max_us - s->cfg.neutral_us);
    int32_t pulse = (int32_t)s->cfg.neutral_us + (int32_t)(speed * (float)half);
    // Apply deadband
    int32_t diff = pulse - (int32_t)s->cfg.neutral_us;
    if (diff < 0) diff = -diff;
    if (diff <= (int32_t)s->cfg.deadband_us)
        pulse = (int32_t)s->cfg.neutral_us;
    esc_set_us(gpio, (uint16_t)pulse);
}

void esc_brake(uint gpio) {
    esc_slot_t *s = get_slot(gpio);
    if (!s || s->state != ESC_STATE_ARMED) return;
    s->last_cmd = get_absolute_time();
    raw_pulse(gpio, s->cfg.neutral_us);
}
