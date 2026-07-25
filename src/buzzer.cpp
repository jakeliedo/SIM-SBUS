#include "buzzer.h"

// Non-blocking beep sequence queue
struct BeepStep { uint16_t freq; uint16_t ms; };

static const uint8_t QUEUE_CAP = 12;
static BeepStep _queue[QUEUE_CAP];
static uint8_t  _qHead  = 0;
static uint8_t  _qTail  = 0;
static uint8_t  _pin    = 0;
static uint32_t _endMs  = 0;
static bool     _active = false;

// ── Internals ─────────────────────────────────────────────────

static void enqueue(uint16_t freq, uint16_t ms) {
    uint8_t next = (_qTail + 1) % QUEUE_CAP;
    if (next == _qHead) return;   // Queue full — drop step
    _queue[_qTail] = {freq, ms};
    _qTail = next;
}

static void playNext() {
    if (_qHead == _qTail) { _active = false; return; }
    BeepStep s = _queue[_qHead];
    _qHead = (_qHead + 1) % QUEUE_CAP;
    if (s.freq > 0) ledcWriteTone(_pin, s.freq);
    else            ledcWrite(_pin, 0);
    _endMs  = millis() + s.ms;
    _active = true;
}

static void schedule(const BeepStep* steps, uint8_t n) {
    // Cancel current playback and reset queue
    ledcWrite(_pin, 0);
    _qHead = _qTail = 0;
    _active = false;
    for (uint8_t i = 0; i < n; i++) enqueue(steps[i].freq, steps[i].ms);
    playNext();
}

// ── Public API ────────────────────────────────────────────────

void buzzerBegin(uint8_t pin) {
    _pin = pin;
    pinMode(pin, OUTPUT);
    // Attach LEDC at a default frequency; ledcWriteTone() will change it
    ledcAttach(pin, 2000, 8);
    ledcWrite(pin, 0);
}

void buzzerUpdate() {
    if (!_active) return;
    if (millis() >= _endMs) {
        ledcWrite(_pin, 0);
        playNext();
    }
}

bool buzzerIsPlaying() { return _active || (_qHead != _qTail); }

// ── Predefined sounds ─────────────────────────────────────────

void buzzerSignalAcquired() {
    static const BeepStep s[] = {
        {800,  80}, {0, 40}, {1200, 100}
    };
    schedule(s, 3);
}

void buzzerSignalLost() {
    static const BeepStep s[] = {
        {400, 600}
    };
    schedule(s, 1);
}

void buzzerModeChanged() {
    static const BeepStep s[] = {
        {1000, 80}, {0, 60}, {1000, 80}
    };
    schedule(s, 3);
}

void buzzerUsbConnected() {
    static const BeepStep s[] = {
        {880, 80}, {0, 30}, {1100, 80}, {0, 30}, {1320, 120}
    };
    schedule(s, 5);
}
