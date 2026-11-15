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
//   Trigger distance: 2 cm. It is set with the module's potentiometer, not in code:
//   hold the door 2 cm away and turn the pot until the module's LED just switches on.
// Short range: reliable up to ~1-2 cm, best on a white/light surface.
// Once the door has been seen opening, the mission ignores the IR sensor for good
// (it sits close to the ground and could see the ground as "close" while driving).
const int IR_PIN = PA5;
// Level OUT reads when the door is in front of the sensor.
// This module reads HIGH when close (serial log: ir=1 with the door shut in the dark box).
// Most TCRT5000 boards are LOW - flip this if a different module is used.
const int IR_CLOSE_LEVEL = HIGH;

// ---------------- Servos (unfurl) ----------------
// Both on timer 3. The Servo library takes over the whole timer, so no motor PWM may use it.
const int NUM_SERVOS = 2;
const int SERVO_PINS[NUM_SERVOS] = {PB0, PA7};  // servo 1, servo 2
// Both servos are mounted mirrored: they get 180 - angle.
// So for both: 180 = furled (closed), 0 = unfurled (open).
const bool SERVO_REVERSED[NUM_SERVOS] = {true, true};

// All angles below are PANEL angles (0 = furled/closed, 180 = unfurled); the
// mirroring above turns them into the real angle for each servo.
// Servo convention: at power-up we ASSUME the panel is furled (SERVO_PARK_ANGLE),
// because its real position can't be read. Real angles at boot: both servos at 180.
// The mission ends UNFURLED (never refurls), so furl it by hand before the next run
// ('f' in the main sketch's Serial Monitor, or 'x' in HardwareTest).
const int SERVO_PARK_ANGLE = 0;   // start position (furled)
const int SERVO_STOW_ANGLE = 0;   // position while in the box (furled)
const int SERVO_UNFURL_ANGLE = 180;  // unfurl: both servos 180 -> 0
const int SERVO_FURL_ANGLE   = 0;    // furl:   both servos 0 -> 180 (= park)
const unsigned long SERVO_STEP_MS = 15;  // ms per degree: higher = slower (90 deg = 1.35 s)

// ---------------- Status LED ----------------
// Blue Pill onboard LED: PC13, lights when the pin is LOW. Blinks a different pattern per state.
const int LED_PIN = PC13;
const int LED_ON_LEVEL = LOW;

// ---------------- Thresholds ----------------
const float IN_BOX_FRACTION = 0.5;  // in box when L1+L2 light < outside light * this (front L3 ignored)

// A light sensor reading below this is treated as INVALID (loose wire, short to GND,
// broken sensor) and left out of every light decision. Real darkness reads ~400 with 8k.
const int LDR_MIN_VALID = 50;

// ---------------- Timings (ms) ----------------
const unsigned long LIGHT_SAMPLE_MS  = 2000;   // BOOT: average outside light over this long
const unsigned long STABLE_MS        = 2000;   // condition must hold this long to count
const unsigned long DOOR_DEBOUNCE_MS = 1000;   // door must read "open" this long
const unsigned long DOOR_SETTLE_MS   = 5000;   // wait after door opens before driving
const unsigned long DRIVE_TIME_MS    = 2500;   // DRIVE_OUT: drive this long, then stop - raise to go further
const unsigned long STABLE_WAIT_MS   = 3000;   // stand still before unfurling
const unsigned long LOG_EVERY_MS     = 500;

// ---------------- Speeds (0..255) ----------------
const int DRIVE_SPEED = 255;  // DRIVE_OUT speed: full
