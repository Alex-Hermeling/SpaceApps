// SpaceApps
// STM32F103C8T6 "Blue Pill" + 2x TB6612FNG + 3 DC motors, 3 LDR light sensors, IR door sensor, 2 servos.
// Pins: pinout.md / config.h. Motor C is set up but not used by the mission yet.
//
// Mission:
//   BOOT -> STOW_SERVO -> WAIT_IN_BOX -> WAIT_DOOR_OPEN -> DOOR_SETTLE -> DRIVE_OUT
//        -> STABILIZE -> PARK_SERVO -> UNFURL (0 -> 180) -> UNFURLED
//        -> FURL (180 -> 0) -> FURLED -> UNFURL ... (loop)
// Sandstorm = light dark (vs. lightSurface, logged in STABILIZE) for STORM_DARK_MS -> furl.
// Light back for STORM_CLEAR_MS -> unfurl again. Loops forever.
// Manual override in the Serial Monitor: 'f' = furl, 'u' = unfurl.
//
// Servo rule: the program always ends with both servos at SERVO_PARK_ANGLE (0),
// so at power-up it is assumed to be there. STOW_SERVO / PARK_SERVO make sure it is
// at SERVO_STOW_ANGLE before the box and back at SERVO_PARK_ANGLE at the end.
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
  UNFURL,
  UNFURLED,
  FURL,
  FURLED
};

const char* STATE_NAMES[] = {
  "BOOT", "STOW_SERVO", "WAIT_IN_BOX", "WAIT_DOOR_OPEN", "DOOR_SETTLE",
  "DRIVE_OUT", "STABILIZE", "PARK_SERVO", "UNFURL", "UNFURLED", "FURL", "FURLED"
};

// Serial command for the panel: 'f' = furl, 'u' = unfurl, 0 = none.
// Stand-in for sandstorm detection, which isn't built yet.
char readPanelCommand() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'f' || c == 'u') return c;
  }
  return 0;
}

State state = BOOT;
unsigned long stateStart = 0;   // millis() when the current state began
unsigned long condSince  = 0;   // millis() when the current condition became true (0 = not true)
unsigned long lastLog    = 0;

// Light readings logged during the mission
int lightOutside   = 0;
int lightInside    = 0;
int lightThreshold = 0;   // halfway between inside and outside
int lightSurface   = 0;   // light where the rover stopped (sandstorm reference)
bool dimmingLogged = false;

// BOOT averaging
unsigned long bootSum   = 0;
unsigned long bootCount = 0;
unsigned long lastSample = 0;

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
  Serial.print(" [");
  for (int i = 0; i < NUM_LDR; i++) {
    if (i) Serial.print(' ');
    Serial.print(readLightSensor(i));
  }
  Serial.print(']');
  Serial.print("  ir=");     Serial.print(readIr());
  Serial.print("  door=");   Serial.print(doorClosed() ? "closed" : "open");
  Serial.print("  servo=");  Serial.print(servoAngle);
  if (lightSurface) { Serial.print("  surface="); Serial.print(lightSurface); }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1500);  // give the Serial Monitor time to connect
#if defined(ARDUINO_ARCH_STM32F1)
  // Same as HardwareTest: free PA15/PB3/PB4 from JTAG (keeps SWD for ST-Link).
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
#endif
  motorsInit();
  sensorsInit();
  servoInit();  // assumes the servo is at SERVO_PARK_ANGLE (where the last run left it)
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
        Serial.print("Servo stowed at "); Serial.print(SERVO_STOW_ANGLE);
        Serial.println(" - put the rover in the box");
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
        driveForward(DRIVE_SPEED);
        enterState(DRIVE_OUT);
      }
      break;

    case DRIVE_OUT:
      // Full speed for a fixed time, then stop.
      driveForward(DRIVE_SPEED);
      if (inState() >= DRIVE_TIME_MS) {
        brakeAll();
        Serial.println("Out of the box");
        bootSum = 0; bootCount = 0;  // STABILIZE averages the surface light
        enterState(STABILIZE);
      }
      break;

    case STABILIZE:
      // Stand still before unfurling, and log the light here as the sandstorm reference.
      brakeAll();
      if (millis() - lastSample >= 10) {
        lastSample = millis();
        bootSum += readLight();   // reused for averaging (BOOT is long over)
        bootCount++;
      }
      if (inState() >= STABLE_WAIT_MS) {
        stopMotors();
        lightSurface = bootSum / bootCount;
        Serial.print("Logged light SURFACE = "); Serial.println(lightSurface);
        servoMoveTo(SERVO_PARK_ANGLE);
        enterState(PARK_SERVO);
      }
      break;

    case PARK_SERVO:
      // Always leave the servo at SERVO_PARK_ANGLE at the end, so the next power-up can assume it.
      if (servoAtTarget()) {
        Serial.print("Servo parked at "); Serial.println(SERVO_PARK_ANGLE);
        servoMoveTo(SERVO_UNFURL_ANGLE);
        enterState(UNFURL);
      }
      break;

    case UNFURL:
      // Both servos 0 -> 180, slowly.
      if (servoAtTarget()) {
        Serial.println("UNFURLED - watching for sandstorm ('f' = furl now)");
        dimmingLogged = false;
        enterState(UNFURLED);
      }
      break;

    case UNFURLED: {
      // Sandstorm: light dims (~10 s), then stays dark for STORM_DARK_MS -> furl.
      // Short blinks can't trigger it: dark must be continuous.
      int light = readLight();
      bool dim = light < lightSurface * STORM_DIM_FRACTION;
      if (dim && !dimmingLogged) Serial.println("Light dimming...");
      dimmingLogged = dim;
      bool dark = light < lightSurface * STORM_DARK_FRACTION;
      bool stormNow = heldFor(dark, STORM_DARK_MS);
      if (stormNow || readPanelCommand() == 'f') {
        Serial.println(stormNow ? "SANDSTORM - furling" : "Manual furl");
        servoMoveTo(SERVO_FURL_ANGLE);
        enterState(FURL);
      }
      break;
    }

    case FURL:
      // Both servos 180 -> 0, slowly. Ends at 0, so the next power-up can assume it.
      if (servoAtTarget()) {
        Serial.println("FURLED (servos at 0, safe to power off) - waiting for light ('u' = unfurl now)");
        enterState(FURLED);
      }
      break;

    case FURLED: {
      // Sandstorm over: light back for STORM_CLEAR_MS -> unfurl again.
      bool clear = readLight() > lightSurface * STORM_CLEAR_FRACTION;
      bool overNow = heldFor(clear, STORM_CLEAR_MS);
      if (overNow || readPanelCommand() == 'u') {
        Serial.println(overNow ? "Sandstorm over - unfurling" : "Manual unfurl");
        servoMoveTo(SERVO_UNFURL_ANGLE);
        enterState(UNFURL);
      }
      break;
    }
  }

  servoUpdate();  // step the servos towards their target

  logSensors();
}

void loop() {
  runMission();
}
