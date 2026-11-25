#include <stdio.h>

#include "bic/a3144.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#define SENSOR_PIN 15
#define SENSOR_EDGES (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)

static bic_a3144_t sensor = BIC_A3144_INIT;
static volatile uint32_t pending_events;
static volatile bool irq_error;

static void sensor_changed(void *arg, uint32_t events) {
    (void)arg;
    pending_events |= events;
}

static void gpio_irq(uint gpio, uint32_t events) {
    if (gpio == SENSOR_PIN &&
        bic_a3144_handle_irq(&sensor, events) != BIC_A3144_OK) {
        irq_error = true;
    }
}

int main(void) {
    stdio_init_all();
    bic_a3144_status_t status = bic_a3144_init(&sensor, SENSOR_PIN, true,
                                               sensor_changed, NULL);
    if (status != BIC_A3144_OK) {
        printf("A3144 initialization failed: %d\n", (int)status);
        return 1;
    }
    gpio_set_irq_enabled_with_callback(SENSOR_PIN, SENSOR_EDGES, true, gpio_irq);

    bool detected;
    status = bic_a3144_read(&sensor, &detected);
    if (status != BIC_A3144_OK) {
        printf("A3144 read failed: %d\n", (int)status);
        return 1;
    }
    printf("A3144 on GP%d; initial state: %s\n", SENSOR_PIN,
           detected ? "detected" : "released");
    while (true) {
        uint32_t saved = save_and_disable_interrupts();
        uint32_t events = pending_events;
        bool failed = irq_error;
        pending_events = 0;
        irq_error = false;
        restore_interrupts(saved);

        if (failed) printf("A3144 interrupt dispatch failed\n");
        if (events & BIC_A3144_DETECTED) printf("Magnetic field detected\n");
        if (events & BIC_A3144_RELEASED) printf("Magnetic field released\n");
        sleep_ms(10);
    }
}
