#pragma once
#include <Arduino.h>

// SBUS protocol constants
#define SBUS_FRAME_SIZE   25
#define SBUS_START_BYTE   0x0F
#define SBUS_END_BYTE     0x00
#define SBUS_BAUD         100000

// Raw 11-bit channel range → pulse width mapping
// 172 = 1000 µs, 992 = 1500 µs, 1811 = 2000 µs
#define SBUS_RAW_MIN      172
#define SBUS_RAW_MID      992
#define SBUS_RAW_MAX      1811

void     sbusBegin(uint8_t rxPin);
bool     sbusUpdate();                    // Call in loop(); returns true on new frame
uint16_t sbusGetChannel(uint8_t ch);     // ch 0-15 → µs (1000–2000)
bool     sbusIsFailsafe();
bool     sbusIsConnected();
