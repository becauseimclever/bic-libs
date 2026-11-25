/* Adapted from CHIRBot; Copyright (c) 2026 Darren, MIT; see third_party/CHIRBot/LICENSE. */
#include "common.h"
#include "pico/time.h"

bic_nes_snes_status_t bic_nes_snes_reader_init(
    bic_nes_snes_reader_t *reader, bic_nes_snes_profile_t profile,
    uint32_t latch_pin, uint32_t clock_pin, uint32_t data_pin) {
    if (!reader || !bic_nes_snes_valid_profile(profile) ||
        !bic_nes_snes_valid_pins(latch_pin, clock_pin, data_pin))
        return BIC_NES_SNES_INVALID_ARGUMENT;
    if (reader->initialized) return BIC_NES_SNES_ALREADY_INITIALIZED;

    bic_nes_snes_input_pin(latch_pin);
    gpio_put(latch_pin, false);
    gpio_set_dir(latch_pin, GPIO_OUT);
    bic_nes_snes_input_pin(clock_pin);
    gpio_put(clock_pin, true);
    gpio_set_dir(clock_pin, GPIO_OUT);
    bic_nes_snes_input_pin(data_pin);
    reader->latch_pin = latch_pin;
    reader->clock_pin = clock_pin;
    reader->data_pin = data_pin;
    reader->profile = profile;
    reader->initialized = true;
    return BIC_NES_SNES_OK;
}

bic_nes_snes_status_t bic_nes_snes_reader_read(
    const bic_nes_snes_reader_t *reader, uint16_t *buttons) {
    if (!reader || !buttons) return BIC_NES_SNES_INVALID_ARGUMENT;
    if (!reader->initialized) return BIC_NES_SNES_NOT_INITIALIZED;
    unsigned count = reader->profile == BIC_NES_SNES_NES ? 8u : 16u;
    uint16_t result = 0;
    gpio_put(reader->latch_pin, true);
    sleep_us(12);
    gpio_put(reader->latch_pin, false);
    sleep_us(6);
    for (unsigned bit = 0; bit < count; ++bit) {
        if (!gpio_get(reader->data_pin)) result |= (uint16_t)(1u << bit);
        gpio_put(reader->clock_pin, false);
        sleep_us(6);
        gpio_put(reader->clock_pin, true);
        sleep_us(6);
    }
    *buttons = result;
    return BIC_NES_SNES_OK;
}

bic_nes_snes_status_t bic_nes_snes_reader_deinit(bic_nes_snes_reader_t *reader) {
    if (!reader) return BIC_NES_SNES_INVALID_ARGUMENT;
    if (!reader->initialized) return BIC_NES_SNES_NOT_INITIALIZED;
    bic_nes_snes_input_pin(reader->latch_pin);
    bic_nes_snes_input_pin(reader->clock_pin);
    bic_nes_snes_input_pin(reader->data_pin);
    reader->initialized = false;
    return BIC_NES_SNES_OK;
}
