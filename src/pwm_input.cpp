#include "pwm_input.h"
#include "config.h"

// Per-channel state (accessed from ISR — all volatile)
static volatile uint32_t _riseUs[PWM_CHANNELS]        = {};
static volatile uint16_t _pulseUs[PWM_CHANNELS]        = {};
static volatile uint32_t _lastUpdateUs[PWM_CHANNELS]   = {};
static          uint8_t  _pins[PWM_CHANNELS]            = {};

// Shared ISR handler — called by each channel's dedicated wrapper
static void IRAM_ATTR handlePwm(uint8_t idx) {
    uint32_t now = micros();
    if (digitalRead(_pins[idx]) == HIGH) {
        // Rising edge: record start
        _riseUs[idx] = now;
    } else {
        // Falling edge: compute pulse width
        uint32_t dt = now - _riseUs[idx];
        if (dt >= PWM_MIN_US && dt <= PWM_MAX_US) {
            _pulseUs[idx]      = (uint16_t)dt;
            _lastUpdateUs[idx] = now;
        }
    }
}

// One thin wrapper ISR per channel (Arduino requires plain function pointers)
static void IRAM_ATTR isr0() { handlePwm(0); }
static void IRAM_ATTR isr1() { handlePwm(1); }
static void IRAM_ATTR isr2() { handlePwm(2); }
static void IRAM_ATTR isr3() { handlePwm(3); }
static void IRAM_ATTR isr4() { handlePwm(4); }
static void IRAM_ATTR isr5() { handlePwm(5); }

static void (* const _isrTable[PWM_CHANNELS])() = {
    isr0, isr1, isr2, isr3, isr4, isr5
};

void pwmBegin(const uint8_t pins[PWM_CHANNELS]) {
    for (uint8_t i = 0; i < PWM_CHANNELS; i++) {
        _pins[i]          = pins[i];
        _pulseUs[i]       = RC_MID_US;
        _riseUs[i]        = 0;
        _lastUpdateUs[i]  = 0;
        pinMode(pins[i], INPUT);
        attachInterrupt(digitalPinToInterrupt(pins[i]), _isrTable[i], CHANGE);
    }
}

uint16_t pwmGetChannel(uint8_t ch) {
    if (ch >= PWM_CHANNELS) return RC_MID_US;
    return (uint16_t)constrain((int)_pulseUs[ch], RC_MIN_US, RC_MAX_US);
}

bool pwmIsChannelValid(uint8_t ch) {
    if (ch >= PWM_CHANNELS) return false;
    return (micros() - _lastUpdateUs[ch]) < ((uint32_t)RC_TIMEOUT_MS * 1000UL);
}

bool pwmIsConnected() {
    return pwmIsChannelValid(0);
}
