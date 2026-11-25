#include "bic/usb_device.h"

bic_usb_status_t bic_usb_device_descriptor(const bic_usb_profile_t *profile,
                                           uint16_t vendor_id, uint16_t product_id,
                                           uint8_t *out, size_t capacity) {
    if (!profile || !out || !vendor_id || !product_id) return BIC_USB_INVALID_ARGUMENT;
    if (capacity < BIC_USB_DEVICE_DESCRIPTOR_SIZE) return BIC_USB_BUFFER_TOO_SMALL;
    const uint8_t descriptor[BIC_USB_DEVICE_DESCRIPTOR_SIZE] = {
        18, 1, 0x00, 0x02, profile->device_class, profile->device_subclass,
        profile->device_protocol, 64,
        (uint8_t)vendor_id, (uint8_t)(vendor_id >> 8),
        (uint8_t)product_id, (uint8_t)(product_id >> 8),
        0x00, 0x01, 0, 0, 0, 1
    };
    for (size_t i = 0; i < sizeof(descriptor); ++i) out[i] = descriptor[i];
    return BIC_USB_OK;
}
