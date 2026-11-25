#include <stdio.h>

#include "bic/drv8833.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    bic_drv8833_t driver = BIC_DRV8833_INIT;
    const bic_drv8833_config_t config = BIC_DRV8833_PICO_MOTOR_SHIM_CONFIG;
    bic_drv8833_status_t status = bic_drv8833_init(&driver, &config);
    if (status != BIC_DRV8833_OK) {
        printf("DRV8833 initialization failed: %d\n", (int)status);
        return 1;
    }

    printf("Motor SHIM: 1/2 = motor 1 forward/reverse, 3/4 = motor 2 forward/reverse\n");
    printf("s = coast both, b = brake both, ? = help. Motors start only on command.\n");
    while (true) {
        int key = getchar_timeout_us(100000);
        switch (key) {
        case '1': status = bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_A, 2500); break;
        case '2': status = bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_A, -2500); break;
        case '3': status = bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_B, 2500); break;
        case '4': status = bic_drv8833_drive(&driver, BIC_DRV8833_MOTOR_B, -2500); break;
        case 's':
            status = bic_drv8833_coast(&driver, BIC_DRV8833_MOTOR_A);
            if (status == BIC_DRV8833_OK) status = bic_drv8833_coast(&driver, BIC_DRV8833_MOTOR_B);
            break;
        case 'b':
            status = bic_drv8833_brake(&driver, BIC_DRV8833_MOTOR_A);
            if (status == BIC_DRV8833_OK) status = bic_drv8833_brake(&driver, BIC_DRV8833_MOTOR_B);
            break;
        case '?':
            printf("1/2: motor 1; 3/4: motor 2; s: coast; b: brake\n");
            continue;
        default: continue;
        }
        if (status != BIC_DRV8833_OK) printf("Motor command failed: %d\n", (int)status);
    }
}
