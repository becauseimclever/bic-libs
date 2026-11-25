#ifndef BIC_USB_XINPUT_H
#define BIC_USB_XINPUT_H

#include "bic/usb_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Microsoft's Xbox 360 wired controller IDs: for development/testing only. */
#define BIC_USB_XINPUT_TEST_VID 0x045e
#define BIC_USB_XINPUT_TEST_PID 0x028e
#define BIC_USB_XINPUT_REPORT_SIZE 20

enum {
    BIC_XINPUT_UP = 0x0001, BIC_XINPUT_DOWN = 0x0002,
    BIC_XINPUT_LEFT = 0x0004, BIC_XINPUT_RIGHT = 0x0008,
    BIC_XINPUT_START = 0x0010, BIC_XINPUT_BACK = 0x0020,
    BIC_XINPUT_LS = 0x0040, BIC_XINPUT_RS = 0x0080,
    BIC_XINPUT_LB = 0x0100, BIC_XINPUT_RB = 0x0200,
    BIC_XINPUT_GUIDE = 0x0400, BIC_XINPUT_A = 0x1000,
    BIC_XINPUT_B = 0x2000, BIC_XINPUT_X = 0x4000,
    BIC_XINPUT_Y = 0x8000
};

typedef struct {
    uint16_t buttons;
    uint8_t left_trigger, right_trigger;
    int16_t left_x, left_y, right_x, right_y;
} bic_usb_xinput_report_t;

typedef enum {
    BIC_XINPUT_OUTPUT_RUMBLE,
    BIC_XINPUT_OUTPUT_LED
} bic_usb_xinput_output_type_t;

typedef struct {
    bic_usb_xinput_output_type_t type;
    uint8_t left_motor, right_motor, led;
} bic_usb_xinput_output_t;

extern const bic_usb_profile_t bic_usb_xinput_profile;
bic_usb_status_t bic_usb_xinput_encode(const bic_usb_xinput_report_t *report,
                                       uint8_t *out, size_t capacity);
bic_usb_status_t bic_usb_xinput_decode_output(const uint8_t *packet, size_t length,
                                              bic_usb_xinput_output_t *output);

#ifdef __cplusplus
}
#endif
#endif
