#ifndef BIC_NES_SNES_COMMON_H
#define BIC_NES_SNES_COMMON_H

#include "bic/nes_snes.h"
#include "hardware/gpio.h"

static inline bool bic_nes_snes_valid_pins(uint32_t latch, uint32_t clock, uint32_t data) {
    return latch < NUM_BANK0_GPIOS && clock < NUM_BANK0_GPIOS &&
           data < NUM_BANK0_GPIOS && latch != clock && latch != data && clock != data;
}

static inline bool bic_nes_snes_valid_profile(bic_nes_snes_profile_t profile) {
    return profile == BIC_NES_SNES_NES || profile == BIC_NES_SNES_SNES;
}

static inline void bic_nes_snes_input_pin(uint32_t pin) {
    gpio_init(pin);
    gpio_set_pulls(pin, false, false);
    gpio_set_inover(pin, GPIO_OVERRIDE_NORMAL);
    gpio_set_outover(pin, GPIO_OVERRIDE_NORMAL);
    gpio_set_oeover(pin, GPIO_OVERRIDE_NORMAL);
}

#endif
