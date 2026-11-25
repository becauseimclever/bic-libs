#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include "bic/a3144.h"
#include "hardware/gpio.h"

static unsigned init_count;
static uint last_pin;
static bool last_direction, pull_up, pull_down, pin_value;
static unsigned callback_count;
static uint32_t callback_events;

void gpio_init(uint pin) { init_count++; last_pin = pin; }
void gpio_set_dir(uint pin, bool output) { assert(pin == last_pin); last_direction = output; }
void gpio_set_pulls(uint pin, bool up, bool down) {
    assert(pin == last_pin);
    pull_up = up;
    pull_down = down;
}
bool gpio_get(uint pin) { assert(pin == last_pin); return pin_value; }
static void changed(void *arg, uint32_t events) {
    assert(arg == &callback_count);
    callback_count++;
    callback_events = events;
}

int main(void) {
    bic_a3144_t sensor = BIC_A3144_INIT;
    bool detected = false;

    assert(bic_a3144_init(NULL, 2, true, changed, NULL) == BIC_A3144_INVALID_ARGUMENT);
    assert(bic_a3144_init(&sensor, NUM_BANK0_GPIOS, true, changed, NULL) == BIC_A3144_INVALID_ARGUMENT);
    assert(init_count == 0);
    assert(bic_a3144_read(&sensor, &detected) == BIC_A3144_NOT_INITIALIZED);
    assert(bic_a3144_init(&sensor, 2, true, changed, &callback_count) == BIC_A3144_OK);
    assert(last_pin == 2 && !last_direction && pull_up && !pull_down);
    assert(bic_a3144_init(&sensor, 3, false, NULL, NULL) == BIC_A3144_ALREADY_INITIALIZED);
    assert(init_count == 1);
    pin_value = true;
    assert(bic_a3144_read(&sensor, &detected) == BIC_A3144_OK && !detected);
    pin_value = false;
    assert(bic_a3144_read(&sensor, &detected) == BIC_A3144_OK && detected);
    assert(bic_a3144_read(&sensor, NULL) == BIC_A3144_INVALID_ARGUMENT);
    assert(bic_a3144_handle_irq(&sensor, GPIO_IRQ_EDGE_FALL) == BIC_A3144_OK);
    assert(callback_count == 1 && callback_events == BIC_A3144_DETECTED);
    assert(bic_a3144_handle_irq(&sensor, GPIO_IRQ_EDGE_RISE) == BIC_A3144_OK);
    assert(callback_count == 2 && callback_events == BIC_A3144_RELEASED);
    assert(bic_a3144_handle_irq(&sensor, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE) == BIC_A3144_OK);
    assert(callback_count == 3 && callback_events == (BIC_A3144_DETECTED | BIC_A3144_RELEASED));
    assert(bic_a3144_handle_irq(&sensor, 0) == BIC_A3144_INVALID_ARGUMENT);
    assert(bic_a3144_handle_irq(&sensor, 1) == BIC_A3144_INVALID_ARGUMENT);
    assert(callback_count == 3);
    assert(bic_a3144_deinit(&sensor) == BIC_A3144_OK && !pull_up && !pull_down);
    assert(bic_a3144_handle_irq(&sensor, GPIO_IRQ_EDGE_FALL) == BIC_A3144_NOT_INITIALIZED);
    assert(bic_a3144_deinit(&sensor) == BIC_A3144_NOT_INITIALIZED);
    assert(bic_a3144_init(&sensor, 4, false, NULL, NULL) == BIC_A3144_OK);
    assert(!pull_up && !pull_down);
    assert(bic_a3144_handle_irq(&sensor, GPIO_IRQ_EDGE_FALL) == BIC_A3144_NO_CALLBACK);
    assert(bic_a3144_deinit(&sensor) == BIC_A3144_OK);
    return 0;
}
