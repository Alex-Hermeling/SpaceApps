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
  for (int k = 0; k < 16; k++) sum += analogRead(LDR_PINS[i]);
  return sum / 16;
}

// A reading below LDR_MIN_VALID is probably a loose wire / short / broken sensor.
inline bool ldrValid(int v) { return v >= LDR_MIN_VALID; }

// Average of all VALID sensors (logging only). 0 if none are valid.
inline int readLight() {
  long sum = 0; int n = 0;
  for (int i = 0; i < NUM_LDR; i++) {
    int v = readLightSensor(i);
    if (ldrValid(v)) { sum += v; n++; }
  }
  return n ? sum / n : 0;
}

// Light for the in-box check: average of the VALID sensors except the front one (FRONT_LDR).
// If none of those are valid, falls back to the front sensor; 0 if nothing is valid.
inline int readBoxLight() {
  long sum = 0; int n = 0;
  for (int i = 0; i < NUM_LDR; i++) {
    if (i == FRONT_LDR) continue;
    int v = readLightSensor(i);
    if (ldrValid(v)) { sum += v; n++; }
  }
  if (n) return sum / n;
  int f = readLightSensor(FRONT_LDR);
  return ldrValid(f) ? f : 0;
}

// TCRT5000 output: IR_CLOSE_LEVEL = something close.
inline int readIr() { return digitalRead(IR_PIN); }

// True when the (closed) door is in front of the sensor.
inline bool doorClosed() { return readIr() == IR_CLOSE_LEVEL; }
