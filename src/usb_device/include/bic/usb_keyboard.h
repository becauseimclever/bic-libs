#ifndef BIC_USB_KEYBOARD_H
#define BIC_USB_KEYBOARD_H
#include "bic/usb_device.h"
#ifdef __cplusplus
extern "C" {
#endif
#define BIC_USB_KEYBOARD_REPORT_SIZE 8
extern const bic_usb_profile_t bic_usb_keyboard_profile;
/* Six USB HID key usages; use zero for empty slots. */
bic_usb_status_t bic_usb_keyboard_encode(uint8_t modifiers, const uint8_t keys[6],
                                          uint8_t *out, size_t capacity);
bic_usb_status_t bic_usb_keyboard_decode_leds(const uint8_t *packet, size_t length,
                                               uint8_t *leds);
#ifdef __cplusplus
}
#endif
#endif
