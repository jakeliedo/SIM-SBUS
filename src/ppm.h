#pragma once
#include <Arduino.h>

#define PPM_MAX_CHANNELS  12
#define PPM_SYNC_US       3500   // Rising-to-rising gap > this = sync pulse
#define PPM_MIN_US        750    // Minimum accepted channel period (µs)
#define PPM_MAX_US        2250   // Maximum accepted channel period (µs)

void     ppmBegin(uint8_t pin);
bool     ppmUpdate();                    // Returns true when a new frame arrived
uint16_t ppmGetChannel(uint8_t ch);     // ch 0-based → µs clamped to 1000–2000
uint8_t  ppmGetChannelCount();
bool     ppmIsConnected();
