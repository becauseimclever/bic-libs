#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "bic/usb_device.h"
#include "bic/usb_dinput.h"
#include "bic/usb_keyboard.h"
#include "bic/usb_mouse.h"
#include "bic/usb_xinput.h"

static void check_configuration(const bic_usb_profile_t *profile, unsigned interfaces,
                                unsigned endpoints) {
    const uint8_t *p = profile->configuration;
    size_t length = profile->configuration_length;
    assert(length >= 9 && p[0] == 9 && p[1] == 2);
    assert(((size_t)p[2] | ((size_t)p[3] << 8)) == length && p[4] == interfaces);
    unsigned seen_interfaces = 0, seen_endpoints = 0, expected_endpoints = 0;
    for (size_t pos = 9; pos < length;) {
        assert(pos + 2 <= length);
        size_t n = p[pos];
        assert(n >= 2 && pos + n <= length);
        if (p[pos + 1] == 4) {
            assert(n == 9);
            assert(seen_endpoints == expected_endpoints);
            expected_endpoints += p[pos + 4];
            seen_interfaces++;
        } else if (p[pos + 1] == 5) {
            assert(n == 7);
            seen_endpoints++;
        } else if (p[pos + 1] == 0x21 && profile->hid_report) {
            assert(n == 9);
            assert(((size_t)p[pos + 7] | ((size_t)p[pos + 8] << 8)) ==
                   profile->hid_report_length);
        }
        pos += n;
    }
    assert(seen_interfaces == interfaces && seen_endpoints == endpoints);
    assert(seen_endpoints == expected_endpoints);
}

static void check_hid_report(const bic_usb_profile_t *profile, unsigned input_bits,
                             unsigned output_bits, bool expect_null_hat) {
    const uint8_t *p = profile->hid_report;
    size_t length = profile->hid_report_length;
    unsigned size = 0, count = 0, inputs = 0, outputs = 0, depth = 0;
    unsigned page = 0, usage = 0, input_fields = 0;
    int logical_min = 0, logical_max = 0;
    bool saw_null = false;
    assert(p && length);
    for (size_t pos = 0; pos < length;) {
        uint8_t prefix = p[pos++];
        assert(prefix != 0xfe);
        unsigned bytes = prefix & 3;
        if (bytes == 3) bytes = 4;
        assert(pos + bytes <= length);
        unsigned value = 0;
        for (unsigned i = 0; i < bytes; ++i) value |= (unsigned)p[pos + i] << (8 * i);
        pos += bytes;
        unsigned type = (prefix >> 2) & 3, tag = prefix >> 4;
        if (type == 1 && tag == 0) page = value;
        if (type == 1 && tag == 1) logical_min = bytes == 1 ? (int8_t)value : (int)value;
        if (type == 1 && tag == 2) logical_max = logical_min < 0 && bytes == 1
                                                     ? (int8_t)value : (int)value;
        if (type == 1 && tag == 7) size = value;
        if (type == 1 && tag == 9) count = value;
        if (type == 1 && tag == 8) assert(!"Unexpected report ID");
        if (type == 2 && tag == 0) usage = value;
        if (type == 0 && tag == 8) {
            inputs += size * count;
            if (value & 0x40) saw_null = true;
            if (profile == &bic_usb_dinput_profile) {
                if (input_fields == 0)
                    assert(page == 1 && size == 8 && count == 6 &&
                           logical_min == -127 && logical_max == 127);
                else if (input_fields == 1)
                    assert(page == 1 && usage == 0x39 && size == 8 && count == 1 &&
                           logical_min == 0 && logical_max == 7 && (value & 0x40));
                else if (input_fields == 2)
                    assert(page == 9 && size == 1 && count == 32);
                else assert(!"Unexpected gamepad input");
            } else if (profile == &bic_usb_keyboard_profile) {
                if (input_fields == 0) assert(page == 7 && size == 1 && count == 8);
                else if (input_fields == 1) assert(size == 8 && count == 1 && (value & 1));
                else if (input_fields == 2) assert(page == 7 && size == 8 && count == 6);
                else assert(!"Unexpected keyboard input");
            } else if (profile == &bic_usb_mouse_profile) {
                if (input_fields == 0) assert(page == 9 && size == 1 && count == 5);
                else if (input_fields == 1) assert(size == 3 && count == 1 && (value & 1));
                else if (input_fields == 2)
                    assert(page == 1 && size == 8 && count == 3 && (value & 4) &&
                           logical_min == -127 && logical_max == 127);
                else if (input_fields == 3)
                    assert(page == 12 && usage == 0x238 && size == 8 && count == 1 && (value & 4));
                else assert(!"Unexpected mouse input");
            }
            input_fields++;
        }
        if (type == 0 && tag == 9) outputs += size * count;
        if (type == 0 && tag == 10) depth++;
        if (type == 0 && tag == 12) { assert(depth); depth--; }
    }
    assert(!depth && inputs == input_bits && outputs == output_bits);
    assert(saw_null == expect_null_hat);
}

int main(void) {
    uint8_t buf[32], unchanged[32];
    memset(buf, 0xa5, sizeof(buf));
    memcpy(unchanged, buf, sizeof(buf));
    assert(bic_usb_device_descriptor(NULL, 1, 2, buf, sizeof(buf)) == BIC_USB_INVALID_ARGUMENT);
    assert(bic_usb_device_descriptor(&bic_usb_xinput_profile, 1, 2, buf, 17) ==
           BIC_USB_BUFFER_TOO_SMALL);
    assert(!memcmp(buf, unchanged, sizeof(buf)));
    assert(bic_usb_device_descriptor(&bic_usb_xinput_profile, BIC_USB_XINPUT_TEST_VID,
                                     BIC_USB_XINPUT_TEST_PID, buf, sizeof(buf)) == BIC_USB_OK);
    assert(buf[0] == 18 && buf[1] == 1 && buf[4] == 0xff && buf[7] == 64);
    assert(buf[8] == 0x5e && buf[9] == 0x04 && buf[10] == 0x8e && buf[11] == 0x02);
    assert(buf[14] == 0 && buf[15] == 0 && buf[16] == 0 && buf[17] == 1);
    assert(bic_usb_device_descriptor(&bic_usb_keyboard_profile, 0xcafe, 0x1234,
                                     buf, sizeof(buf)) == BIC_USB_OK);
    assert(buf[4] == 0 && buf[8] == 0xfe && buf[9] == 0xca);

    check_configuration(&bic_usb_xinput_profile, 4, 7);
    check_configuration(&bic_usb_dinput_profile, 1, 1);
    check_configuration(&bic_usb_keyboard_profile, 1, 1);
    check_configuration(&bic_usb_mouse_profile, 1, 1);
    assert(!bic_usb_xinput_profile.hid_report && !bic_usb_xinput_profile.hid_report_length);
    check_hid_report(&bic_usb_dinput_profile, 88, 0, true);
    check_hid_report(&bic_usb_keyboard_profile, 64, 8, false);
    check_hid_report(&bic_usb_mouse_profile, 40, 0, false);

    bic_usb_xinput_report_t xinput = {
        BIC_XINPUT_A | BIC_XINPUT_UP, 0x55, 0xaa, -32768, 32767, -1, 1
    };
    const uint8_t expected_xinput[20] = {
        0, 20, 1, 0x10, 0x55, 0xaa, 0, 0x80, 0xff, 0x7f,
        0xff, 0xff, 1, 0, 0, 0, 0, 0, 0, 0
    };
    assert(bic_usb_xinput_encode(&xinput, buf, 19) == BIC_USB_BUFFER_TOO_SMALL);
    assert(bic_usb_xinput_encode(&xinput, buf, sizeof(buf)) == BIC_USB_OK);
    assert(!memcmp(buf, expected_xinput, sizeof(expected_xinput)));
    bic_usb_xinput_output_t output;
    const uint8_t rumble[] = {0, 8, 0, 10, 20, 0, 0, 0};
    const uint8_t led[] = {1, 3, 7};
    assert(bic_usb_xinput_decode_output(rumble, 7, &output) == BIC_USB_BUFFER_TOO_SMALL);
    assert(bic_usb_xinput_decode_output(rumble, sizeof(rumble), &output) == BIC_USB_OK);
    assert(output.type == BIC_XINPUT_OUTPUT_RUMBLE && output.left_motor == 10 &&
           output.right_motor == 20);
    assert(bic_usb_xinput_decode_output(led, sizeof(led), &output) == BIC_USB_OK);
    assert(output.type == BIC_XINPUT_OUTPUT_LED && output.led == 7);
    const uint8_t unknown[] = {2, 1};
    assert(bic_usb_xinput_decode_output(unknown, sizeof(unknown), &output) == BIC_USB_UNSUPPORTED);

    bic_usb_dinput_report_t gamepad = {-127, 127, 0, -1, 1, 2, 8, 0x12345678};
    const uint8_t expected_gamepad[11] = {0x81, 0x7f, 0, 0xff, 1, 2, 8, 0x78, 0x56, 0x34, 0x12};
    assert(bic_usb_dinput_encode(&gamepad, buf, sizeof(buf)) == BIC_USB_OK);
    assert(!memcmp(buf, expected_gamepad, sizeof(expected_gamepad)));
    gamepad.hat = 9;
    assert(bic_usb_dinput_encode(&gamepad, buf, sizeof(buf)) == BIC_USB_INVALID_ARGUMENT);
    gamepad.hat = 8; gamepad.x = INT8_MIN;
    assert(bic_usb_dinput_encode(&gamepad, buf, sizeof(buf)) == BIC_USB_INVALID_ARGUMENT);

    const uint8_t keys[6] = {4, 5, 0, 0, 0, 0};
    const uint8_t expected_keyboard[8] = {2, 0, 4, 5, 0, 0, 0, 0};
    assert(bic_usb_keyboard_encode(2, keys, buf, 7) == BIC_USB_BUFFER_TOO_SMALL);
    assert(bic_usb_keyboard_encode(2, keys, buf, sizeof(buf)) == BIC_USB_OK);
    assert(!memcmp(buf, expected_keyboard, sizeof(expected_keyboard)));
    uint8_t leds;
    const uint8_t led_report[] = {0xff};
    assert(bic_usb_keyboard_decode_leds(led_report, 0, &leds) == BIC_USB_BUFFER_TOO_SMALL);
    assert(bic_usb_keyboard_decode_leds(led_report, 1, &leds) == BIC_USB_OK && leds == 0x1f);

    const uint8_t expected_mouse[] = {3, 0x81, 0x7f, 0xff, 1};
    assert(bic_usb_mouse_encode(3, -127, 127, -1, 1, buf, sizeof(buf)) == BIC_USB_OK);
    assert(!memcmp(buf, expected_mouse, sizeof(expected_mouse)));
    assert(bic_usb_mouse_encode(0x20, 0, 0, 0, 0, buf, sizeof(buf)) == BIC_USB_INVALID_ARGUMENT);
    assert(bic_usb_mouse_encode(0, INT8_MIN, 0, 0, 0, buf, sizeof(buf)) == BIC_USB_INVALID_ARGUMENT);
    return 0;
}
