#ifndef BIC_NES_SNES_H
#define BIC_NES_SNES_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/pio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BIC_NES_SNES_OK = 0,
    BIC_NES_SNES_INVALID_ARGUMENT,
    BIC_NES_SNES_NOT_INITIALIZED,
    BIC_NES_SNES_ALREADY_INITIALIZED,
    BIC_NES_SNES_NO_RESOURCES,
    BIC_NES_SNES_HARDWARE_ERROR
} bic_nes_snes_status_t;

typedef enum {
    BIC_NES_SNES_NES,
    BIC_NES_SNES_SNES
} bic_nes_snes_profile_t;

/* Active-high masks in controller serial order. */
enum {
    BIC_NES_A = 1u << 0, BIC_NES_B = 1u << 1,
    BIC_NES_SELECT = 1u << 2, BIC_NES_START = 1u << 3,
    BIC_NES_UP = 1u << 4, BIC_NES_DOWN = 1u << 5,
    BIC_NES_LEFT = 1u << 6, BIC_NES_RIGHT = 1u << 7,
    BIC_SNES_B = 1u << 0, BIC_SNES_Y = 1u << 1,
    BIC_SNES_SELECT = 1u << 2, BIC_SNES_START = 1u << 3,
    BIC_SNES_UP = 1u << 4, BIC_SNES_DOWN = 1u << 5,
    BIC_SNES_LEFT = 1u << 6, BIC_SNES_RIGHT = 1u << 7,
    BIC_SNES_A = 1u << 8, BIC_SNES_X = 1u << 9,
    BIC_SNES_L = 1u << 10, BIC_SNES_R = 1u << 11
};

typedef struct {
    uint32_t latch_pin, clock_pin, data_pin;
    bic_nes_snes_profile_t profile;
    bool initialized;
} bic_nes_snes_reader_t;

typedef struct {
    PIO pio;
    uint32_t latch_pin, clock_pin, data_pin;
    unsigned sm, offset;
    bic_nes_snes_profile_t profile;
    bool initialized;
} bic_nes_snes_emulator_t;

#define BIC_NES_SNES_READER_INIT {0}
#define BIC_NES_SNES_EMULATOR_INIT {0}

/* Pins are exclusive, distinct GPIOs; signal level conversion is external. */
bic_nes_snes_status_t bic_nes_snes_reader_init(
    bic_nes_snes_reader_t *reader, bic_nes_snes_profile_t profile,
    uint32_t latch_pin, uint32_t clock_pin, uint32_t data_pin);

/* Blocking poll: returns active-high buttons in serial order. */
bic_nes_snes_status_t bic_nes_snes_reader_read(
    const bic_nes_snes_reader_t *reader, uint16_t *buttons);
bic_nes_snes_status_t bic_nes_snes_reader_deinit(bic_nes_snes_reader_t *reader);

/* Claims one state machine and program space on the selected PIO. */
bic_nes_snes_status_t bic_nes_snes_emulator_init(
    bic_nes_snes_emulator_t *emulator, bic_nes_snes_profile_t profile, PIO pio,
    uint32_t latch_pin, uint32_t clock_pin, uint32_t data_pin);

/* Publishes the latest state; calls must be serialized by the application. */
bic_nes_snes_status_t bic_nes_snes_emulator_set_buttons(
    bic_nes_snes_emulator_t *emulator, uint16_t buttons);
bic_nes_snes_status_t bic_nes_snes_emulator_deinit(bic_nes_snes_emulator_t *emulator);

#ifdef __cplusplus
}
#endif

#endif
