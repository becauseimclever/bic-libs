/* Adapted from CHIRBot; Copyright (c) 2026 Darren, MIT; see third_party/CHIRBot/LICENSE. */
#include "common.h"
#include "nes_snes_output.pio.h"

static bool pio_pins_accessible(PIO pio, uint32_t latch, uint32_t clock, uint32_t data) {
    unsigned base = pio_get_gpio_base(pio);
#if !PICO_PIO_USE_GPIO_BASE
    if (base != 0) return false;
#endif
    return latch >= base && latch - base < 32 && clock >= base && clock - base < 32 &&
           data >= base && data - base < 32;
}

static uint32_t wire_word(bic_nes_snes_profile_t profile, uint16_t buttons) {
    return profile == BIC_NES_SNES_NES ? (uint32_t)(~buttons) & 0xffu : buttons;
}

bic_nes_snes_status_t bic_nes_snes_emulator_init(
    bic_nes_snes_emulator_t *emulator, bic_nes_snes_profile_t profile, PIO pio,
    uint32_t latch_pin, uint32_t clock_pin, uint32_t data_pin) {
    if (!emulator || !bic_nes_snes_valid_profile(profile) ||
        !bic_nes_snes_valid_pins(latch_pin, clock_pin, data_pin) ||
        (pio != pio0 && pio != pio1
#if NUM_PIOS > 2
         && pio != pio2
#endif
        )) return BIC_NES_SNES_INVALID_ARGUMENT;
    if (emulator->initialized) return BIC_NES_SNES_ALREADY_INITIALIZED;
    if (!pio_pins_accessible(pio, latch_pin, clock_pin, data_pin))
        return BIC_NES_SNES_INVALID_ARGUMENT;
    int sm = pio_claim_unused_sm(pio, false);
    if (sm < 0) return BIC_NES_SNES_NO_RESOURCES;
    int offset = pio_add_program(pio, &nes_snes_output_program);
    if (offset < 0) {
        pio_sm_unclaim(pio, (uint)sm);
        return BIC_NES_SNES_NO_RESOURCES;
    }
    pio_sm_config config = nes_snes_output_program_get_default_config((uint)offset);
    sm_config_set_out_pins(&config, data_pin, 1);
    sm_config_set_set_pins(&config, data_pin, 1);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_pins(&config, clock_pin);
    sm_config_set_in_shift(&config, false, false, 32);
    sm_config_set_jmp_pin(&config, latch_pin);
    if (pio_sm_init(pio, (uint)sm, (uint)offset, &config) != 0 ||
        pio_sm_set_consecutive_pindirs(pio, (uint)sm, data_pin, 1, true) != 0) {
        pio_remove_program(pio, &nes_snes_output_program, (uint)offset);
        pio_sm_unclaim(pio, (uint)sm);
        return BIC_NES_SNES_HARDWARE_ERROR;
    }

    bic_nes_snes_input_pin(latch_pin);
    bic_nes_snes_input_pin(clock_pin);
    bic_nes_snes_input_pin(data_pin);
    uint64_t mask = UINT64_C(1) << data_pin;
    pio_sm_set_pins_with_mask64(pio, (uint)sm,
                                profile == BIC_NES_SNES_NES ? mask : 0, mask);
    gpio_set_outover(data_pin, profile == BIC_NES_SNES_SNES
                                   ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);
    pio_gpio_init(pio, data_pin);
    pio_sm_put(pio, (uint)sm, wire_word(profile, 0));
    pio_sm_set_enabled(pio, (uint)sm, true);
    emulator->pio = pio;
    emulator->sm = (uint)sm;
    emulator->offset = (uint)offset;
    emulator->latch_pin = latch_pin;
    emulator->clock_pin = clock_pin;
    emulator->data_pin = data_pin;
    emulator->profile = profile;
    emulator->initialized = true;
    return BIC_NES_SNES_OK;
}

bic_nes_snes_status_t bic_nes_snes_emulator_set_buttons(
    bic_nes_snes_emulator_t *emulator, uint16_t buttons) {
    if (!emulator) return BIC_NES_SNES_INVALID_ARGUMENT;
    if (!emulator->initialized) return BIC_NES_SNES_NOT_INITIALIZED;
    if (emulator->profile == BIC_NES_SNES_NES && (buttons & 0xff00u))
        return BIC_NES_SNES_INVALID_ARGUMENT;
    pio_sm_clear_fifos(emulator->pio, emulator->sm);
    pio_sm_put(emulator->pio, emulator->sm, wire_word(emulator->profile, buttons));
    return BIC_NES_SNES_OK;
}

bic_nes_snes_status_t bic_nes_snes_emulator_deinit(bic_nes_snes_emulator_t *emulator) {
    if (!emulator) return BIC_NES_SNES_INVALID_ARGUMENT;
    if (!emulator->initialized) return BIC_NES_SNES_NOT_INITIALIZED;
    pio_sm_set_enabled(emulator->pio, emulator->sm, false);
    bic_nes_snes_input_pin(emulator->data_pin);
    bic_nes_snes_input_pin(emulator->latch_pin);
    bic_nes_snes_input_pin(emulator->clock_pin);
    pio_remove_program(emulator->pio, &nes_snes_output_program, emulator->offset);
    pio_sm_unclaim(emulator->pio, emulator->sm);
    emulator->initialized = false;
    return BIC_NES_SNES_OK;
}
