#include "hid_joystick16.h"
#if SOC_USB_OTG_SUPPORTED
#if CONFIG_TINYUSB_HID_ENABLED

#include <string.h>

#define REPORT_ID_JOYSTICK16 1

// Report body sent via hid.SendReport() — report ID byte is prepended
// automatically by TinyUSB, so it is NOT included in this struct.
typedef struct __attribute__((packed)) {
    int16_t  x, y, z, rx, ry, rz;  // 6 axes x 16-bit = 12 bytes
    uint16_t buttons;              // 16 buttons     = 2 bytes
    uint8_t  hat;                  // 4-bit hat + 4-bit padding = 1 byte
} hid_joystick16_report_t;         // total 15 bytes

// Raw USB HID report descriptor: Joystick, 6 x 16-bit axes, 16 buttons, 1 hat
static const uint8_t report_descriptor[] = {
    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x04,                    // Usage (Joystick)
    0xA1, 0x01,                    // Collection (Application)
        0x85, REPORT_ID_JOYSTICK16,//   Report ID
        0x05, 0x01,                //   Usage Page (Generic Desktop)
        0x09, 0x30,                //   Usage (X)
        0x09, 0x31,                //   Usage (Y)
        0x09, 0x32,                //   Usage (Z)
        0x09, 0x33,                //   Usage (Rx)
        0x09, 0x34,                //   Usage (Ry)
        0x09, 0x35,                //   Usage (Rz)
        0x16, 0x01, 0x80,          //   Logical Minimum (-32767)
        0x26, 0xFF, 0x7F,          //   Logical Maximum (32767)
        0x75, 0x10,                //   Report Size (16)
        0x95, 0x06,                //   Report Count (6)
        0x81, 0x02,                //   Input (Data,Var,Abs)

        0x05, 0x09,                //   Usage Page (Button)
        0x19, 0x01,                //   Usage Minimum (Button 1)
        0x29, 0x10,                //   Usage Maximum (Button 16)
        0x15, 0x00,                //   Logical Minimum (0)
        0x25, 0x01,                //   Logical Maximum (1)
        0x75, 0x01,                //   Report Size (1)
        0x95, 0x10,                //   Report Count (16)
        0x81, 0x02,                //   Input (Data,Var,Abs)

        0x05, 0x01,                //   Usage Page (Generic Desktop)
        0x09, 0x39,                //   Usage (Hat switch)
        0x15, 0x00,                //   Logical Minimum (0)
        0x25, 0x07,                //   Logical Maximum (7)
        0x35, 0x00,                //   Physical Minimum (0)
        0x46, 0x3B, 0x01,          //   Physical Maximum (315)
        0x65, 0x14,                //   Unit (Eng Rot: Degrees)
        0x75, 0x04,                //   Report Size (4)
        0x95, 0x01,                //   Report Count (1)
        0x81, 0x42,                //   Input (Data,Var,Abs,Null State)

        0x65, 0x00,                //   Unit (None)
        0x75, 0x04,                //   Report Size (4)  -- padding
        0x95, 0x01,                //   Report Count (1)
        0x81, 0x03,                //   Input (Const,Var,Abs)
    0xC0                            // End Collection
};

HIDJoystick16::HIDJoystick16() : hid() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

uint16_t HIDJoystick16::_onGetDescriptor(uint8_t *dst) {
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void HIDJoystick16::begin() { hid.begin(); }
void HIDJoystick16::end()   {}

bool HIDJoystick16::send(
    int16_t x, int16_t y, int16_t z,
    int16_t rx, int16_t ry, int16_t rz,
    uint16_t buttons, uint8_t hat
) {
    hid_joystick16_report_t report = { x, y, z, rx, ry, rz, buttons, hat };
    return hid.SendReport(REPORT_ID_JOYSTICK16, &report, sizeof(report));
}

#endif /* CONFIG_TINYUSB_HID_ENABLED */
#endif /* SOC_USB_OTG_SUPPORTED */
