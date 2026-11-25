#include "bic/usb_xinput.h"
#include "bic/usb_dinput.h"
#include "bic/usb_keyboard.h"
#include "bic/usb_mouse.h"
#include "bic/nes_snes.h"

int main(void) {
    bic_nes_snes_reader_t reader = BIC_NES_SNES_READER_INIT;
    bic_nes_snes_emulator_t emulator = BIC_NES_SNES_EMULATOR_INIT;
    uint8_t bytes[32];
    const uint8_t keys[6] = {0};
    bic_usb_xinput_report_t xinput = {0};
    bic_usb_dinput_report_t gamepad = {0};
    gamepad.hat = 8;
    return bic_nes_snes_reader_init(&reader, BIC_NES_SNES_NES, 2, 0, 4) != BIC_NES_SNES_OK ||
           bic_nes_snes_reader_deinit(&reader) != BIC_NES_SNES_OK ||
           bic_nes_snes_emulator_init(&emulator, BIC_NES_SNES_SNES, pio0, 2, 0, 4) != BIC_NES_SNES_OK ||
           bic_nes_snes_emulator_set_buttons(&emulator, BIC_SNES_A) != BIC_NES_SNES_OK ||
           bic_nes_snes_emulator_deinit(&emulator) != BIC_NES_SNES_OK ||
           bic_usb_device_descriptor(&bic_usb_xinput_profile, 0x045e, 0x028e,
                                      bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_xinput_encode(&xinput, bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_dinput_encode(&gamepad, bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_keyboard_encode(0, keys, bytes, sizeof(bytes)) != BIC_USB_OK ||
           bic_usb_mouse_encode(0, 0, 0, 0, 0, bytes, sizeof(bytes)) != BIC_USB_OK ||
           !bic_usb_dinput_profile.configuration || !bic_usb_keyboard_profile.hid_report ||
           !bic_usb_mouse_profile.hid_report;
}
