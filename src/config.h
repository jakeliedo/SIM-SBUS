#pragma once

// =============================================================
// Pin Definitions — ESP32-S3 Black Gold
// Adjust to match your specific board's silk-screen labels.
// =============================================================

// ── Signal inputs ─────────────────────────────────────────────
#define PIN_SBUS_RX      4    // UART1 RX  — inverted SBUS (100 kbaud 8E2)
#define PIN_PPM_IN       5    // GPIO ISR  — PPM sum signal (rising edge)
#define PIN_PWM_CH1      6    // GPIO ISR  — PWM channel 1  (CHANGE)
#define PIN_PWM_CH2      7    // GPIO ISR  — PWM channel 2
#define PIN_PWM_CH3      8    // GPIO ISR  — PWM channel 3
#define PIN_PWM_CH4      9    // GPIO ISR  — PWM channel 4
#define PIN_PWM_CH5      10   // GPIO ISR  — PWM channel 5
#define PIN_PWM_CH6      11   // GPIO ISR  — PWM channel 6

// ── Outputs / UI ──────────────────────────────────────────────
#define PIN_BUZZER       2    // Passive buzzer via LEDC PWM
#define PIN_MODE_BTN     0    // Mode-select button (active LOW, boot button)

// ── USB OTG — fixed by hardware, do NOT reassign ─────────────
//    GPIO19 = D−,  GPIO20 = D+   (connected to USB-C OTG port)

// =============================================================
// Input modes
// =============================================================
typedef enum : uint8_t {
    MODE_SBUS  = 0,
    MODE_PPM   = 1,
    MODE_PWM   = 2,
    MODE_COUNT = 3
} InputMode_t;

static const char* const MODE_NAMES[MODE_COUNT] = {
    "SBUS", "PPM", "PWM"
};

// =============================================================
// RC channel parameters
// =============================================================
#define RC_CHANNELS        6      // Channels forwarded to HID axes
#define RC_SBUS_CHANNELS   16     // Total SBUS channels available

#define RC_MIN_US          1000   // Min pulse width (µs)
#define RC_MID_US          1500   // Center pulse width (µs)
#define RC_MAX_US          2000   // Max pulse width (µs)

#define RC_TIMEOUT_MS      500    // Signal-lost if no frame for this long

// =============================================================
// HID axis output range (signed 16-bit)
// =============================================================
#define HID_AXIS_MIN       (-32767)
#define HID_AXIS_MAX       ( 32767)

// =============================================================
// Button debounce / long-press thresholds
// =============================================================
#define BTN_DEBOUNCE_MS    50
#define BTN_LONG_PRESS_MS  1000
