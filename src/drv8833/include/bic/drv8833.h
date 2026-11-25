#ifndef BIC_DRV8833_H
#define BIC_DRV8833_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BIC_DRV8833_NO_PIN UINT8_MAX
#define BIC_DRV8833_MAX_DUTY 10000

typedef enum {
    BIC_DRV8833_OK = 0,
    BIC_DRV8833_INVALID_ARGUMENT,
    BIC_DRV8833_PWM_CONFLICT,
    BIC_DRV8833_NOT_INITIALIZED,
    BIC_DRV8833_ALREADY_INITIALIZED,
    BIC_DRV8833_UNSUPPORTED,
    BIC_DRV8833_ASLEEP
} bic_drv8833_status_t;

typedef enum {
    BIC_DRV8833_MOTOR_A = 0,
    BIC_DRV8833_MOTOR_B = 1
} bic_drv8833_motor_t;

typedef struct {
    uint8_t input[2][2];
    uint8_t sleep_pin;
    uint8_t fault_pin;
    uint32_t pwm_frequency_hz;
} bic_drv8833_config_t;

/* Motor 1 = GP6/GP7; motor 2 = GP27/GP26. */
#define BIC_DRV8833_PICO_MOTOR_SHIM_CONFIG \
    { { {6, 7}, {27, 26} }, BIC_DRV8833_NO_PIN, BIC_DRV8833_NO_PIN, 20000 }

typedef struct {
    bic_drv8833_config_t config;
    uint16_t period;
    bool initialized;
    bool asleep;
} bic_drv8833_t;

#define BIC_DRV8833_INIT {0}

/* Initialize with both bridges coasting. Owns the GPIOs and their PWM slices. */
bic_drv8833_status_t bic_drv8833_init(bic_drv8833_t *driver,
                                      const bic_drv8833_config_t *config);

/* Set signed duty in [-10000, 10000]. Zero coasts; PWM uses fast decay. */
bic_drv8833_status_t bic_drv8833_drive(bic_drv8833_t *driver,
                                       bic_drv8833_motor_t motor, int32_t duty);
bic_drv8833_status_t bic_drv8833_coast(bic_drv8833_t *driver, bic_drv8833_motor_t motor);
bic_drv8833_status_t bic_drv8833_brake(bic_drv8833_t *driver, bic_drv8833_motor_t motor);

/* Requires a connected nSLEEP pin. Sleep clears both motor commands. */
bic_drv8833_status_t bic_drv8833_sleep(bic_drv8833_t *driver);
bic_drv8833_status_t bic_drv8833_wake(bic_drv8833_t *driver);

/* Requires a connected nFAULT pin. The result is true when nFAULT is low. */
bic_drv8833_status_t bic_drv8833_fault(const bic_drv8833_t *driver, bool *faulted);

/* Coast both bridges, assert nSLEEP if connected, and release the PWM slices. */
bic_drv8833_status_t bic_drv8833_deinit(bic_drv8833_t *driver);

#ifdef __cplusplus
}
#endif

#endif
