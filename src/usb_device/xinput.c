/*
 * XInput wired interface and report layout derived from GP2040-CE
 * (Copyright 2021 Jason Skuby; MIT; see third_party/GP2040-CE/LICENSE).
 */
#include "bic/usb_xinput.h"

#include <string.h>

static const uint8_t configuration[] = {
    9, 2, 0x99, 0, 4, 1, 0, 0x80, 50,
    9, 4, 0, 0, 2, 0xff, 0x5d, 1, 0,
    0x11, 0x21, 0, 1, 1, 0x25, 0x81, 0x14, 0, 0, 0, 0, 0x13, 0x02, 0x08, 0, 0,
    7, 5, 0x81, 3, 0x20, 0, 1,
    7, 5, 0x02, 3, 0x20, 0, 8,
    9, 4, 1, 0, 4, 0xff, 0x5d, 3, 0,
    0x1b, 0x21, 0, 1, 1, 1, 0x83, 0x40, 1, 0x04, 0x20, 0x16, 0x85,
    0, 0, 0, 0, 0, 0, 0x16, 0x06, 0, 0, 0, 0, 0, 0,
    7, 5, 0x83, 3, 0x20, 0, 2,
    7, 5, 0x04, 3, 0x20, 0, 4,
    7, 5, 0x85, 3, 0x20, 0, 0x40,
    7, 5, 0x06, 3, 0x20, 0, 0x10,
    9, 4, 2, 0, 1, 0xff, 0x5d, 2, 0,
    9, 0x21, 0, 1, 1, 0x22, 0x86, 3, 0,
    7, 5, 0x86, 3, 0x20, 0, 0x10,
    9, 4, 3, 0, 0, 0xff, 0xfd, 0x13, 0,
    6, 0x41, 0, 1, 1, 3
};
_Static_assert(sizeof(configuration) == 0x99, "Incorrect XInput descriptor length");

const bic_usb_profile_t bic_usb_xinput_profile = {
    0xff, 0xff, 0xff, configuration, sizeof(configuration), NULL, 0
};

static void write16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

bic_usb_status_t bic_usb_xinput_encode(const bic_usb_xinput_report_t *report,
                                       uint8_t *out, size_t capacity) {
    if (!report || !out) return BIC_USB_INVALID_ARGUMENT;
    if (capacity < BIC_USB_XINPUT_REPORT_SIZE) return BIC_USB_BUFFER_TOO_SMALL;
    memset(out, 0, BIC_USB_XINPUT_REPORT_SIZE);
    out[1] = BIC_USB_XINPUT_REPORT_SIZE;
    write16(out + 2, report->buttons);
    out[4] = report->left_trigger;
    out[5] = report->right_trigger;
    write16(out + 6, (uint16_t)report->left_x);
    write16(out + 8, (uint16_t)report->left_y);
    write16(out + 10, (uint16_t)report->right_x);
    write16(out + 12, (uint16_t)report->right_y);
    return BIC_USB_OK;
}

bic_usb_status_t bic_usb_xinput_decode_output(const uint8_t *packet, size_t length,
                                              bic_usb_xinput_output_t *output) {
    if (!packet || !output) return BIC_USB_INVALID_ARGUMENT;
    if (length < 2) return BIC_USB_BUFFER_TOO_SMALL;
    if (packet[0] == 0 && packet[1] == 8) {
        if (length < 8) return BIC_USB_BUFFER_TOO_SMALL;
        output->type = BIC_XINPUT_OUTPUT_RUMBLE;
        output->left_motor = packet[3];
        output->right_motor = packet[4];
        output->led = 0;
        return BIC_USB_OK;
    }
    if (packet[0] == 1 && packet[1] == 3) {
        if (length < 3) return BIC_USB_BUFFER_TOO_SMALL;
        output->type = BIC_XINPUT_OUTPUT_LED;
        output->left_motor = output->right_motor = 0;
        output->led = packet[2];
        return BIC_USB_OK;
    }
    return BIC_USB_UNSUPPORTED;
}
