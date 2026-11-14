// config.h - all pins, thresholds and timings in one place.
#pragma once
#include <Arduino.h>

// Hardware testing lives in the separate HardwareTest/ sketch.
// If you change pins here, change them in HardwareTest/HardwareTest.ino too.

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
// LDR divider: 3.3V -> LDR -> PA4 -> 8k -> GND  (brighter = higher value)
const int LDR_PIN = PA4;
// TCRT5000 IR reflective module (3-pin: VCC, GND, OUT), powered from 3.3V.
//   OUT: level set by IR_CLOSE_LEVEL when something reflective is close.
//   Set the trigger distance with the module's potentiometer.
// Short range: reliable up to ~1-2 cm, best on a white/light surface.
const int IR_PIN = PA5;
// Level OUT reads when the door is in front of the sensor.
// This module reads HIGH when close (serial log: ir=1 with the door shut in the dark box).
// Most TCRT5000 boards are LOW - flip this if a different module is used.
const int IR_CLOSE_LEVEL = HIGH;

// ---------------- Servo (unfurl) ----------------
// Must NOT be on timer 2 (PA0/PA1 motor PWM): the Servo library takes over the whole timer.
const int SERVO_PIN = PA6;  // TIM3_CH1

// ---------------- Thresholds ----------------
const float IN_BOX_FRACTION = 0.5;  // in box when light < outside light * this

// ---------------- Timings (ms) ----------------
const unsigned long LIGHT_SAMPLE_MS  = 2000;   // BOOT: average outside light over this long
const unsigned long STABLE_MS        = 2000;   // condition must hold this long to count
const unsigned long DOOR_DEBOUNCE_MS = 1000;   // door must read "open" this long
const unsigned long DOOR_SETTLE_MS   = 5000;   // wait after door opens before driving
const unsigned long CLEAR_EXTRA_MS   = 5000;   // keep driving after "outside" - raise to go further out
const unsigned long DRIVE_MAX_MS     = 20000;  // safety: never drive longer than this (keep > ~2 s + exit time + CLEAR_EXTRA_MS)
const unsigned long STABLE_WAIT_MS   = 3000;   // stand still before unfurling
const unsigned long LOG_EVERY_MS     = 500;

// ---------------- Speeds (0..255) ----------------
const int DRIVE_SLOW_SPEED = 75;  // slow, to protect the solar panel
