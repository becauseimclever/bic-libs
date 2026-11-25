#include "bic/uln2003.h"

#include "hardware/gpio.h"
#include "hardware/timer.h"

static void set_coils(const bic_uln2003_t *motor, uint8_t pattern) {
    for (unsigned i = 0; i < 4; ++i)
        if (!(pattern & (1u << i))) gpio_put(motor->config.input_pins[i], false);
    for (unsigned i = 0; i < 4; ++i)
        if (pattern & (1u << i)) gpio_put(motor->config.input_pins[i], true);
}

bic_uln2003_status_t bic_uln2003_init(bic_uln2003_t *motor,
                                      const bic_uln2003_config_t *config) {
    if (!motor || !config) return BIC_ULN2003_INVALID_ARGUMENT;
    if (motor->initialized) return BIC_ULN2003_ALREADY_INITIALIZED;
    if (config->mode != BIC_ULN2003_HALF_STEP && config->mode != BIC_ULN2003_FULL_STEP)
        return BIC_ULN2003_INVALID_ARGUMENT;
    for (unsigned i = 0; i < 4; ++i) {
        if (config->input_pins[i] >= NUM_BANK0_GPIOS) return BIC_ULN2003_INVALID_ARGUMENT;
        for (unsigned j = 0; j < i; ++j)
            if (config->input_pins[i] == config->input_pins[j])
                return BIC_ULN2003_INVALID_ARGUMENT;
    }

    motor->config = *config;
    motor->moving = false;
    motor->has_phase = false;
    motor->remaining = 0;
    for (unsigned i = 0; i < 4; ++i) {
        gpio_init(config->input_pins[i]);
        gpio_put(config->input_pins[i], false);
        gpio_set_dir(config->input_pins[i], GPIO_OUT);
    }
    motor->initialized = true;
    return BIC_ULN2003_OK;
}

bic_uln2003_status_t bic_uln2003_move(bic_uln2003_t *motor, int32_t steps,
                                      uint32_t interval_us, bool hold_after_move) {
    if (!motor || !steps || !interval_us) return BIC_ULN2003_INVALID_ARGUMENT;
    if (!motor->initialized) return BIC_ULN2003_NOT_INITIALIZED;
    if (motor->moving) return BIC_ULN2003_BUSY;
    motor->direction = steps > 0 ? 1 : -1;
    motor->remaining = steps > 0 ? (uint32_t)steps : (uint32_t)(-(int64_t)steps);
    motor->interval_us = interval_us;
    motor->hold_after_move = hold_after_move;
    motor->first_step = true;
    motor->moving = true;
    return BIC_ULN2003_OK;
}

bic_uln2003_status_t bic_uln2003_update(bic_uln2003_t *motor, bool *moving) {
    static const uint8_t half_steps[] = {1, 3, 2, 6, 4, 12, 8, 9};
    static const uint8_t full_steps[] = {3, 6, 12, 9};
    if (!motor || !moving) return BIC_ULN2003_INVALID_ARGUMENT;
    if (!motor->initialized) return BIC_ULN2003_NOT_INITIALIZED;

    if (motor->moving) {
        uint64_t now = time_us_64();
        if (motor->first_step || now - motor->last_step_us >= motor->interval_us) {
            if (motor->remaining) {
                uint8_t count = motor->config.mode == BIC_ULN2003_HALF_STEP ? 8 : 4;
                if (!motor->has_phase) {
                    motor->phase = 0;
                    motor->has_phase = true;
                } else if (motor->direction > 0) {
                    motor->phase = (uint8_t)((motor->phase + 1) % count);
                } else {
                    motor->phase = (uint8_t)((motor->phase + count - 1) % count);
                }
                set_coils(motor, motor->config.mode == BIC_ULN2003_HALF_STEP
                                     ? half_steps[motor->phase] : full_steps[motor->phase]);
                motor->remaining--;
                motor->last_step_us = now;
                motor->first_step = false;
            } else {
                if (!motor->hold_after_move) set_coils(motor, 0);
                motor->moving = false;
            }
        }
    }
    *moving = motor->moving;
    return BIC_ULN2003_OK;
}

bic_uln2003_status_t bic_uln2003_stop(bic_uln2003_t *motor, bool hold) {
    if (!motor) return BIC_ULN2003_INVALID_ARGUMENT;
    if (!motor->initialized) return BIC_ULN2003_NOT_INITIALIZED;
    motor->moving = false;
    motor->remaining = 0;
    if (!hold) set_coils(motor, 0);
    return BIC_ULN2003_OK;
}

bic_uln2003_status_t bic_uln2003_deinit(bic_uln2003_t *motor) {
    if (!motor) return BIC_ULN2003_INVALID_ARGUMENT;
    if (!motor->initialized) return BIC_ULN2003_NOT_INITIALIZED;
    bic_uln2003_stop(motor, false);
    motor->initialized = false;
    return BIC_ULN2003_OK;
}
