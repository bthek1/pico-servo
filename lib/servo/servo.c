#include "servo.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// 125 MHz / (100 * 25000) = 50 Hz
#define SERVO_CLKDIV  100.0f
#define SERVO_WRAP    24999
#define SERVO_PERIOD  20000  // µs
#define MAX_GPIO      30

const servo_config_t SERVO_SER0006 = { SERVO_POSITIONAL, 500,  2500, 1500 };
const servo_config_t SERVO_SG92R   = { SERVO_POSITIONAL, 500,  2500, 1500 };
const servo_config_t SERVO_MG996R  = { SERVO_CONTINUOUS, 1000, 2000, 1500 };

static servo_config_t s_cfg[MAX_GPIO];
static bool           s_inited[MAX_GPIO];

void servo_init_config(uint gpio, const servo_config_t *cfg) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice, SERVO_CLKDIV);
    pwm_set_wrap(slice, SERVO_WRAP);
    pwm_set_enabled(slice, true);
    if (gpio < MAX_GPIO) {
        s_cfg[gpio]    = *cfg;
        s_inited[gpio] = true;
    }
}

void servo_init(uint gpio) {
    servo_init_config(gpio, &SERVO_SER0006);
}

static const servo_config_t *get_cfg(uint gpio) {
    if (gpio < MAX_GPIO && s_inited[gpio]) return &s_cfg[gpio];
    return &SERVO_SER0006;
}

void servo_set_us(uint gpio, uint16_t pulse_us) {
    const servo_config_t *cfg = get_cfg(gpio);
    if (pulse_us < cfg->min_us) pulse_us = cfg->min_us;
    if (pulse_us > cfg->max_us) pulse_us = cfg->max_us;
    uint16_t level = (uint16_t)((uint32_t)pulse_us * (SERVO_WRAP + 1) / SERVO_PERIOD);
    pwm_set_gpio_level(gpio, level);
}

void servo_set_deg(uint gpio, float degrees) {
    const servo_config_t *cfg = get_cfg(gpio);
    if (cfg->type != SERVO_POSITIONAL) return;
    if (degrees < 0.0f)   degrees = 0.0f;
    if (degrees > 180.0f) degrees = 180.0f;
    uint16_t pulse_us = (uint16_t)(cfg->min_us + degrees / 180.0f * (float)(cfg->max_us - cfg->min_us));
    servo_set_us(gpio, pulse_us);
}

void servo_set_speed(uint gpio, float speed) {
    const servo_config_t *cfg = get_cfg(gpio);
    if (cfg->type != SERVO_CONTINUOUS) return;
    if (speed < -1.0f) speed = -1.0f;
    if (speed >  1.0f) speed =  1.0f;
    int32_t half  = (int32_t)(cfg->max_us - cfg->min_us) / 2;
    int32_t pulse = (int32_t)cfg->stop_us + (int32_t)(speed * (float)half);
    if (pulse < (int32_t)cfg->min_us) pulse = (int32_t)cfg->min_us;
    if (pulse > (int32_t)cfg->max_us) pulse = (int32_t)cfg->max_us;
    servo_set_us(gpio, (uint16_t)pulse);
}

void servo_set_stop_us(uint gpio, uint16_t stop_us) {
    if (gpio < MAX_GPIO && s_inited[gpio] && s_cfg[gpio].type == SERVO_CONTINUOUS)
        s_cfg[gpio].stop_us = stop_us;
}

void servo_safe_stop(uint gpio) {
    const servo_config_t *cfg = get_cfg(gpio);
    servo_set_us(gpio, cfg->stop_us);
}
