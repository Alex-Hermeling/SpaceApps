// SpaceApps
// STM32F103C8T6 "Blue Pill" + 2x TB6612FNG + 3 DC motors, 3 LDR light sensors, IR door sensor, 2 servos.
// Pins: pinout.md / config.h. Motor C is set up but not used by the mission yet.
//
// Mission:
//   BOOT -> STOW_SERVO -> WAIT_IN_BOX -> WAIT_DOOR_OPEN -> DOOR_SETTLE -> DRIVE_OUT
//        -> STABILIZE -> PARK_SERVO -> UNFURL (0 -> 180) -> UNFURLED
//        -> FURL (180 -> 0) -> FURLED -> UNFURL ... (loop)
// In-box check uses L1 (rear) + L2 (side) only (front sensor L3 ignored); the door itself is the IR sensor.
// Sandstorm = ANY one light sensor dark (vs. its own surface light, logged in STABILIZE)
// for STORM_DARK_MS -> furl. That same sensor light again for STORM_CLEAR_MS -> unfurl. Loops.
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

// ---------------- Status LED patterns (onboard LED, PC13) ----------------
// Each state repeats: `pulses` blinks of `onMs` on / `offMs` off, then `gapMs` dark.
// pulses = 0 means solid on. Order must match enum State.
struct LedPattern { int pulses; unsigned int onMs, offMs, gapMs; };
const LedPattern LED_PATTERNS[] = {
  {1, 100, 100,   0},  // BOOT            fast blink (logging outside light)
  {1,  50,  50,   0},  // STOW_SERVO      flicker (servos moving)
  {1, 100, 900,   0},  // WAIT_IN_BOX     1 short blink per second
  {2, 100, 150, 900},  // WAIT_DOOR_OPEN  double blink
  {1, 250, 250,   0},  // DOOR_SETTLE     medium blink (5 s countdown)
  {0,   0,   0,   0},  // DRIVE_OUT       solid on
  {3, 100, 150, 900},  // STABILIZE       triple blink
  {1,  50,  50,   0},  // PARK_SERVO      flicker (servos moving)
  {1,  50,  50,   0},  // UNFURL          flicker (servos moving)
  {1,  50,1950,   0},  // UNFURLED        short heartbeat every 2 s
  {1,  50,  50,   0},  // FURL            flicker (servos moving)
  {1,1000,1000,   0},  // FURLED          slow 1 s on / 1 s off
};
static_assert(sizeof(LED_PATTERNS) / sizeof(LED_PATTERNS[0]) == FURLED + 1,
              "LED_PATTERNS needs one entry per State");

// Serial command for the panel: 'f' = furl, 'u' = unfurl, 0 = none (manual override).
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
int lightSurface[NUM_LDR] = {0};   // per sensor, where the rover stopped (sandstorm reference)
bool surfaceLogged = false;
bool doorOpened    = false;   // latched once the door is seen opening: IR ignored after that
bool dimmingLogged = false;

// Sandstorm: per-sensor dark timers, and which sensor triggered the furl (-1 = manual)
unsigned long darkSince[NUM_LDR] = {0};
int stormSensor = -1;

// BOOT averaging
unsigned long bootSum   = 0;
unsigned long bootCount = 0;
unsigned long lastSample = 0;

// STABILIZE averaging, per sensor
unsigned long surfSum[NUM_LDR] = {0};
unsigned long surfCount = 0;

void enterState(State s) {
  state = s;
  stateStart = millis();
  condSince = 0;
  Serial.print("\n>>> ");
  Serial.println(STATE_NAMES[s]);
}

// Drive the status LED from the current state's pattern (non-blocking).
void ledUpdate() {
  const LedPattern& p = LED_PATTERNS[state];
  bool on;
  if (p.pulses == 0) {
    on = true;  // solid
  } else {
    unsigned long period = p.onMs + p.offMs;
    unsigned long cycle  = p.pulses * period + p.gapMs;
    unsigned long t = (millis() - stateStart) % cycle;
    on = (t < p.pulses * period) && (t % period < p.onMs);
  }
  digitalWrite(LED_PIN, on ? LED_ON_LEVEL : !LED_ON_LEVEL);
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
    int v = readLightSensor(i);
    Serial.print(v);
    if (!ldrValid(v)) Serial.print('!');  // ! = invalid reading (< LDR_MIN_VALID), ignored
  }
  Serial.print(']');
  Serial.print("  ir=");     Serial.print(readIr());
  Serial.print("  door=");
  if (doorOpened) Serial.print("open (IR ignored)");
  else            Serial.print(doorClosed() ? "closed" : "open");
  Serial.print("  servo=");  Serial.print(servoAngle);
  if (surfaceLogged) {
    Serial.print("  surface=[");
    for (int i = 0; i < NUM_LDR; i++) {
      if (i) Serial.print(' ');
      Serial.print(lightSurface[i]);
    }
    Serial.print(']');
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1500);  // give the Serial Monitor time to connect
#if defined(ARDUINO_ARCH_STM32F1)
  // Same as HardwareTest: free PA15/PB3/PB4 from JTAG (keeps SWD for ST-Link).
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
#endif
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, !LED_ON_LEVEL);  // off
  motorsInit();
  sensorsInit();
  servoInit();  // assumes the servo is at SERVO_PARK_ANGLE (where the last run left it)
  Serial.println("SpaceApps starting");
  enterState(BOOT);
}

void runMission() {
  switch (state) {

    case BOOT:
      // Average the outside light (rover boots outside the box). Front sensor ignored.
      if (millis() - lastSample >= 10) {
        lastSample = millis();
        bootSum += readBoxLight();
        bootCount++;
      }
      if (inState() >= LIGHT_SAMPLE_MS) {
        lightOutside = bootSum / bootCount;
        Serial.print("Logged light OUTSIDE = "); Serial.println(lightOutside);
        if (!ldrValid(lightOutside))
          Serial.println("WARNING: no valid light sensor at boot - the in-box check can't pass. Check the wiring.");
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
      // Know we're in the box: door close to the IR sensor AND dark compared to outside
      // (L1 rear + L2 side only - the front sensor L3 is ignored).
      bool dark = readBoxLight() < lightOutside * IN_BOX_FRACTION;
      if (heldFor(doorClosed() && dark, STABLE_MS)) {
        lightInside = readBoxLight();
        Serial.print("Logged light INSIDE = ");  Serial.println(lightInside);
        enterState(WAIT_DOOR_OPEN);
      }
      break;
    }

    case WAIT_DOOR_OPEN:
      // Door opening: IR no longer sees the door within ~2 cm.
      // From here on the IR sensor is ignored: it is close to the ground and could
      // read "close" during movement. Nothing after this state looks at it.
      if (heldFor(!doorClosed(), DOOR_DEBOUNCE_MS)) {
        doorOpened = true;
        Serial.println("Door opened - IR sensor ignored from now on");
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
        for (int i = 0; i < NUM_LDR; i++) surfSum[i] = 0;
        surfCount = 0;
        enterState(STABILIZE);
      }
      break;

    case STABILIZE:
      // Stand still before unfurling, and log each sensor's light as its sandstorm reference.
      brakeAll();
      if (millis() - lastSample >= 10) {
        lastSample = millis();
        for (int i = 0; i < NUM_LDR; i++) surfSum[i] += readLightSensor(i);
        surfCount++;
      }
      if (inState() >= STABLE_WAIT_MS) {
        stopMotors();
        Serial.print("Logged light SURFACE = [");
        for (int i = 0; i < NUM_LDR; i++) {
          lightSurface[i] = surfSum[i] / surfCount;
          if (i) Serial.print(' ');
          Serial.print(lightSurface[i]);
        }
        Serial.println("]  (L1 rear, L2 side, L3 front)");
        for (int i = 0; i < NUM_LDR; i++) {
          if (!ldrValid(lightSurface[i])) {
            Serial.print("WARNING: L"); Serial.print(i + 1);
            Serial.println(" reads invalid - it is ignored for sandstorm detection");
          }
        }
        surfaceLogged = true;
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
        for (int i = 0; i < NUM_LDR; i++) darkSince[i] = 0;
        enterState(UNFURLED);
      }
      break;

    case UNFURLED: {
      // Sandstorm: ANY one sensor dark (vs. its own surface light) for STORM_DARK_MS -> furl.
      bool anyDim = false;
      int darkSensor = -1;
      unsigned long now = millis();
      for (int i = 0; i < NUM_LDR; i++) {
        int v = readLightSensor(i);
        // Skip sensors that read invalid now, or were invalid when the surface light was
        // logged: a loose / shorted sensor must not fake a sandstorm.
        if (!ldrValid(v) || !ldrValid(lightSurface[i])) { darkSince[i] = 0; continue; }
        if (v < lightSurface[i] * STORM_DIM_FRACTION) anyDim = true;
        if (v < lightSurface[i] * STORM_DARK_FRACTION) {
          if (darkSince[i] == 0) darkSince[i] = now ? now : 1;
          if (darkSensor < 0 && now - darkSince[i] >= STORM_DARK_MS) darkSensor = i;
        } else {
          darkSince[i] = 0;
        }
      }
      if (anyDim && !dimmingLogged) Serial.println("Light dimming...");
      dimmingLogged = anyDim;

      if (darkSensor >= 0) {
        stormSensor = darkSensor;
        Serial.print("SANDSTORM (L"); Serial.print(stormSensor + 1); Serial.println(" dark) - furling");
        servoMoveTo(SERVO_FURL_ANGLE);
        enterState(FURL);
      } else if (readPanelCommand() == 'f') {
        stormSensor = -1;
        Serial.println("Manual furl");
        servoMoveTo(SERVO_FURL_ANGLE);
        enterState(FURL);
      }
      break;
    }

    case FURL:
      // Both servos 180 -> 0, slowly. Ends at 0, so the next power-up can assume it.
      if (servoAtTarget()) {
        Serial.print("FURLED (servos at 0, safe to power off) - ");
        if (stormSensor >= 0) {
          Serial.print("waiting for L"); Serial.print(stormSensor + 1); Serial.println(" to see light ('u' = unfurl now)");
        } else {
          Serial.println("send 'u' to unfurl");
        }
        enterState(FURLED);
      }
      break;

    case FURLED: {
      // Sandstorm over: the sensor that went dark sees light again for STORM_CLEAR_MS -> unfurl.
      // After a manual furl (stormSensor = -1) only 'u' unfurls.
      bool overNow = false;
      if (stormSensor >= 0) {
        // An invalid reading (< LDR_MIN_VALID) is never "clear". If that sensor breaks
        // while furled, the rover stays furled until 'u'.
        int v = readLightSensor(stormSensor);
        bool clear = ldrValid(v) && v > lightSurface[stormSensor] * STORM_CLEAR_FRACTION;
        overNow = heldFor(clear, STORM_CLEAR_MS);
      }
      if (overNow || readPanelCommand() == 'u') {
        if (overNow) {
          Serial.print("Sandstorm over (L"); Serial.print(stormSensor + 1); Serial.println(" light) - unfurling");
        } else {
          Serial.println("Manual unfurl");
        }
        servoMoveTo(SERVO_UNFURL_ANGLE);
        enterState(UNFURL);
      }
      break;
    }
  }

  servoUpdate();  // step the servos towards their target
  ledUpdate();    // blink the status LED for the current state

  logSensors();
}

void loop() {
  runMission();
}
