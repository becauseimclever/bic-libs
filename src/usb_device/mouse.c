/* HID layout adapted from TinyUSB, Copyright (c) 2019 Ha Thach; see third_party/TinyUSB/LICENSE. */
#include "bic/usb_mouse.h"
#include "hid_descriptor.h"

static const uint8_t report_descriptor[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x05, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x05, 0x81, 0x02,
    0x75, 0x03, 0x95, 0x01, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
    0x05, 0x0c, 0x0a, 0x38, 0x02, 0x15, 0x81, 0x25, 0x7f,
    0x75, 0x08, 0x95, 0x01, 0x81, 0x06, 0xc0, 0xc0
};
static const uint8_t configuration[] = { BIC_USB_HID_CONFIGURATION(report_descriptor) };
const bic_usb_profile_t bic_usb_mouse_profile = {
    0, 0, 0, configuration, sizeof(configuration), report_descriptor, sizeof(report_descriptor)
};

bic_usb_status_t bic_usb_mouse_encode(uint8_t buttons, int8_t x, int8_t y,
                                      int8_t wheel, int8_t pan, uint8_t *out, size_t capacity) {
    if (!out || (buttons & ~0x1fu) || x == INT8_MIN || y == INT8_MIN ||
        wheel == INT8_MIN || pan == INT8_MIN) return BIC_USB_INVALID_ARGUMENT;
    if (capacity < BIC_USB_MOUSE_REPORT_SIZE) return BIC_USB_BUFFER_TOO_SMALL;
    out[0] = buttons;
    out[1] = (uint8_t)x;
    out[2] = (uint8_t)y;
    out[3] = (uint8_t)wheel;
    out[4] = (uint8_t)pan;
    return BIC_USB_OK;
}
