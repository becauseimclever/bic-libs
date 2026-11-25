#ifndef BIC_A3144_H
#define BIC_A3144_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BIC_A3144_OK = 0,
    BIC_A3144_INVALID_ARGUMENT,
    BIC_A3144_NOT_INITIALIZED,
    BIC_A3144_ALREADY_INITIALIZED,
    BIC_A3144_NO_CALLBACK
} bic_a3144_status_t;

enum {
    BIC_A3144_DETECTED = 1u,
    BIC_A3144_RELEASED = 2u
};

typedef void (*bic_a3144_callback_t)(void *arg, uint32_t events);

typedef struct {
    uint32_t pin;
    bic_a3144_callback_t callback;
    void *callback_arg;
    bool initialized;
} bic_a3144_t;

#define BIC_A3144_INIT {0}

/* The callback may be NULL for polling-only use. */
bic_a3144_status_t bic_a3144_init(bic_a3144_t *sensor, uint32_t pin,
                                  bool internal_pull_up,
                                  bic_a3144_callback_t callback, void *arg);

/* Read the current active-low output. */
bic_a3144_status_t bic_a3144_read(const bic_a3144_t *sensor, bool *detected);

/* Forward edge events for this sensor from the application's GPIO handler. */
bic_a3144_status_t bic_a3144_handle_irq(bic_a3144_t *sensor, uint32_t events);

/* Disable the application's IRQ for this pin before deinitializing it. */
bic_a3144_status_t bic_a3144_deinit(bic_a3144_t *sensor);

#ifdef __cplusplus
}
#endif

#endif
