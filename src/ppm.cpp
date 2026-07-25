#include "ppm.h"
#include "config.h"

// All ISR-shared variables are volatile
static volatile uint32_t _risingUs                     = 0;
static volatile bool     _risingValid                  = false;
static volatile uint16_t _channels[PPM_MAX_CHANNELS]   = {};
static volatile uint8_t  _chIdx                        = 0;
static volatile uint8_t  _chCount                      = 0;
static volatile uint32_t _lastFrameUs                  = 0;
static volatile bool     _newFrame                     = false;

static void IRAM_ATTR ppmISR() {
    uint32_t now = micros();

    if (!_risingValid) {
        // First edge after init — start timing from here
        _risingValid = true;
        _risingUs    = now;
        return;
    }

    uint32_t dt = now - _risingUs;
    _risingUs   = now;

    if (dt > PPM_SYNC_US) {
        // Sync gap: end of frame
        if (_chIdx > 0) {
            _chCount      = _chIdx;
            _lastFrameUs  = now;
            _newFrame     = true;
        }
        _chIdx = 0;
    } else if (dt >= PPM_MIN_US && dt <= PPM_MAX_US) {
        // Valid channel period
        if (_chIdx < PPM_MAX_CHANNELS) {
            _channels[_chIdx++] = (uint16_t)dt;
        }
    }
    // Pulses shorter than PPM_MIN_US are the short sync pulses (≈0.3 ms); ignore
}

void ppmBegin(uint8_t pin) {
    pinMode(pin, INPUT);
    for (int i = 0; i < PPM_MAX_CHANNELS; i++) _channels[i] = RC_MID_US;
    attachInterrupt(digitalPinToInterrupt(pin), ppmISR, RISING);
}

bool ppmUpdate() {
    if (!_newFrame) return false;
    _newFrame = false;
    return true;
}

uint16_t ppmGetChannel(uint8_t ch) {
    if (ch >= PPM_MAX_CHANNELS) return RC_MID_US;
    return (uint16_t)constrain((int)_channels[ch], RC_MIN_US, RC_MAX_US);
}

uint8_t ppmGetChannelCount() { return _chCount; }

bool ppmIsConnected() {
    return (micros() - _lastFrameUs) < ((uint32_t)RC_TIMEOUT_MS * 1000UL);
}
