#ifndef BIC_USB_DINPUT_H
#define BIC_USB_DINPUT_H
#include "bic/usb_device.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BIC_USB_DINPUT_REPORT_SIZE 11
typedef struct {
    int8_t x, y, z, rz, rx, ry; /* -127..127 */
    uint8_t hat; /* 0..7 = up clockwise, 8 = center */
    uint32_t buttons;
} bic_usb_dinput_report_t;
extern const bic_usb_profile_t bic_usb_dinput_profile;
bic_usb_status_t bic_usb_dinput_encode(const bic_usb_dinput_report_t *report,
                                        uint8_t *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
