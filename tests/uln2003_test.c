#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>

#include "bic/uln2003.h"
#include "hardware/gpio.h"

static bool gpio_value[NUM_BANK0_GPIOS];
static bool gpio_output[NUM_BANK0_GPIOS];
static unsigned init_count;
static uint64_t now_us;

void gpio_init(uint pin) { assert(pin < NUM_BANK0_GPIOS); init_count++; }
void gpio_put(uint pin, bool value) { gpio_value[pin] = value; }
void gpio_set_dir(uint pin, bool output) { gpio_output[pin] = output; }
uint64_t time_us_64(void) { return now_us; }

static unsigned coils(void) {
    unsigned pattern = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (gpio_value[i + 2]) pattern |= 1u << i;
    return pattern;
}

int main(void) {
    bic_uln2003_t motor = BIC_ULN2003_INIT;
    bic_uln2003_config_t config = {{2, 3, 4, 5}, BIC_ULN2003_HALF_STEP};
    bool moving;
    const unsigned half_forward[] = {1, 3, 2, 6, 4, 12, 8, 9};
    const unsigned half_reverse[] = {8, 12, 4, 6, 2, 3, 1, 9};
    const unsigned full_forward[] = {3, 6, 12, 9};
    const unsigned full_reverse[] = {12, 6, 3, 9};

    assert(bic_uln2003_init(NULL, &config) == BIC_ULN2003_INVALID_ARGUMENT);
    assert(bic_uln2003_init(&motor, NULL) == BIC_ULN2003_INVALID_ARGUMENT);
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_NOT_INITIALIZED);
    bic_uln2003_config_t invalid = config;
    invalid.input_pins[3] = 2;
    assert(bic_uln2003_init(&motor, &invalid) == BIC_ULN2003_INVALID_ARGUMENT);
    invalid = config;
    invalid.input_pins[0] = NUM_BANK0_GPIOS;
    assert(bic_uln2003_init(&motor, &invalid) == BIC_ULN2003_INVALID_ARGUMENT);
    invalid = config;
    invalid.mode = (bic_uln2003_mode_t)2;
    assert(bic_uln2003_init(&motor, &invalid) == BIC_ULN2003_INVALID_ARGUMENT);
    assert(init_count == 0);

    assert(bic_uln2003_init(&motor, &config) == BIC_ULN2003_OK);
    assert(bic_uln2003_init(&motor, &config) == BIC_ULN2003_ALREADY_INITIALIZED);
    assert(init_count == 4 && coils() == 0);
    for (unsigned i = 2; i < 6; ++i) assert(gpio_output[i]);
    assert(bic_uln2003_move(&motor, 0, 10, false) == BIC_ULN2003_INVALID_ARGUMENT);
    assert(bic_uln2003_move(&motor, 1, 0, false) == BIC_ULN2003_INVALID_ARGUMENT);
    assert(bic_uln2003_update(&motor, NULL) == BIC_ULN2003_INVALID_ARGUMENT);
    assert(bic_uln2003_move(&motor, 8, 10, true) == BIC_ULN2003_OK);
    assert(bic_uln2003_move(&motor, 1, 10, false) == BIC_ULN2003_BUSY);
    assert(coils() == 0);
    for (unsigned i = 0; i < 8; ++i) {
        assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && moving);
        assert(coils() == half_forward[i]);
        assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && moving);
        assert(coils() == half_forward[i]);
        now_us += 10;
    }
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && !moving);
    assert(coils() == 9);

    assert(bic_uln2003_move(&motor, -8, 10, false) == BIC_ULN2003_OK);
    for (unsigned i = 0; i < 8; ++i) {
        assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && moving);
        assert(coils() == half_reverse[i]);
        now_us += 10;
    }
    assert(coils() == 9);
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && !moving);
    assert(coils() == 0);

    assert(bic_uln2003_move(&motor, 3, 10, true) == BIC_ULN2003_OK);
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && coils() == 1);
    now_us += 1000;
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && coils() == 3);
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && coils() == 3);
    assert(bic_uln2003_stop(&motor, true) == BIC_ULN2003_OK && coils() == 3);
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && !moving);
    assert(bic_uln2003_stop(&motor, false) == BIC_ULN2003_OK && coils() == 0);
    assert(bic_uln2003_move(&motor, INT32_MIN, 10, false) == BIC_ULN2003_OK);
    assert(motor.remaining == UINT32_C(2147483648));
    assert(bic_uln2003_stop(&motor, false) == BIC_ULN2003_OK);
    assert(bic_uln2003_deinit(&motor) == BIC_ULN2003_OK && coils() == 0);
    assert(bic_uln2003_deinit(&motor) == BIC_ULN2003_NOT_INITIALIZED);

    config.mode = BIC_ULN2003_FULL_STEP;
    assert(bic_uln2003_init(&motor, &config) == BIC_ULN2003_OK);
    now_us = UINT64_MAX - 5;
    assert(bic_uln2003_move(&motor, 4, 10, false) == BIC_ULN2003_OK);
    for (unsigned i = 0; i < 4; ++i) {
        assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && moving);
        assert(coils() == full_forward[i]);
        now_us += 10;
    }
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && !moving);
    assert(coils() == 0);
    assert(bic_uln2003_move(&motor, -4, 10, true) == BIC_ULN2003_OK);
    for (unsigned i = 0; i < 4; ++i) {
        assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && moving);
        assert(coils() == full_reverse[i]);
        now_us += 10;
    }
    assert(bic_uln2003_update(&motor, &moving) == BIC_ULN2003_OK && !moving);
    assert(coils() == 9);
    assert(bic_uln2003_deinit(&motor) == BIC_ULN2003_OK);
    assert(coils() == 0);
    return 0;
}
