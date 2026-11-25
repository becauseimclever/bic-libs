#ifndef TEST_HARDWARE_PWM_H
#define TEST_HARDWARE_PWM_H
#include "hardware/gpio.h"
typedef struct { uint8_t integer, fraction; uint16_t wrap; } pwm_config;
uint pwm_gpio_to_slice_num(uint pin);
uint pwm_gpio_to_channel(uint pin);
pwm_config pwm_get_default_config(void);
void pwm_config_set_clkdiv_int_frac(pwm_config *config, uint8_t integer, uint8_t fraction);
void pwm_config_set_wrap(pwm_config *config, uint16_t wrap);
void pwm_init(uint slice, const pwm_config *config, bool start);
void pwm_set_gpio_level(uint pin, uint16_t level);
void pwm_set_enabled(uint slice, bool enabled);
#endif
