#pragma once
#include <Arduino.h>

void buzzerBegin(uint8_t pin);
void buzzerUpdate();           // Call every loop iteration (non-blocking)
bool buzzerIsPlaying();

// Predefined notification sounds
void buzzerSignalAcquired();   // Short rising double-beep  (RC link OK)
void buzzerSignalLost();       // Long low beep             (RC link lost)
void buzzerModeChanged();      // Two quick beeps           (mode button)
void buzzerUsbConnected();     // 3-note rising melody      (power-on / USB ready)
