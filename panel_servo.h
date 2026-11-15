// panel_servo.h - slow, non-blocking servo moves. Both servos move together.
// (Not named servo.h: on macOS that would clash with the library's <Servo.h>.)
//
// Call servoUpdate() every loop(); it steps 1 degree every SERVO_STEP_MS
// towards the target set with servoMoveTo().
#pragma once
#include <Servo.h>
#include "config.h"

Servo panelServos[NUM_SERVOS];
int servoAngle  = SERVO_PARK_ANGLE;  // PANEL angle (0 = furled), assumed at power-up
int servoTarget = SERVO_PARK_ANGLE;
unsigned long servoLastStep = 0;

// Write a panel angle to every servo; mirrored servos (SERVO_REVERSED) get 180 - angle.
inline void servoWriteAll(int angle) {
  for (int i = 0; i < NUM_SERVOS; i++)
    panelServos[i].write(SERVO_REVERSED[i] ? 180 - angle : angle);
}

inline void servoInit() {
  // The panel is assumed to already be at SERVO_PARK_ANGLE (furled - the program
  // always ends there), so writing that angle doesn't make the servos jump.
  for (int i = 0; i < NUM_SERVOS; i++) panelServos[i].attach(SERVO_PINS[i]);
  servoWriteAll(servoAngle);
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
  servoWriteAll(servoAngle);
}
