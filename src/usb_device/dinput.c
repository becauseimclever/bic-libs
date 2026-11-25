#include "bic/usb_dinput.h"
#include "hid_descriptor.h"

static const uint8_t report_descriptor[] = {
    0x05, 0x01, 0x09, 0x05, 0xa1, 0x01,
    0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x09, 0x33, 0x09, 0x34,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x06, 0x81, 0x02,
    0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3b, 0x01,
    0x65, 0x14, 0x75, 0x08, 0x95, 0x01, 0x81, 0x42,
    0x65, 0x00, 0x35, 0x00, 0x45, 0x01,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x20, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x20, 0x81, 0x02, 0xc0
};
static const uint8_t configuration[] = { BIC_USB_HID_CONFIGURATION(report_descriptor) };
const bic_usb_profile_t bic_usb_dinput_profile = {
    0, 0, 0, configuration, sizeof(configuration), report_descriptor, sizeof(report_descriptor)
};

bic_usb_status_t bic_usb_dinput_encode(const bic_usb_dinput_report_t *report,
                                        uint8_t *out, size_t capacity) {
    if (!report || !out || report->hat > 8 || report->x == INT8_MIN || report->y == INT8_MIN ||
        report->z == INT8_MIN || report->rz == INT8_MIN ||
        report->rx == INT8_MIN || report->ry == INT8_MIN)
        return BIC_USB_INVALID_ARGUMENT;
    if (capacity < BIC_USB_DINPUT_REPORT_SIZE) return BIC_USB_BUFFER_TOO_SMALL;
    out[0] = (uint8_t)report->x;
    out[1] = (uint8_t)report->y;
    out[2] = (uint8_t)report->z;
    out[3] = (uint8_t)report->rz;
    out[4] = (uint8_t)report->rx;
    out[5] = (uint8_t)report->ry;
    out[6] = report->hat;
    for (unsigned i = 0; i < 4; ++i) out[7 + i] = (uint8_t)(report->buttons >> (8 * i));
    return BIC_USB_OK;
}
