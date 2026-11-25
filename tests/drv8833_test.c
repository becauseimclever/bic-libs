#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include "bic/drv8833.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static bool gpio_value[NUM_BANK0_GPIOS];
static bool gpio_output[NUM_BANK0_GPIOS];
static uint gpio_function[NUM_BANK0_GPIOS];
static bool slice_enabled[8];
static uint16_t pwm_live[8][2], pwm_pending[8][2];
static unsigned slice_initializations[8];
static unsigned gpio_initializations, wake_delays;
static pwm_config slice_config[8];

void gpio_init(uint pin) { assert(pin < NUM_BANK0_GPIOS); gpio_initializations++; }
void gpio_put(uint pin, bool value) { gpio_value[pin] = value; }
void gpio_set_dir(uint pin, bool output) { gpio_output[pin] = output; }
void gpio_pull_up(uint pin) { gpio_value[pin] = true; }
void gpio_set_function(uint pin, uint function) { gpio_function[pin] = function; }
bool gpio_get(uint pin) { return gpio_value[pin]; }
uint pwm_gpio_to_slice_num(uint pin) { return (pin >> 1) & 7; }
uint pwm_gpio_to_channel(uint pin) { return pin & 1; }
pwm_config pwm_get_default_config(void) { pwm_config c = {1, 0, 65535}; return c; }
void pwm_config_set_clkdiv_int_frac(pwm_config *c, uint8_t i, uint8_t f) {
    c->integer = i; c->fraction = f;
}
void pwm_config_set_wrap(pwm_config *c, uint16_t wrap) { c->wrap = wrap; }
void pwm_init(uint slice, const pwm_config *c, bool start) {
    slice_initializations[slice]++;
    slice_config[slice] = *c;
    slice_enabled[slice] = start;
    memset(pwm_live[slice], 0, sizeof(pwm_live[slice]));
    memset(pwm_pending[slice], 0, sizeof(pwm_pending[slice]));
}
void pwm_set_gpio_level(uint pin, uint16_t level) {
    uint slice = pwm_gpio_to_slice_num(pin), channel = pwm_gpio_to_channel(pin);
    pwm_pending[slice][channel] = level;
    if (!slice_enabled[slice]) pwm_live[slice][channel] = level;
}
void pwm_set_enabled(uint slice, bool enabled) { slice_enabled[slice] = enabled; }
uint32_t clock_get_hz(int clock) { (void)clock; return 125000000; }
void sleep_us(uint64_t us) { assert(us >= 1000); wake_delays++; }
static void tick(void) {
    for (unsigned i = 0; i < 8; ++i)
        if (slice_enabled[i]) memcpy(pwm_live[i], pwm_pending[i], sizeof(pwm_live[i]));
}
static uint16_t level(uint pin) {
    return pwm_live[pwm_gpio_to_slice_num(pin)][pwm_gpio_to_channel(pin)];
}

int main(void) {
    bic_drv8833_t driver = BIC_DRV8833_INIT;
    bic_drv8833_config_t config = { { {2, 3}, {4, 5} }, 8, 9, 20000 };
    bool faulted = false;

    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_A, 1) == BIC_DRV8833_NOT_INITIALIZED);
    assert(bic_drv8833_init(NULL, &config) == BIC_DRV8833_INVALID_ARGUMENT);
    bic_drv8833_config_t invalid = config;
    invalid.input[1][0] = 18;
    assert(bic_drv8833_init(&driver, &invalid) == BIC_DRV8833_PWM_CONFLICT);
    invalid = config;
    invalid.sleep_pin = 3;
    assert(bic_drv8833_init(&driver, &invalid) == BIC_DRV8833_INVALID_ARGUMENT);
    invalid = config;
    invalid.input[0][0] = NUM_BANK0_GPIOS;
    assert(bic_drv8833_init(&driver, &invalid) == BIC_DRV8833_INVALID_ARGUMENT);
    invalid = config;
    invalid.pwm_frequency_hz = 0;
    assert(bic_drv8833_init(&driver, &invalid) == BIC_DRV8833_INVALID_ARGUMENT);
    invalid.pwm_frequency_hz = 1;
    assert(bic_drv8833_init(&driver, &invalid) == BIC_DRV8833_INVALID_ARGUMENT);
    invalid.pwm_frequency_hz = 125000000;
    assert(bic_drv8833_init(&driver, &invalid) == BIC_DRV8833_INVALID_ARGUMENT);
    assert(gpio_initializations == 0);

    assert(bic_drv8833_init(&driver, &config) == BIC_DRV8833_OK);
    assert(bic_drv8833_init(&driver, &config) == BIC_DRV8833_ALREADY_INITIALIZED);
    assert(driver.period == 6250 && slice_config[1].wrap == 6249);
    assert(slice_config[1].integer == 1 && slice_config[1].fraction == 0);
    assert(slice_initializations[1] == 1 && slice_initializations[2] == 1);
    assert(slice_enabled[1] && slice_enabled[2] && gpio_value[8] && wake_delays == 1);
    assert(level(2) == 0 && level(3) == 0 && level(4) == 0 && level(5) == 0);
    assert(gpio_output[2] && gpio_function[2] == GPIO_FUNC_PWM);
    assert(bic_drv8833_fault(&driver, &faulted) == BIC_DRV8833_OK && !faulted);
    gpio_value[9] = false;
    assert(bic_drv8833_fault(&driver, &faulted) == BIC_DRV8833_OK && faulted);

    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_A, 5000) == BIC_DRV8833_OK);
    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_B, -10000) == BIC_DRV8833_OK);
    tick();
    assert(level(2) == 3125 && level(3) == 0);
    assert(level(4) == 0 && level(5) == 6250);
    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_A, -10000) == BIC_DRV8833_OK);
    tick();
    assert(level(2) == 0 && level(3) == 6250);
    assert(bic_drv8833_brake(&driver, BIC_DRV8833_MOTOR_A) == BIC_DRV8833_OK);
    tick();
    assert(level(2) == 6250 && level(3) == 6250);
    assert(bic_drv8833_coast(&driver, BIC_DRV8833_MOTOR_A) == BIC_DRV8833_OK);
    tick();
    assert(level(2) == 0 && level(3) == 0 && level(5) == 6250);
    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_A, 10001) == BIC_DRV8833_INVALID_ARGUMENT);
    assert(bic_drv8833_drive(&driver, (bic_drv8833_motor_t)2, 1) == BIC_DRV8833_INVALID_ARGUMENT);

    assert(bic_drv8833_sleep(&driver) == BIC_DRV8833_OK);
    assert(!gpio_value[8] && !slice_enabled[1] && !slice_enabled[2]);
    assert(level(5) == 0);
    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_B, 1) == BIC_DRV8833_ASLEEP);
    assert(bic_drv8833_wake(&driver) == BIC_DRV8833_OK);
    assert(gpio_value[8] && slice_enabled[2] && level(5) == 0 && wake_delays == 2);
    assert(bic_drv8833_deinit(&driver) == BIC_DRV8833_OK);
    assert(!gpio_value[8] && !slice_enabled[1] && !slice_enabled[2]);
    assert(gpio_function[2] == GPIO_FUNC_SIO && gpio_output[2] && !gpio_value[2]);
    assert(bic_drv8833_deinit(&driver) == BIC_DRV8833_NOT_INITIALIZED);

    bic_drv8833_config_t shim = BIC_DRV8833_PICO_MOTOR_SHIM_CONFIG;
    assert(bic_drv8833_init(&driver, &shim) == BIC_DRV8833_OK);
    assert(bic_drv8833_sleep(&driver) == BIC_DRV8833_UNSUPPORTED);
    assert(bic_drv8833_fault(&driver, &faulted) == BIC_DRV8833_UNSUPPORTED);
    assert(bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_B, 10000) == BIC_DRV8833_OK);
    tick();
    assert(level(27) == driver.period && level(26) == 0);
    assert(bic_drv8833_deinit(&driver) == BIC_DRV8833_OK);
    return 0;
}
