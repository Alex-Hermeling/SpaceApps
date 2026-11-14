// panel_servo.h - slow, non-blocking servo moves.
// (Not named servo.h: on macOS that would clash with the library's <Servo.h>.)
//
// Call servoUpdate() every loop(); it steps 1 degree every SERVO_STEP_MS
// towards the target set with servoMoveTo().
#pragma once
#include <Servo.h>
#include "config.h"

Servo panelServo;
int servoAngle  = SERVO_PARK_ANGLE;  // assumed position at power-up
int servoTarget = SERVO_PARK_ANGLE;
unsigned long servoLastStep = 0;

inline void servoInit() {
  // The servo is assumed to already be at SERVO_PARK_ANGLE (the program always
  // ends there), so writing that angle doesn't make it jump.
  panelServo.attach(SERVO_PIN);
  panelServo.write(servoAngle);
}

inline void servoMoveTo(int angle) {
  servoTarget = constrain(angle, 0, 180);
}

inline bool servoAtTarget() { return servoAngle == servoTarget; }

inline void servoUpdate() {
  if (servoAtTarget()) return;
  if (millis() - servoLastStep < SERVO_STEP_MS) return;
  servoLastStep = millis();
  servoAngle += (servoTarget > servoAngle) ? 1 : -1;
  panelServo.write(servoAngle);
}
