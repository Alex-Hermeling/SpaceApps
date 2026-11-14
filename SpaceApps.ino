// SpaceApps
// STM32F103C8T6 "Blue Pill" + TB6612FNG + 2 DC motors, LDR light sensor, IR door sensor, servo.
//
// Mission (everything before unfurling):
//   BOOT -> STOW_SERVO -> WAIT_IN_BOX -> WAIT_DOOR_OPEN -> DOOR_SETTLE -> DRIVE_OUT
//        -> STABILIZE -> PARK_SERVO -> READY_TO_UNFURL
//
// Servo rule: the program always ends with the servo at 90 (SERVO_PARK_ANGLE),
// so at power-up it is assumed to be at 90. It goes to 0 before entering the box.
//
// Pins, thresholds and timings: config.h
// Motor control: motors.h    Sensors: sensors.h    Servo: panel_servo.h

#include <Servo.h>
#include "config.h"
#include "motors.h"
#include "sensors.h"
#include "panel_servo.h"

enum State {
  BOOT,
  STOW_SERVO,
  WAIT_IN_BOX,
  WAIT_DOOR_OPEN,
  DOOR_SETTLE,
  DRIVE_OUT,
  STABILIZE,
  PARK_SERVO,
  READY_TO_UNFURL
};

const char* STATE_NAMES[] = {
  "BOOT", "STOW_SERVO", "WAIT_IN_BOX", "WAIT_DOOR_OPEN", "DOOR_SETTLE",
  "DRIVE_OUT", "STABILIZE", "PARK_SERVO", "READY_TO_UNFURL"
};

State state = BOOT;
unsigned long stateStart = 0;   // millis() when the current state began
unsigned long condSince  = 0;   // millis() when the current condition became true (0 = not true)
unsigned long lastLog    = 0;

// Light readings logged during the mission
int lightOutside   = 0;
int lightInside    = 0;
int lightThreshold = 0;   // halfway between inside and outside

// BOOT averaging
unsigned long bootSum   = 0;
unsigned long bootCount = 0;
unsigned long lastSample = 0;

// DRIVE_OUT: when light first said "outside" (0 = not yet)
unsigned long outsideAt = 0;

void enterState(State s) {
  state = s;
  stateStart = millis();
  condSince = 0;
  Serial.print("\n>>> ");
  Serial.println(STATE_NAMES[s]);
}

// True once `cond` has been true continuously for `ms`.
bool heldFor(bool cond, unsigned long ms) {
  if (!cond) { condSince = 0; return false; }
  unsigned long now = millis();
  if (condSince == 0) condSince = now ? now : 1;
  return now - condSince >= ms;
}

unsigned long inState() { return millis() - stateStart; }

void logSensors() {
  if (millis() - lastLog < LOG_EVERY_MS) return;
  lastLog = millis();
  Serial.print(STATE_NAMES[state]);
  Serial.print("  light=");  Serial.print(readLight());
  Serial.print("  ir=");     Serial.print(readIr());
  Serial.print("  door=");   Serial.print(doorClosed() ? "closed" : "open");
  Serial.print("  servo=");  Serial.println(servoAngle);
}

void setup() {
  Serial.begin(115200);
  delay(1500);  // give the Serial Monitor time to connect
  motorsInit();
  sensorsInit();
  servoInit();  // assumes the servo is at 90 (where the last run left it)
  Serial.println("SpaceApps starting");
  enterState(BOOT);
}

void runMission() {
  switch (state) {

    case BOOT:
      // Average the outside light (rover boots outside the box).
      if (millis() - lastSample >= 10) {
        lastSample = millis();
        bootSum += readLight();
        bootCount++;
      }
      if (inState() >= LIGHT_SAMPLE_MS) {
        lightOutside = bootSum / bootCount;
        Serial.print("Logged light OUTSIDE = "); Serial.println(lightOutside);
        servoMoveTo(SERVO_STOW_ANGLE);
        enterState(STOW_SERVO);
      }
      break;

    case STOW_SERVO:
      // Fold the servo to 0 before going in the box.
      if (servoAtTarget()) {
        Serial.println("Servo stowed at 0 - put the rover in the box");
        enterState(WAIT_IN_BOX);
      }
      break;

    case WAIT_IN_BOX: {
      // Know we're in the box: door close to the IR sensor AND dark compared to outside.
      bool dark = readLight() < lightOutside * IN_BOX_FRACTION;
      if (heldFor(doorClosed() && dark, STABLE_MS)) {
        lightInside = readLight();
        lightThreshold = (lightInside + lightOutside) / 2;
        Serial.print("Logged light INSIDE = ");  Serial.println(lightInside);
        Serial.print("Outside threshold = ");    Serial.println(lightThreshold);
        enterState(WAIT_DOOR_OPEN);
      }
      break;
    }

    case WAIT_DOOR_OPEN:
      // Door opening: IR no longer sees the door within ~5 cm.
      if (heldFor(!doorClosed(), DOOR_DEBOUNCE_MS)) {
        Serial.println("Door opened");
        enterState(DOOR_SETTLE);
      }
      break;

    case DOOR_SETTLE:
      // Internal timer: let the door finish opening.
      if (inState() >= DOOR_SETTLE_MS) {
        outsideAt = 0;
        driveForward(DRIVE_SLOW_SPEED);
        enterState(DRIVE_OUT);
      }
      break;

    case DRIVE_OUT:
      // Drive slowly to protect the solar panel.
      driveForward(DRIVE_SLOW_SPEED);

      if (outsideAt == 0 && heldFor(readLight() > lightThreshold, STABLE_MS)) {
        outsideAt = millis();
        Serial.println("Light says OUTSIDE - clearing the box");
      }
      if (outsideAt != 0 && millis() - outsideAt >= CLEAR_EXTRA_MS) {
        brakeAll();
        Serial.println("Out of the box");
        enterState(STABILIZE);
      } else if (inState() >= DRIVE_MAX_MS) {
        brakeAll();
        Serial.println("Drive TIMEOUT - stopping for safety");
        enterState(STABILIZE);
      }
      break;

    case STABILIZE:
      // Stand still before unfurling.
      brakeAll();
      if (inState() >= STABLE_WAIT_MS) {
        stopMotors();
        servoMoveTo(SERVO_PARK_ANGLE);
        enterState(PARK_SERVO);
      }
      break;

    case PARK_SERVO:
      // Always leave the servo at 90 at the end, so the next power-up can assume it.
      if (servoAtTarget()) {
        Serial.println("Servo parked at 90");
        Serial.println("READY TO UNFURL");
        enterState(READY_TO_UNFURL);
      }
      break;

    case READY_TO_UNFURL:
      // TODO: unfurl panels, then sandstorm -> furl loop.
      // Any future code must also finish with the servo back at SERVO_PARK_ANGLE.
      break;
  }

  servoUpdate();  // step the servo towards its target

  if (state != READY_TO_UNFURL) logSensors();
}

void loop() {
  runMission();
}
