#ifndef BIC_ULN2003_H
#define BIC_ULN2003_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BIC_ULN2003_OK = 0,
    BIC_ULN2003_INVALID_ARGUMENT,
    BIC_ULN2003_NOT_INITIALIZED,
    BIC_ULN2003_ALREADY_INITIALIZED,
    BIC_ULN2003_BUSY
} bic_uln2003_status_t;

typedef enum {
    BIC_ULN2003_HALF_STEP = 0,
    BIC_ULN2003_FULL_STEP
} bic_uln2003_mode_t;

typedef struct {
    uint8_t input_pins[4];
    bic_uln2003_mode_t mode;
} bic_uln2003_config_t;

typedef struct {
    bic_uln2003_config_t config;
    uint64_t last_step_us;
    uint32_t interval_us;
    uint32_t remaining;
    uint8_t phase;
    int8_t direction;
    bool initialized;
    bool moving;
    bool has_phase;
    bool first_step;
    bool hold_after_move;
} bic_uln2003_t;

#define BIC_ULN2003_INIT {0}

/* input_pins are the GPIOs connected to driver IN1 through IN4, in order. */
bic_uln2003_status_t bic_uln2003_init(bic_uln2003_t *motor,
                                      const bic_uln2003_config_t *config);

/* Queue a nonzero signed number of steps. The first step occurs in update(). */
bic_uln2003_status_t bic_uln2003_move(bic_uln2003_t *motor, int32_t steps,
                                      uint32_t interval_us, bool hold_after_move);

/* Service one due step at most; moving is false after the final step's dwell. */
bic_uln2003_status_t bic_uln2003_update(bic_uln2003_t *motor, bool *moving);

/* Stop immediately, optionally leaving the current coil pattern energized. */
bic_uln2003_status_t bic_uln2003_stop(bic_uln2003_t *motor, bool hold);

/* Stop and de-energize the coils. */
bic_uln2003_status_t bic_uln2003_deinit(bic_uln2003_t *motor);

#ifdef __cplusplus
}
#endif

#endif
