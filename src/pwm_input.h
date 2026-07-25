#pragma once
#include <Arduino.h>

#define PWM_CHANNELS   6
#define PWM_MIN_US     750    // Minimum valid high-pulse width (µs)
#define PWM_MAX_US     2250   // Maximum valid high-pulse width (µs)

// pins[] must be exactly PWM_CHANNELS entries
void     pwmBegin(const uint8_t pins[PWM_CHANNELS]);
uint16_t pwmGetChannel(uint8_t ch);          // µs clamped to 1000–2000
bool     pwmIsChannelValid(uint8_t ch);      // true if recently updated
bool     pwmIsConnected();                   // true if CH1 is valid
