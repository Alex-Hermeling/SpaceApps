// config.h - all pins, thresholds and timings in one place.
#pragma once
#include <Arduino.h>

// Pins follow pinout.md (tested with HardwareTest).
// If you change pins here, change them in HardwareTest/HardwareTest.ino and pinout.md too.

// ---------------- Motor driver #1 (TB6612FNG, 2 channels) ----------------
// Avoided pins: PA9/PA10 (serial upload), PA11/PA12 (USB),
// PA13/PA14 (SWD / ST-Link), PC13 (onboard LED), PB2 (BOOT1).
const int PWMA = PA0;   // motor A speed (PWM, timer 2)
const int AIN1 = PB15;  // motor A direction
const int AIN2 = PB14;  // motor A direction
const int PWMB = PA1;   // motor B speed (PWM, timer 2)
const int BIN1 = PB6;   // motor B direction
const int BIN2 = PB7;   // motor B direction
const int STBY = PB5;   // HIGH = enabled, shared by both drivers

// ---------------- Motor driver #2 (TB6612FNG, channel A only) = motor C ----------------
const int PWMC = PA8;   // motor C speed (PWM, timer 1)
const int CIN1 = PB12;  // motor C direction
const int CIN2 = PB13;  // motor C direction

// ---------------- Sensor pins ----------------
// 3 light sensors: 3.3V -> LDR -> pin -> 8k -> GND  (brighter = higher value)
const int NUM_LDR = 3;
const int LDR_PINS[NUM_LDR] = {PA4, PA2, PB1};  // L1 rear, L2 side, L3 front
const int FRONT_LDR = 2;  // L3 (PB1) = front sensor: ignored for the in-box check
// TCRT5000 IR reflective module (3-pin: VCC, GND, OUT), powered from 3.3V.
//   OUT: level set by IR_CLOSE_LEVEL when something reflective is close.
//   Set the trigger distance with the module's potentiometer.
// Short range: reliable up to ~1-2 cm, best on a white/light surface.
const int IR_PIN = PA5;
// Level OUT reads when the door is in front of the sensor.
// This module reads HIGH when close (serial log: ir=1 with the door shut in the dark box).
// Most TCRT5000 boards are LOW - flip this if a different module is used.
const int IR_CLOSE_LEVEL = HIGH;

// ---------------- Servos (unfurl) ----------------
// Both on timer 3. The Servo library takes over the whole timer, so no motor PWM may use it.
const int NUM_SERVOS = 2;
const int SERVO_PINS[NUM_SERVOS] = {PB0, PA7};  // servo 1, servo 2

// Servo convention: the program always ENDS with the servos at SERVO_PARK_ANGLE,
// so at power-up we ASSUME it is there (its real position can't be read).
const int SERVO_PARK_ANGLE = 0;   // start + end position (boot at 0)
const int SERVO_STOW_ANGLE = 0;   // position while in the box
const int SERVO_UNFURL_ANGLE = 180;  // unfurl: both servos 0 -> 180
const int SERVO_FURL_ANGLE   = 0;    // furl:   both servos 180 -> 0 (= park, so the rule holds)
const unsigned long SERVO_STEP_MS = 15;  // ms per degree: higher = slower (90 deg = 1.35 s)

// ---------------- Thresholds ----------------
const float IN_BOX_FRACTION = 0.5;  // in box when L1+L2 light < outside light * this (front L3 ignored)

// Sandstorm: checked PER SENSOR, each against its own surface light
// (logged while standing still after driving out). ANY one sensor dark -> furl.
// A loose LDR floats at ~1000-2000, so keep "dark" below ~1000 (lit ~3000 * 0.3 = ~900)
// or a loose sensor could fake a sandstorm.
const float STORM_DIM_FRACTION   = 0.75;  // below this = "dimming" (log message only)
const float STORM_DARK_FRACTION  = 0.3;   // below this = dark
const float STORM_CLEAR_FRACTION = 0.6;   // the sensor that went dark is above this = sandstorm over

// ---------------- Timings (ms) ----------------
const unsigned long LIGHT_SAMPLE_MS  = 2000;   // BOOT: average outside light over this long
const unsigned long STABLE_MS        = 2000;   // condition must hold this long to count
const unsigned long DOOR_DEBOUNCE_MS = 1000;   // door must read "open" this long
const unsigned long DOOR_SETTLE_MS   = 5000;   // wait after door opens before driving
const unsigned long DRIVE_TIME_MS    = 2500;   // DRIVE_OUT: drive this long, then stop - raise to go further
const unsigned long STABLE_WAIT_MS   = 3000;   // stand still before unfurling
const unsigned long STORM_DARK_MS    = 2000;   // any one sensor dark this long = sandstorm -> furl
const unsigned long STORM_CLEAR_MS   = 10000;  // that sensor light again this long = storm over -> unfurl
const unsigned long LOG_EVERY_MS     = 500;

// ---------------- Speeds (0..255) ----------------
const int DRIVE_SPEED = 255;  // DRIVE_OUT speed: full
