#pragma once
#include "soc/soc_caps.h"
#if SOC_USB_OTG_SUPPORTED

#include "USBHID.h"
#if CONFIG_TINYUSB_HID_ENABLED

// Hat switch: 0-7 = 8 directions, value outside that range (Null State) = centered
#define JOYSTICK16_HAT_CENTER 15

// Custom 16-bit-resolution USB HID Joystick (6 axes + 16 buttons + hat)
// Standard USBHIDGamepad only supports int8_t axes (-127..127); this class
// uses a raw TinyUSB HID report descriptor with int16_t axes for smoother
// flight-sim control resolution (65536 steps vs 256).
class HIDJoystick16 : public USBHIDDevice {
private:
    USBHID hid;

public:
    HIDJoystick16();
    void begin();
    void end();

    // x, y, z, rx, ry, rz: -32767 .. 32767
    // buttons: bit i = button i+1 pressed
    // hat: 0-7 = direction, JOYSTICK16_HAT_CENTER = centered/no input
    bool send(
        int16_t x, int16_t y, int16_t z,
        int16_t rx, int16_t ry, int16_t rz,
        uint16_t buttons = 0, uint8_t hat = JOYSTICK16_HAT_CENTER
    );

    // internal use
    uint16_t _onGetDescriptor(uint8_t *buffer) override;
};

#endif /* CONFIG_TINYUSB_HID_ENABLED */
#endif /* SOC_USB_OTG_SUPPORTED */
