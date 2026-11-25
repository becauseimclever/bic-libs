#include "bic/a3144.h"

#include "hardware/gpio.h"

bic_a3144_status_t bic_a3144_init(bic_a3144_t *sensor, uint32_t pin,
                                  bool internal_pull_up,
                                  bic_a3144_callback_t callback, void *arg) {
    if (!sensor || pin >= NUM_BANK0_GPIOS) return BIC_A3144_INVALID_ARGUMENT;
    if (sensor->initialized) return BIC_A3144_ALREADY_INITIALIZED;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, internal_pull_up, false);
    sensor->pin = pin;
    sensor->callback = callback;
    sensor->callback_arg = arg;
    sensor->initialized = true;
    return BIC_A3144_OK;
}

bic_a3144_status_t bic_a3144_read(const bic_a3144_t *sensor, bool *detected) {
    if (!sensor || !detected) return BIC_A3144_INVALID_ARGUMENT;
    if (!sensor->initialized) return BIC_A3144_NOT_INITIALIZED;
    *detected = !gpio_get(sensor->pin);
    return BIC_A3144_OK;
}

bic_a3144_status_t bic_a3144_handle_irq(bic_a3144_t *sensor, uint32_t events) {
    const uint32_t edge_mask = GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE;
    if (!sensor || !events || (events & ~edge_mask)) return BIC_A3144_INVALID_ARGUMENT;
    if (!sensor->initialized) return BIC_A3144_NOT_INITIALIZED;
    if (!sensor->callback) return BIC_A3144_NO_CALLBACK;

    uint32_t notification = 0;
    if (events & GPIO_IRQ_EDGE_FALL) notification |= BIC_A3144_DETECTED;
    if (events & GPIO_IRQ_EDGE_RISE) notification |= BIC_A3144_RELEASED;
    sensor->callback(sensor->callback_arg, notification);
    return BIC_A3144_OK;
}

bic_a3144_status_t bic_a3144_deinit(bic_a3144_t *sensor) {
    if (!sensor) return BIC_A3144_INVALID_ARGUMENT;
    if (!sensor->initialized) return BIC_A3144_NOT_INITIALIZED;
    gpio_set_pulls(sensor->pin, false, false);
    sensor->initialized = false;
    sensor->callback = NULL;
    sensor->callback_arg = NULL;
    return BIC_A3144_OK;
}
