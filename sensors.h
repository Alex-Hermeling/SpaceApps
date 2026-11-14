// sensors.h - LDR light sensor and IR door sensor.
#pragma once
#include "config.h"

inline void sensorsInit() {
#if defined(ARDUINO_ARCH_STM32)
  analogReadResolution(12);  // official core defaults to 10-bit; use 0..4095 like Roger's core
#endif
  pinMode(LDR_PIN, INPUT_ANALOG);
#if IR_MODE == IR_ANALOG
  pinMode(IR_PIN, INPUT_ANALOG);
#else
  pinMode(IR_PIN, INPUT_PULLUP);
#endif
}

// Light level, 0 (dark) .. 4095 (bright), averaged over 16 samples.
inline int readLight() {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(LDR_PIN);
  return sum / 16;
}

// Raw IR value: analog 0..4095, or digital 0/1.
inline int readIrRaw() {
#if IR_MODE == IR_ANALOG
  return analogRead(IR_PIN);
#else
  return digitalRead(IR_PIN);
#endif
}

// True when something (the closed door) is within ~5 cm.
inline bool doorClosed() {
#if IR_MODE == IR_ANALOG
  return readIrRaw() > IR_CLOSE_RAW;
#else
  return readIrRaw() == IR_CLOSE_LEVEL;
#endif
}
