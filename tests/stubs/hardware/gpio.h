#ifndef TEST_HARDWARE_GPIO_H
#define TEST_HARDWARE_GPIO_H
#include <stdbool.h>
#include <stdint.h>
typedef unsigned int uint;
#define NUM_BANK0_GPIOS 30
#define GPIO_IN false
#define GPIO_OUT true
#define GPIO_FUNC_SIO 0
#define GPIO_FUNC_PWM 4
#define GPIO_IRQ_EDGE_FALL 4u
#define GPIO_IRQ_EDGE_RISE 8u
#define GPIO_OVERRIDE_NORMAL 0
#define GPIO_OVERRIDE_INVERT 1
void gpio_init(uint pin);
void gpio_put(uint pin, bool value);
void gpio_set_dir(uint pin, bool output);
void gpio_pull_up(uint pin);
void gpio_set_pulls(uint pin, bool up, bool down);
void gpio_set_function(uint pin, uint function);
void gpio_set_inover(uint pin, uint value);
void gpio_set_outover(uint pin, uint value);
void gpio_set_oeover(uint pin, uint value);
bool gpio_get(uint pin);
#endif
