#ifndef BIC_USB_DEVICE_H
#define BIC_USB_DEVICE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BIC_USB_DEVICE_DESCRIPTOR_SIZE 18

typedef enum {
    BIC_USB_OK = 0,
    BIC_USB_INVALID_ARGUMENT,
    BIC_USB_BUFFER_TOO_SMALL,
    BIC_USB_UNSUPPORTED
} bic_usb_status_t;

typedef struct {
    uint8_t device_class, device_subclass, device_protocol;
    const uint8_t *configuration;
    size_t configuration_length;
    const uint8_t *hid_report;
    size_t hid_report_length;
} bic_usb_profile_t;

/* Build a device descriptor with no string indices and the supplied USB IDs. */
bic_usb_status_t bic_usb_device_descriptor(const bic_usb_profile_t *profile,
                                           uint16_t vendor_id, uint16_t product_id,
                                           uint8_t *out, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
