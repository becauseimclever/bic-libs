#include "bic/usb_xinput.h"
#include "bic/usb_dinput.h"
#include "bic/usb_keyboard.h"
#include "bic/usb_mouse.h"

int main(void) {
    uint8_t bytes[32];
    const uint8_t keys[6] = {0};
    bic_usb_xinput_report_t xinput = {0};
    bic_usb_dinput_report_t gamepad = {0};
    gamepad.hat = 8;
    return bic_usb_device_descriptor(&bic_usb_xinput_profile, 0x045e, 0x028e,
                                      bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_xinput_encode(&xinput, bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_dinput_encode(&gamepad, bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_keyboard_encode(0, keys, bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_mouse_encode(0, 0, 0, 0, 0, bytes, sizeof(bytes)) != BIC_USB_OK ||
           !bic_usb_dinput_profile.configuration || !bic_usb_keyboard_profile.hid_report ||
           !bic_usb_mouse_profile.hid_report;
}
