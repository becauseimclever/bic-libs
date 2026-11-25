#include "bic/drv8833.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"

static bic_drv8833_status_t check_motor(const bic_drv8833_t *driver,
                                        bic_drv8833_motor_t motor) {
    if (!driver) return BIC_DRV8833_INVALID_ARGUMENT;
    if (!driver->initialized) return BIC_DRV8833_NOT_INITIALIZED;
    if (motor != BIC_DRV8833_MOTOR_A && motor != BIC_DRV8833_MOTOR_B)
        return BIC_DRV8833_INVALID_ARGUMENT;
    if (driver->asleep) return BIC_DRV8833_ASLEEP;
    return BIC_DRV8833_OK;
}

static bool valid_pin(uint8_t pin) {
    return pin < NUM_BANK0_GPIOS;
}

static void set_levels(const bic_drv8833_t *driver, bic_drv8833_motor_t motor,
                       uint16_t first, uint16_t second) {
    uint8_t pin1 = driver->config.input[motor][0];
    uint8_t pin2 = driver->config.input[motor][1];
    if (!first) {
        pwm_set_gpio_level(pin1, first);
        pwm_set_gpio_level(pin2, second);
    } else {
        pwm_set_gpio_level(pin2, second);
        pwm_set_gpio_level(pin1, first);
    }
}

static void enable_slices(const bic_drv8833_t *driver, bool enabled) {
    for (unsigned i = 0; i < 4; ++i) {
        unsigned slice = pwm_gpio_to_slice_num(driver->config.input[i / 2][i % 2]);
        bool seen = false;
        for (unsigned j = 0; j < i; ++j)
            if (pwm_gpio_to_slice_num(driver->config.input[j / 2][j % 2]) == slice)
                seen = true;
        if (!seen) pwm_set_enabled(slice, enabled);
    }
}

bic_drv8833_status_t bic_drv8833_init(bic_drv8833_t *driver,
                                      const bic_drv8833_config_t *config) {
    if (!driver || !config) return BIC_DRV8833_INVALID_ARGUMENT;
    if (driver->initialized) return BIC_DRV8833_ALREADY_INITIALIZED;

    uint8_t pins[6] = { config->input[0][0], config->input[0][1],
                        config->input[1][0], config->input[1][1],
                        config->sleep_pin, config->fault_pin };
    for (unsigned i = 0; i < 6; ++i) {
        if (i >= 4 && pins[i] == BIC_DRV8833_NO_PIN) continue;
        if (!valid_pin(pins[i])) return BIC_DRV8833_INVALID_ARGUMENT;
        for (unsigned j = 0; j < i; ++j) {
            if (pins[i] == pins[j]) return BIC_DRV8833_INVALID_ARGUMENT;
            if (i < 4 && j < 4 &&
                pwm_gpio_to_slice_num(pins[i]) == pwm_gpio_to_slice_num(pins[j]) &&
                pwm_gpio_to_channel(pins[i]) == pwm_gpio_to_channel(pins[j]))
                return BIC_DRV8833_PWM_CONFLICT;
        }
    }

    uint32_t clock_hz = clock_get_hz(clk_sys);
    if (!config->pwm_frequency_hz || !clock_hz) return BIC_DRV8833_INVALID_ARGUMENT;
    uint64_t numerator = (uint64_t)clock_hz * 16;
    uint64_t denominator = (uint64_t)config->pwm_frequency_hz * UINT16_MAX;
    uint64_t divider = (numerator + denominator - 1) / denominator;
    if (divider < 16) divider = 16;
    if (divider > 4095) return BIC_DRV8833_INVALID_ARGUMENT;
    uint64_t period = numerator / ((uint64_t)config->pwm_frequency_hz * divider);
    if (period < 2 || period > UINT16_MAX) return BIC_DRV8833_INVALID_ARGUMENT;

    driver->config = *config;
    driver->period = (uint16_t)period;
    driver->asleep = false;

    if (config->sleep_pin != BIC_DRV8833_NO_PIN) {
        gpio_init(config->sleep_pin);
        gpio_put(config->sleep_pin, false);
        gpio_set_dir(config->sleep_pin, GPIO_OUT);
    }
    if (config->fault_pin != BIC_DRV8833_NO_PIN) {
        gpio_init(config->fault_pin);
        gpio_set_dir(config->fault_pin, GPIO_IN);
        gpio_pull_up(config->fault_pin);
    }
    for (unsigned i = 0; i < 4; ++i) {
        gpio_init(pins[i]);
        gpio_put(pins[i], false);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    pwm_config pwm = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&pwm, (uint8_t)(divider / 16),
                                   (uint8_t)(divider % 16));
    pwm_config_set_wrap(&pwm, (uint16_t)(period - 1));
    for (unsigned i = 0; i < 4; ++i) {
        unsigned slice = pwm_gpio_to_slice_num(pins[i]);
        bool seen = false;
        for (unsigned j = 0; j < i; ++j)
            if (pwm_gpio_to_slice_num(pins[j]) == slice) seen = true;
        if (!seen) pwm_init(slice, &pwm, false);
        pwm_set_gpio_level(pins[i], 0);
    }
    for (unsigned i = 0; i < 4; ++i) gpio_set_function(pins[i], GPIO_FUNC_PWM);
    enable_slices(driver, true);
    driver->initialized = true;
    if (config->sleep_pin != BIC_DRV8833_NO_PIN) {
        gpio_put(config->sleep_pin, true);
        sleep_us(1000);
    }
    return BIC_DRV8833_OK;
}

bic_drv8833_status_t bic_drv8833_drive(bic_drv8833_t *driver,
                                       bic_drv8833_motor_t motor, int32_t duty) {
    bic_drv8833_status_t status = check_motor(driver, motor);
    if (status != BIC_DRV8833_OK) return status;
    if (duty < -BIC_DRV8833_MAX_DUTY || duty > BIC_DRV8833_MAX_DUTY)
        return BIC_DRV8833_INVALID_ARGUMENT;
    uint32_t magnitude = (uint32_t)(duty < 0 ? -duty : duty);
    uint16_t level = (uint16_t)((magnitude * driver->period + BIC_DRV8833_MAX_DUTY / 2)
                                / BIC_DRV8833_MAX_DUTY);
    set_levels(driver, motor, duty > 0 ? level : 0, duty < 0 ? level : 0);
    return BIC_DRV8833_OK;
}

bic_drv8833_status_t bic_drv8833_coast(bic_drv8833_t *driver, bic_drv8833_motor_t motor) {
    return bic_drv8833_drive(driver, motor, 0);
}

bic_drv8833_status_t bic_drv8833_brake(bic_drv8833_t *driver, bic_drv8833_motor_t motor) {
    bic_drv8833_status_t status = check_motor(driver, motor);
    if (status != BIC_DRV8833_OK) return status;
    set_levels(driver, motor, driver->period, driver->period);
    return BIC_DRV8833_OK;
}

bic_drv8833_status_t bic_drv8833_sleep(bic_drv8833_t *driver) {
    if (!driver) return BIC_DRV8833_INVALID_ARGUMENT;
    if (!driver->initialized) return BIC_DRV8833_NOT_INITIALIZED;
    if (driver->config.sleep_pin == BIC_DRV8833_NO_PIN) return BIC_DRV8833_UNSUPPORTED;
    gpio_put(driver->config.sleep_pin, false);
    enable_slices(driver, false);
    set_levels(driver, BIC_DRV8833_MOTOR_A, 0, 0);
    set_levels(driver, BIC_DRV8833_MOTOR_B, 0, 0);
    driver->asleep = true;
    return BIC_DRV8833_OK;
}

bic_drv8833_status_t bic_drv8833_wake(bic_drv8833_t *driver) {
    if (!driver) return BIC_DRV8833_INVALID_ARGUMENT;
    if (!driver->initialized) return BIC_DRV8833_NOT_INITIALIZED;
    if (driver->config.sleep_pin == BIC_DRV8833_NO_PIN) return BIC_DRV8833_UNSUPPORTED;
    if (driver->asleep) {
        enable_slices(driver, true);
        gpio_put(driver->config.sleep_pin, true);
        sleep_us(1000);
        driver->asleep = false;
    }
    return BIC_DRV8833_OK;
}

bic_drv8833_status_t bic_drv8833_fault(const bic_drv8833_t *driver, bool *faulted) {
    if (!driver || !faulted) return BIC_DRV8833_INVALID_ARGUMENT;
    if (!driver->initialized) return BIC_DRV8833_NOT_INITIALIZED;
    if (driver->config.fault_pin == BIC_DRV8833_NO_PIN) return BIC_DRV8833_UNSUPPORTED;
    *faulted = !gpio_get(driver->config.fault_pin);
    return BIC_DRV8833_OK;
}

bic_drv8833_status_t bic_drv8833_deinit(bic_drv8833_t *driver) {
    if (!driver) return BIC_DRV8833_INVALID_ARGUMENT;
    if (!driver->initialized) return BIC_DRV8833_NOT_INITIALIZED;
    if (driver->config.sleep_pin != BIC_DRV8833_NO_PIN)
        gpio_put(driver->config.sleep_pin, false);
    for (unsigned motor = 0; motor < 2; ++motor) {
        for (unsigned input = 0; input < 2; ++input) {
            uint8_t pin = driver->config.input[motor][input];
            gpio_set_function(pin, GPIO_FUNC_SIO);
            gpio_put(pin, false);
            gpio_set_dir(pin, GPIO_OUT);
            pwm_set_gpio_level(pin, 0);
        }
    }
    enable_slices(driver, false);
    driver->initialized = false;
    driver->asleep = false;
    return BIC_DRV8833_OK;
}
