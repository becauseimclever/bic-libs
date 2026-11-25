#include <stdio.h>

#include "bic/uln2003.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    bic_uln2003_t motor = BIC_ULN2003_INIT;
    const bic_uln2003_config_t config = {{2, 3, 4, 5}, BIC_ULN2003_HALF_STEP};
    bic_uln2003_status_t status = bic_uln2003_init(&motor, &config);
    if (status != BIC_ULN2003_OK) {
        printf("Stepper initialization failed: %d\n", (int)status);
        return 1;
    }
    printf("28BYJ-48: f/r = move 128 half-steps, s = stop, ? = help\n");
    bool was_moving = false;
    while (true) {
        bool moving;
        status = bic_uln2003_update(&motor, &moving);
        if (status != BIC_ULN2003_OK) {
            printf("Stepper update failed: %d\n", (int)status);
            return 1;
        }
        if (was_moving && !moving) printf("Move complete; coils released\n");
        was_moving = moving;
        int key = getchar_timeout_us(1000);
        if (key == 'f' || key == 'r') {
            status = bic_uln2003_move(&motor, key == 'f' ? 128 : -128, 5000, false);
            if (status != BIC_ULN2003_OK) printf("Cannot start move: %d\n", (int)status);
        } else if (key == 's') {
            status = bic_uln2003_stop(&motor, false);
            if (status != BIC_ULN2003_OK) {
                printf("Cannot stop motor: %d\n", (int)status);
            } else {
                was_moving = false;
                printf("Stopped; coils released\n");
            }
        } else if (key == '?') {
            printf("f/r = move 128 half-steps, s = stop and release\n");
        }
    }
}
