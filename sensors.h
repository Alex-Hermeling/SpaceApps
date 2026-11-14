// sensors.h - LDR light sensor and TCRT5000 IR door sensor.
#pragma once
#include "config.h"

inline void sensorsInit() {
#if defined(ARDUINO_ARCH_STM32)
  analogReadResolution(12);  // official core defaults to 10-bit; use 0..4095 like Roger's core
#endif
  pinMode(LDR_PIN, INPUT_ANALOG);
  pinMode(IR_PIN, INPUT_PULLUP);
}

// Light level, 0 (dark) .. 4095 (bright), averaged over 16 samples.
inline int readLight() {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(LDR_PIN);
  return sum / 16;
}

// TCRT5000 output: LOW = something close.
inline int readIr() { return digitalRead(IR_PIN); }

// True when the (closed) door is in front of the sensor.
inline bool doorClosed() { return readIr() == LOW; }
