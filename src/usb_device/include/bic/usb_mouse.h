#ifndef BIC_USB_MOUSE_H
#define BIC_USB_MOUSE_H
#include "bic/usb_device.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BIC_USB_MOUSE_REPORT_SIZE 5
extern const bic_usb_profile_t bic_usb_mouse_profile;
/* Relative movement in -127..127 and USB HID mouse button bitmask. */
bic_usb_status_t bic_usb_mouse_encode(uint8_t buttons, int8_t x, int8_t y,
                                      int8_t wheel, int8_t pan, uint8_t *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
