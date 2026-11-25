/* HID layout adapted from TinyUSB, Copyright (c) 2019 Ha Thach; see third_party/TinyUSB/LICENSE. */
#include "bic/usb_keyboard.h"
#include "hid_descriptor.h"

static const uint8_t report_descriptor[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07,
    0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x01,
    0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x75, 0x01, 0x95, 0x05, 0x91, 0x02,
    0x75, 0x03, 0x95, 0x01, 0x91, 0x01,
    0x05, 0x07, 0x19, 0x00, 0x2a, 0xff, 0x00, 0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, 0x06, 0x81, 0x00, 0xc0
};
static const uint8_t configuration[] = { BIC_USB_HID_CONFIGURATION(report_descriptor) };
const bic_usb_profile_t bic_usb_keyboard_profile = {
    0, 0, 0, configuration, sizeof(configuration), report_descriptor, sizeof(report_descriptor)
};

bic_usb_status_t bic_usb_keyboard_encode(uint8_t modifiers, const uint8_t keys[6],
                                          uint8_t *out, size_t capacity) {
    if (!keys || !out) return BIC_USB_INVALID_ARGUMENT;
    if (capacity < BIC_USB_KEYBOARD_REPORT_SIZE) return BIC_USB_BUFFER_TOO_SMALL;
    out[0] = modifiers;
    out[1] = 0;
    for (unsigned i = 0; i < 6; ++i) out[i + 2] = keys[i];
    return BIC_USB_OK;
}

bic_usb_status_t bic_usb_keyboard_decode_leds(const uint8_t *packet, size_t length,
                                               uint8_t *leds) {
    if (!packet || !leds) return BIC_USB_INVALID_ARGUMENT;
    if (length < 1) return BIC_USB_BUFFER_TOO_SMALL;
    *leds = packet[0] & 0x1f;
    return BIC_USB_OK;
}
