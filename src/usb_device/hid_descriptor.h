#ifndef BIC_USB_HID_DESCRIPTOR_H
#define BIC_USB_HID_DESCRIPTOR_H

#define BIC_USB_HID_CONFIGURATION(report) \
    9, 2, 34, 0, 1, 1, 0, 0x80, 50, \
    9, 4, 0, 0, 1, 3, 0, 0, 0, \
    9, 0x21, 0x11, 0x01, 0, 1, 0x22, \
    (uint8_t)(sizeof(report) & 0xff), (uint8_t)(sizeof(report) >> 8), \
    7, 5, 0x81, 3, 64, 0, 1

#endif
