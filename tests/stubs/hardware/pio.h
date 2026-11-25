#ifndef TEST_HARDWARE_PIO_H
#define TEST_HARDWARE_PIO_H
#include <stdbool.h>
#include <stdint.h>
#include "hardware/gpio.h"

#define NUM_PIOS 2
#define PICO_PIO_USE_GPIO_BASE 0
typedef struct { int unused; } pio_hw_t;
typedef pio_hw_t *PIO;
extern pio_hw_t test_pio0, test_pio1;
#define pio0 (&test_pio0)
#define pio1 (&test_pio1)
typedef struct { unsigned unused; } pio_program_t;
typedef struct { uint out_pin, set_pin, in_pin, jmp_pin; bool out_right, in_right; } pio_sm_config;
uint pio_get_gpio_base(PIO pio);
int pio_claim_unused_sm(PIO pio, bool required);
void pio_sm_unclaim(PIO pio, uint sm);
int pio_add_program(PIO pio, const pio_program_t *program);
void pio_remove_program(PIO pio, const pio_program_t *program, uint offset);
void sm_config_set_out_pins(pio_sm_config *c, uint pin, uint count);
void sm_config_set_set_pins(pio_sm_config *c, uint pin, uint count);
void sm_config_set_out_shift(pio_sm_config *c, bool right, bool autopull, uint threshold);
void sm_config_set_in_pins(pio_sm_config *c, uint pin);
void sm_config_set_in_shift(pio_sm_config *c, bool right, bool autopush, uint threshold);
void sm_config_set_jmp_pin(pio_sm_config *c, uint pin);
int pio_sm_init(PIO pio, uint sm, uint offset, const pio_sm_config *config);
int pio_sm_set_consecutive_pindirs(PIO pio, uint sm, uint pin, uint count, bool out);
void pio_sm_set_pins_with_mask64(PIO pio, uint sm, uint64_t value, uint64_t mask);
void pio_gpio_init(PIO pio, uint pin);
void pio_sm_put(PIO pio, uint sm, uint32_t value);
void pio_sm_set_enabled(PIO pio, uint sm, bool enabled);
void pio_sm_clear_fifos(PIO pio, uint sm);
#endif
