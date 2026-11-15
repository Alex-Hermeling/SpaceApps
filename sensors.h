// sensors.h - 3 LDR light sensors and TCRT5000 IR door sensor.
#pragma once
#include "config.h"

inline void sensorsInit() {
#if defined(ARDUINO_ARCH_STM32)
  analogReadResolution(12);  // official core defaults to 10-bit; use 0..4095 like Roger's core
#endif
  for (int i = 0; i < NUM_LDR; i++) pinMode(LDR_PINS[i], INPUT_ANALOG);
  pinMode(IR_PIN, INPUT_PULLUP);
}

// One light sensor (0 = L1 .. 2 = L3), 0 (dark) .. 4095 (bright), averaged over 16 samples.
inline int readLightSensor(int i) {
  long sum = 0;
  analogRead(LDR_PINS[i]);  // discard: first read after switching pins can carry over the last sensor
  for (int k = 0; k < 16; k++) sum += analogRead(LDR_PINS[i]);
  return sum / 16;
}

// Light level used by the mission: average of all 3 sensors.
inline int readLight() {
  long sum = 0;
  for (int i = 0; i < NUM_LDR; i++) sum += readLightSensor(i);
  return sum / NUM_LDR;
}

// TCRT5000 output: IR_CLOSE_LEVEL = something close.
inline int readIr() { return digitalRead(IR_PIN); }

// True when the (closed) door is in front of the sensor.
inline bool doorClosed() { return readIr() == IR_CLOSE_LEVEL; }
