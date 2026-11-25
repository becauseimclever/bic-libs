#ifndef TEST_NES_SNES_OUTPUT_PIO_H
#define TEST_NES_SNES_OUTPUT_PIO_H
#include "hardware/pio.h"
static const pio_program_t nes_snes_output_program = {0};
static inline pio_sm_config nes_snes_output_program_get_default_config(uint offset) {
    (void)offset;
    pio_sm_config config = {0};
    return config;
}
#endif
