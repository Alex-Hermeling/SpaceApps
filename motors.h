// motors.h - TB6612FNG dual DC motor control.
// Works on both the official STMicroelectronics core (ARDUINO_ARCH_STM32)
// and the Roger Clark core (ARDUINO_ARCH_STM32F1).
#pragma once
#include "config.h"

// Write a 0..255 duty cycle to a PWM pin.
inline void writeSpeed(int pin, int duty) {
#if defined(ARDUINO_ARCH_STM32F1)
  pwmWrite(pin, duty * 257);  // Roger Clark core: 16-bit duty
#else
  analogWrite(pin, duty);
#endif
}

// speed: -255 (full reverse) .. 0 (coast) .. 255 (full forward)
inline void setMotor(int pwmPin, int in1, int in2, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);  digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);  digitalWrite(in2, LOW);  // coast
  }
  writeSpeed(pwmPin, abs(speed));
}

inline void motorA(int speed) { setMotor(PWMA, AIN1, AIN2, speed); }
inline void motorB(int speed) { setMotor(PWMB, BIN1, BIN2, speed); }

inline void driveForward(int speed) { motorA(speed); motorB(speed); }
inline void stopMotors()            { motorA(0);     motorB(0); }   // coast

inline void brakeAll() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); writeSpeed(PWMA, 0);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); writeSpeed(PWMB, 0);
}

inline void motorsInit() {
  int pins[] = {AIN1, AIN2, BIN1, BIN2, STBY};
  for (int p : pins) pinMode(p, OUTPUT);

#if defined(ARDUINO_ARCH_STM32F1)
  pinMode(PWMA, PWM);  // Roger Clark core: OUTPUT would disconnect the timer
  pinMode(PWMB, PWM);
#else
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
#endif

#if defined(ARDUINO_ARCH_STM32)
  analogWriteFrequency(20000);  // 20 kHz: no audible whine
#endif

  stopMotors();
  digitalWrite(STBY, HIGH);  // wake the driver
}

// TEST_MODE 1: forward / brake / reverse / coast (blocking, test only)
inline void motorTestLoop() {
  Serial.println("Forward"); driveForward(TEST_SPEED);  delay(2000);
  Serial.println("Brake");   brakeAll();                delay(1000);
  Serial.println("Reverse"); driveForward(-TEST_SPEED); delay(2000);
  Serial.println("Coast");   stopMotors();              delay(2000);
}

// TEST_MODE 2: full speed forward 5 s with PWM bypassed (blocking, test only).
// Spins -> wiring/driver OK, PWM is the problem.
// Doesn't spin -> check power, ground, STBY, VCC, motor wires.
inline void motorDiagnose() {
  Serial.println("DIAGNOSE: full speed forward 5 s, PWM bypassed");
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  digitalWrite(STBY, HIGH);
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  digitalWrite(PWMA, HIGH); digitalWrite(PWMB, HIGH);
  delay(5000);
  digitalWrite(PWMA, LOW);  digitalWrite(PWMB, LOW);
  digitalWrite(AIN1, LOW);  digitalWrite(BIN1, LOW);
  Serial.println("DIAGNOSE done - press RESET to repeat");
}
