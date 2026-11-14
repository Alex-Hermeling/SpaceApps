// config.h - all pins, thresholds and timings in one place.
#pragma once
#include <Arduino.h>

// ---------------- Mode ----------------
// 0 = mission (state machine)
// 1 = motor test loop: forward / brake / reverse / coast, repeating
// 2 = driver diagnose: full speed forward 5 s with PWM bypassed
#define TEST_MODE 0

// ---------------- Motor pins (TB6612FNG) ----------------
// Avoided pins: PA9/PA10 (serial upload), PA11/PA12 (USB),
// PA13/PA14 (SWD / ST-Link), PC13 (onboard LED), PB2 (BOOT1).
const int PWMA = PA0;   // speed (PWM, TIM2_CH1)
const int AIN1 = PB12;  // direction
const int AIN2 = PB13;  // direction
const int PWMB = PA1;   // speed (PWM, TIM2_CH2)
const int BIN1 = PB14;  // direction
const int BIN2 = PB15;  // direction
const int STBY = PA8;   // HIGH = driver enabled

// ---------------- Sensor pins ----------------
// LDR divider: 3.3V -> LDR -> PA4 -> 10k -> GND  (brighter = higher value)
const int LDR_PIN = PA4;
// IR sensor output, sensor powered from 3.3V
const int IR_PIN  = PA5;

// IR sensor type (part unknown for now):
//   IR_DIGITAL - obstacle module (e.g. FC-51), OUT goes LOW when something is close.
//                Set the ~5 cm trigger distance with the module's potentiometer.
//   IR_ANALOG  - analog distance sensor (e.g. Sharp GP2Y0A). Higher = closer.
#define IR_DIGITAL 1
#define IR_ANALOG  2
#define IR_MODE IR_DIGITAL

const int IR_CLOSE_LEVEL = LOW;   // IR_DIGITAL: level meaning "object close"
const int IR_CLOSE_RAW   = 2000;  // IR_ANALOG: raw value (0..4095) at ~5 cm - calibrate from serial log

// ---------------- Thresholds ----------------
const float IN_BOX_FRACTION = 0.5;  // in box when light < outside light * this

// ---------------- Timings (ms) ----------------
const unsigned long LIGHT_SAMPLE_MS  = 2000;   // BOOT: average outside light over this long
const unsigned long STABLE_MS        = 2000;   // condition must hold this long to count
const unsigned long DOOR_DEBOUNCE_MS = 1000;   // door must read "open" this long
const unsigned long DOOR_SETTLE_MS   = 5000;   // wait after door opens before driving
const unsigned long CLEAR_EXTRA_MS   = 1500;   // keep driving after "outside" to fully leave the box
const unsigned long DRIVE_MAX_MS     = 10000;  // safety: never drive longer than this
const unsigned long STABLE_WAIT_MS   = 3000;   // stand still before unfurling
const unsigned long LOG_EVERY_MS     = 500;

// ---------------- Speeds (0..255) ----------------
const int DRIVE_SLOW_SPEED = 100;  // slow, to protect the solar panel
const int TEST_SPEED       = 100;  // TEST_MODE 1
