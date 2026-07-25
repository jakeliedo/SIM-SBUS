#include <Arduino.h>
#include "USB.h"
#include "USBHIDGamepad.h"
#include "config.h"
#include "sbus.h"
#include "ppm.h"
#include "pwm_input.h"
#include "buzzer.h"

// =============================================================
// USB HID Gamepad (TinyUSB, via ESP32-S3 USB-OTG)
// Report layout: x, y, z, rz, rx, ry axes + hat + buttons
// =============================================================
static USBHIDGamepad Gamepad;

// =============================================================
// Application state
// =============================================================
static InputMode_t currentMode   = MODE_SBUS;
static bool        prevConnected = false;

// Mode-button edge detection
static bool     btnWasLow  = false;
static uint32_t btnPressMs = 0;

// PWM pin array (matches config.h order)
static const uint8_t PWM_PINS[PWM_CHANNELS] = {
    PIN_PWM_CH1, PIN_PWM_CH2, PIN_PWM_CH3,
    PIN_PWM_CH4, PIN_PWM_CH5, PIN_PWM_CH6
};

// =============================================================
// Map RC pulse (µs) → signed HID axis (-32767 … +32767)
// =============================================================
static int16_t rcToAxis(uint16_t us) {
    int32_t v = ((int32_t)us - RC_MID_US) * HID_AXIS_MAX / 500;
    return (int16_t)constrain(v, (int32_t)HID_AXIS_MIN, (int32_t)HID_AXIS_MAX);
}

// =============================================================
// Read active-mode channels into ch[]; return connection state
// =============================================================
static bool getChannels(uint16_t ch[RC_CHANNELS]) {
    switch (currentMode) {
        case MODE_SBUS:
            for (int i = 0; i < RC_CHANNELS; i++) ch[i] = sbusGetChannel(i);
            return sbusIsConnected();

        case MODE_PPM:
            for (int i = 0; i < RC_CHANNELS; i++) ch[i] = ppmGetChannel(i);
            return ppmIsConnected();

        case MODE_PWM:
            for (int i = 0; i < RC_CHANNELS; i++) ch[i] = pwmGetChannel(i);
            return pwmIsConnected();

        default:
            for (int i = 0; i < RC_CHANNELS; i++) ch[i] = RC_MID_US;
            return false;
    }
}

// =============================================================
// Mode button — short press cycles SBUS → PPM → PWM → SBUS
// =============================================================
static void handleButton() {
    bool btnLow = (digitalRead(PIN_MODE_BTN) == LOW);
    uint32_t now = millis();

    if (btnLow && !btnWasLow) {
        btnPressMs = now;                             // Record press start
    } else if (!btnLow && btnWasLow) {
        uint32_t held = now - btnPressMs;
        if (held >= BTN_DEBOUNCE_MS && held < BTN_LONG_PRESS_MS) {
            currentMode  = (InputMode_t)((uint8_t)(currentMode + 1) % (uint8_t)MODE_COUNT);
            prevConnected = false;
            buzzerModeChanged();
        }
    }
    btnWasLow = btnLow;
}

// =============================================================
// Setup
// =============================================================
void setup() {
    // Mode button with internal pull-up
    pinMode(PIN_MODE_BTN, INPUT_PULLUP);

    // Buzzer
    buzzerBegin(PIN_BUZZER);

    // RC decoders — all initialised; only active mode is read in loop
    sbusBegin(PIN_SBUS_RX);
    ppmBegin(PIN_PPM_IN);
    pwmBegin(PWM_PINS);

    // USB HID device identity visible in Windows device manager
    USB.manufacturerName("SIM-SBUS");
    USB.productName("RC HID Joystick 6-axis");
    Gamepad.begin();
    USB.begin();

    // Power-on sound (plays while USB enumerates)
    buzzerUsbConnected();
}

// =============================================================
// Main loop
// =============================================================
void loop() {
    static uint32_t lastHidMs = 0;
    uint32_t now = millis();

    // Keep decoders and buzzer ticking
    sbusUpdate();
    ppmUpdate();
    buzzerUpdate();

    // Mode button
    handleButton();

    // Read channels from whichever input is active
    uint16_t ch[RC_CHANNELS];
    bool connected = getChannels(ch);

    // Notify on signal state changes
    if (connected  && !prevConnected) buzzerSignalAcquired();
    if (!connected &&  prevConnected) buzzerSignalLost();
    prevConnected = connected;

    // Send HID report at ≈100 Hz
    // Gamepad.send() returns false silently when USB host is not ready
    if (now - lastHidMs >= 10) {
        lastHidMs = now;

        hid_gamepad_report_t report = {};
        report.x       = rcToAxis(ch[0]);   // CH1 — Aileron
        report.y       = rcToAxis(ch[1]);   // CH2 — Elevator
        report.z       = rcToAxis(ch[2]);   // CH3 — Throttle
        report.rz      = rcToAxis(ch[3]);   // CH4 — Rudder
        report.rx      = rcToAxis(ch[4]);   // CH5 — AUX1
        report.ry      = rcToAxis(ch[5]);   // CH6 — AUX2
        report.hat     = GAMEPAD_HAT_CENTERED;
        report.buttons = 0;

        Gamepad.send(&report);
    }
}
