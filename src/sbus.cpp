#include "sbus.h"
#include "config.h"

// UART1 dedicated to SBUS
static HardwareSerial _sbusSerial(1);

static uint8_t  _buf[SBUS_FRAME_SIZE];
static uint8_t  _bufIdx       = 0;
static uint16_t _rawCh[16];
static bool     _failsafe     = false;
static uint32_t _lastFrameMs  = 0;
static uint32_t _frameStartUs = 0;   // micros() when current frame started

void sbusBegin(uint8_t rxPin) {
    // 100 kbaud, 8 data bits, Even parity, 2 stop bits, signal INVERTED
    _sbusSerial.begin(SBUS_BAUD, SERIAL_8E2, rxPin, -1, /*invert=*/true);
    for (int i = 0; i < 16; i++) _rawCh[i] = SBUS_RAW_MID;
}

bool sbusUpdate() {
    bool newFrame = false;

    while (_sbusSerial.available()) {
        uint8_t b = (uint8_t)_sbusSerial.read();

        // Intra-frame timeout: if >4 ms passed since frame started, the byte
        // stream is corrupt or split across frames — reset and wait for fresh start.
        // One full SBUS frame = 25 bytes × 100 µs = 2.5 ms; 4 ms gives margin.
        if (_bufIdx > 0 && (micros() - _frameStartUs) > 4000) {
            _bufIdx = 0;
        }

        // Wait for start byte at index 0
        if (_bufIdx == 0 && b != SBUS_START_BYTE) continue;

        if (_bufIdx == 0) _frameStartUs = micros();  // stamp start of new frame

        _buf[_bufIdx++] = b;
        if (_bufIdx < SBUS_FRAME_SIZE) continue;

        // Full frame received
        _bufIdx = 0;
        if (_buf[24] != SBUS_END_BYTE) continue;  // Discard corrupt frame

        // Unpack 16 × 11-bit channels from bytes 1–22
        _rawCh[0]  = (uint16_t)((_buf[1]        | _buf[2]  <<  8)                     & 0x7FF);
        _rawCh[1]  = (uint16_t)((_buf[2]  >>  3 | _buf[3]  <<  5)                     & 0x7FF);
        _rawCh[2]  = (uint16_t)((_buf[3]  >>  6 | _buf[4]  <<  2 | _buf[5]  << 10)   & 0x7FF);
        _rawCh[3]  = (uint16_t)((_buf[5]  >>  1 | _buf[6]  <<  7)                     & 0x7FF);
        _rawCh[4]  = (uint16_t)((_buf[6]  >>  4 | _buf[7]  <<  4)                     & 0x7FF);
        _rawCh[5]  = (uint16_t)((_buf[7]  >>  7 | _buf[8]  <<  1 | _buf[9]  <<  9)   & 0x7FF);
        _rawCh[6]  = (uint16_t)((_buf[9]  >>  2 | _buf[10] <<  6)                     & 0x7FF);
        _rawCh[7]  = (uint16_t)((_buf[10] >>  5 | _buf[11] <<  3)                     & 0x7FF);
        _rawCh[8]  = (uint16_t)((_buf[12]       | _buf[13] <<  8)                     & 0x7FF);
        _rawCh[9]  = (uint16_t)((_buf[13] >>  3 | _buf[14] <<  5)                     & 0x7FF);
        _rawCh[10] = (uint16_t)((_buf[14] >>  6 | _buf[15] <<  2 | _buf[16] << 10)   & 0x7FF);
        _rawCh[11] = (uint16_t)((_buf[16] >>  1 | _buf[17] <<  7)                     & 0x7FF);
        _rawCh[12] = (uint16_t)((_buf[17] >>  4 | _buf[18] <<  4)                     & 0x7FF);
        _rawCh[13] = (uint16_t)((_buf[18] >>  7 | _buf[19] <<  1 | _buf[20] <<  9)   & 0x7FF);
        _rawCh[14] = (uint16_t)((_buf[20] >>  2 | _buf[21] <<  6)                     & 0x7FF);
        _rawCh[15] = (uint16_t)((_buf[21] >>  5 | _buf[22] <<  3)                     & 0x7FF);

        _failsafe    = (_buf[23] & 0x08) != 0;
        _lastFrameMs = millis();
        newFrame     = true;
    }
    return newFrame;
}

uint16_t sbusGetChannel(uint8_t ch) {
    if (ch >= 16) return RC_MID_US;
    // Linear map: raw 172→1000 µs, 1811→2000 µs
    return (uint16_t)map((long)_rawCh[ch],
                         SBUS_RAW_MIN, SBUS_RAW_MAX,
                         RC_MIN_US,    RC_MAX_US);
}

bool sbusIsFailsafe()  { return _failsafe; }

bool sbusIsConnected() {
    return (millis() - _lastFrameMs) < RC_TIMEOUT_MS;
}
