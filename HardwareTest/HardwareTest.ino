// HardwareTest - standalone test for all SpaceApps hardware:
// 3 motors (2 drivers), 2 servos, IR sensor, 3 light sensors.
// Open this file on its own in the Arduino IDE and upload it.
// Serial Monitor: 115200 baud, line ending "Newline".
//
// Pins follow ../pinout.md. Keep ../config.h in sync
// (Arduino can't include files from a parent folder).

#include <Servo.h>

// ---------------- Pins ----------------
// Motor driver #1 (TB6612FNG, 2 channels)
const int PWMA = PA0;   // motor A speed (timer 2)
const int AIN1 = PB15;
const int AIN2 = PB14;
const int PWMB = PA1;   // motor B speed (timer 2)
const int BIN1 = PB6;
const int BIN2 = PB7;
const int STBY = PB5;   // shared by both drivers

// Motor driver #2 (TB6612FNG, channel A only) = motor C
const int PWMC = PA8;   // motor C speed (timer 1)
const int CIN1 = PB12;
const int CIN2 = PB13;

// Servos - both on timer 3 (the Servo library takes over the whole timer,
// so no motor PWM may use timer 3)
const int SERVO_PINS[2] = {PB0, PA7};  // servo 1, servo 2

// TCRT5000 IR sensor
const int IR_PIN = PA5;
const int IR_CLOSE_LEVEL = HIGH;  // level OUT reads when the door is close (this module: HIGH)

// Light sensors: 3.3V -> LDR -> pin -> 8k -> GND (brighter = higher, 0..4095)
const int NUM_LDR = 3;
const int LDR_PINS[NUM_LDR] = {PA4, PA2, PB1};  // L1 rear, L2 side, L3 front
const char* LDR_NAMES[NUM_LDR] = {"rear", "side", "front"};
const int FRONT_LDR = 2;  // L3: ignored for the in-box check in the main sketch
const int LDR_MIN_VALID = 50;  // below this = invalid (loose wire / short / broken), same as config.h

// ---------------- State ----------------
int speed = 100;  // 0..255, change with + / -

Servo servos[2];
// Both servos: 180 = furled (closed), 0 = unfurled (open).
const int FURLED_ANGLE[2]   = {180, 180};
const int UNFURLED_ANGLE[2] = {0, 0};
int servoAngle[2] = {FURLED_ANGLE[0], FURLED_ANGLE[1]};  // assumed at power-up: every program ends furled
int sel = 0;                 // selected servo: 0 = servo 1, 1 = servo 2
int servoStepMs = 15;        // ms per degree: higher = slower. 15 -> 180 deg in 2.7 s. 0 = full speed

bool irWatch = false;
bool lightWatch = false;
unsigned long lastIrPrint = 0;
unsigned long lastLightPrint = 0;

// ---------------- Motor control ----------------
void writeSpeed(int pin, int duty) {
#if defined(ARDUINO_ARCH_STM32F1)
  pwmWrite(pin, duty * 257);  // Roger Clark core: 16-bit duty
#else
  analogWrite(pin, duty);
#endif
}

void setMotor(int pwmPin, int in1, int in2, int s) {
  s = constrain(s, -255, 255);
  if (s > 0)      { digitalWrite(in1, HIGH); digitalWrite(in2, LOW);  }
  else if (s < 0) { digitalWrite(in1, LOW);  digitalWrite(in2, HIGH); }
  else            { digitalWrite(in1, LOW);  digitalWrite(in2, LOW);  }  // coast
  writeSpeed(pwmPin, abs(s));
}

void motorA(int s) { setMotor(PWMA, AIN1, AIN2, s); }
void motorB(int s) { setMotor(PWMB, BIN1, BIN2, s); }
void motorC(int s) { setMotor(PWMC, CIN1, CIN2, s); }

void brakeAll() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); writeSpeed(PWMA, 0);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); writeSpeed(PWMB, 0);
  digitalWrite(CIN1, HIGH); digitalWrite(CIN2, HIGH); writeSpeed(PWMC, 0);
}

void coastAll() { motorA(0); motorB(0); motorC(0); }

void pwmPinsMode() {
#if defined(ARDUINO_ARCH_STM32F1)
  pinMode(PWMA, PWM); pinMode(PWMB, PWM); pinMode(PWMC, PWM);
#else
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT); pinMode(PWMC, OUTPUT);
#endif
}

// ---------------- Motor tests ----------------
void autoSequence() {
  Serial.println("Motor A forward"); motorA(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("Motor A reverse"); motorA(-speed); delay(1500); brakeAll(); delay(500);
  Serial.println("Motor B forward"); motorB(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("Motor B reverse"); motorB(-speed); delay(1500); brakeAll(); delay(500);
  Serial.println("Motor C forward"); motorC(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("Motor C reverse"); motorC(-speed); delay(1500); brakeAll(); delay(500);
  Serial.println("A+B forward");     motorA(speed);  motorB(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("A+B reverse");     motorA(-speed); motorB(-speed); delay(1500); brakeAll();
  Serial.println("Sequence done");
}

// Full speed with PWM bypassed: tests wiring/driver without PWM.
void diagnose() {
  Serial.println("DIAGNOSE: motors A, B, C full speed forward 3 s, PWM bypassed");
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT); pinMode(PWMC, OUTPUT);
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  digitalWrite(CIN1, HIGH); digitalWrite(CIN2, LOW);
  digitalWrite(PWMA, HIGH); digitalWrite(PWMB, HIGH); digitalWrite(PWMC, HIGH);
  delay(3000);
  digitalWrite(PWMA, LOW);  digitalWrite(PWMB, LOW);  digitalWrite(PWMC, LOW);
  brakeAll();
  pwmPinsMode();  // back to PWM for the other commands
  Serial.println("  Spun?     -> wiring OK, PWM is the problem");
  Serial.println("  No spin?  -> check VM, GND, STBY, VCC, motor wires");
}

// ---------------- Servos ----------------
// Move the selected servo slowly: 1 degree every servoStepMs.
void servoTo(int n, int angle) {
  angle = constrain(angle, 0, 180);
  if (!servos[n].attached()) servos[n].attach(SERVO_PINS[n]);
  Serial.print("Servo "); Serial.print(n + 1);
  Serial.print(" -> "); Serial.print(angle); Serial.println(" deg");
  while (servoAngle[n] != angle) {
    servoAngle[n] += (angle > servoAngle[n]) ? 1 : -1;
    servos[n].write(servoAngle[n]);
    delay(servoStepMs);
  }
  servos[n].write(servoAngle[n]);
}

// Move BOTH servos together, slowly, each to its own target (same as the main sketch's unfurl / furl).
void servosBothTo(const int target[2]) {
  for (int n = 0; n < 2; n++) if (!servos[n].attached()) servos[n].attach(SERVO_PINS[n]);
  while (servoAngle[0] != target[0] || servoAngle[1] != target[1]) {
    for (int n = 0; n < 2; n++) {
      if (servoAngle[n] != target[n]) servoAngle[n] += (target[n] > servoAngle[n]) ? 1 : -1;
      servos[n].write(servoAngle[n]);
    }
    delay(servoStepMs);
  }
}

void servoSweep(int n) {
  // Furled -> unfurled -> furled, so it always ends closed.
  Serial.print("Servo "); Serial.print(n + 1); Serial.println(" sweep furled -> unfurled -> furled");
  servoTo(n, FURLED_ANGLE[n]);
  servoTo(n, UNFURLED_ANGLE[n]);
  servoTo(n, FURLED_ANGLE[n]);
}

void printServoSpeed() {
  Serial.print("Servo speed: "); Serial.print(servoStepMs);
  Serial.print(" ms/deg (180 deg in ");
  Serial.print(servoStepMs * 180 / 1000.0, 1); Serial.println(" s)");
}

void printSelected() {
  Serial.print("Selected: servo "); Serial.print(sel + 1);
  Serial.print(" (pin "); Serial.print(sel == 0 ? "PB0" : "PA7");
  Serial.print(", at "); Serial.print(servoAngle[sel]); Serial.println(" deg)");
}

// ---------------- IR sensor ----------------
void printIr() {
  int d = digitalRead(IR_PIN);
  Serial.print("IR  OUT="); Serial.print(d);
  Serial.println(d == IR_CLOSE_LEVEL ? "   CLOSE (door)" : "   far");
}

// ---------------- Light sensors ----------------
int readLdr(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(pin);
  return sum / 16;
}

// Prints each sensor with its position, plus "box" = the light the main sketch
// uses for the in-box check (valid sensors except the front one).
// A reading below LDR_MIN_VALID is marked "!" and left out of avg and box, like the main sketch.
void printLight() {
  long total = 0, box = 0;
  int nTotal = 0, nBox = 0;
  Serial.print("LIGHT");
  for (int i = 0; i < NUM_LDR; i++) {
    int v = readLdr(LDR_PINS[i]);
    bool valid = v >= LDR_MIN_VALID;
    if (valid) {
      total += v; nTotal++;
      if (i != FRONT_LDR) { box += v; nBox++; }
    }
    Serial.print("  L"); Serial.print(i + 1);
    Serial.print(' '); Serial.print(LDR_NAMES[i]); Serial.print("=");
    if (v < 1000) Serial.print(' ');
    if (v < 100)  Serial.print(' ');
    if (v < 10)   Serial.print(' ');
    Serial.print(v);
    Serial.print(valid ? ' ' : '!');
  }
  Serial.print("  avg=");
  if (nTotal) Serial.print(total / nTotal); else Serial.print("--");
  Serial.print("   box(L1+L2)=");
  if (nBox) Serial.print(box / nBox); else Serial.print("--");
  if (nTotal < NUM_LDR) Serial.print("   ! = invalid (< 50), check wiring");
  Serial.println();
}

// ---------------- Help ----------------
void printHelp() {
  Serial.println();
  Serial.println("=== SpaceApps HardwareTest ===");
  Serial.println("Motors (A, B = driver 1, C = driver 2):");
  Serial.println("  g  A+B forward        r  A+B reverse");
  Serial.println("  a  motor A forward    z  motor A reverse");
  Serial.println("  b  motor B forward    n  motor B reverse");
  Serial.println("  m  motor C forward    y  motor C reverse");
  Serial.println("  s  stop all (brake)   c  stop all (coast)");
  Serial.println("  +  faster             -  slower");
  Serial.println("  t  auto test sequence (A, B, C, then A+B)");
  Serial.println("  d  diagnose (all 3 full speed, PWM bypassed)");
  Serial.println("Servos (1 = PB0, 2 = PA7) - commands act on the selected servo:");
  Serial.println("  o  switch selected servo (1 <-> 2)");
  Serial.println("  0..180  type an angle + Enter, e.g. 45");
  Serial.println("  ]  +10 deg            [  -10 deg");
  Serial.println("  w  sweep (ends furled) v  release (go limp)");
  Serial.println("  ,  servo slower       .  servo faster");
  Serial.println("  x  park BOTH servos furled at 180 (before powering off)");
  Serial.println("  u  unfurl: BOTH servos -> 0   f  furl: BOTH servos -> 180");
  Serial.println("IR sensor (OUT PA5):");
  Serial.println("  i  live readout on/off   p  print once");
  Serial.println("Light sensors (L1 rear PA4, L2 side PA2, L3 front PB1):");
  Serial.println("  l  live readout on/off   k  print once");
  Serial.println("  h  this help");
  Serial.print("Speed: "); Serial.print(speed);
  Serial.print("   Servo 1: "); Serial.print(servoAngle[0]);
  Serial.print(" deg   Servo 2: "); Serial.print(servoAngle[1]);
  Serial.print(" deg   "); Serial.print(servoStepMs); Serial.println(" ms/deg");
  printSelected();
}

// ---------------- Setup / loop ----------------
void setup() {
  Serial.begin(115200);
  delay(1500);

#if defined(ARDUINO_ARCH_STM32F1)
  // Free PA15, PB3, PB4 from JTAG so they work as normal pins if ever used.
  // With "Serial" or "STLink" upload the Roger Clark core leaves JTAG on.
  // SW_ONLY keeps SWD (PA13/PA14), so ST-Link uploads still work.
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);
#endif

  int outPins[] = {AIN1, AIN2, BIN1, BIN2, CIN1, CIN2, STBY};
  for (int p : outPins) pinMode(p, OUTPUT);
  pinMode(IR_PIN, INPUT_PULLUP);
  for (int p : LDR_PINS) pinMode(p, INPUT_ANALOG);
#if defined(ARDUINO_ARCH_STM32)
  analogReadResolution(12);   // official core defaults to 10-bit
#endif
  pwmPinsMode();
#if defined(ARDUINO_ARCH_STM32)
  analogWriteFrequency(20000);
#endif

  coastAll();
  digitalWrite(STBY, HIGH);   // enables both drivers

  // Assume both servos are furled (at 180): every program ends there.
  for (int n = 0; n < 2; n++) {
    servos[n].attach(SERVO_PINS[n]);
    servos[n].write(servoAngle[n]);
  }
  printHelp();
}

void loop() {
  if (irWatch && millis() - lastIrPrint >= 200) {
    lastIrPrint = millis();
    printIr();
  }
  if (lightWatch && millis() - lastLightPrint >= 200) {
    lastLightPrint = millis();
    printLight();
  }

  if (!Serial.available()) return;

  // A number = angle for the selected servo
  if (isDigit(Serial.peek())) {
    servoTo(sel, Serial.parseInt());
    return;
  }

  char cmd = Serial.read();
  if (cmd == '\n' || cmd == '\r' || cmd == ' ') return;

  switch (cmd) {
    // Servos
    case 'o': sel = 1 - sel; printSelected(); break;
    case ']': servoTo(sel, servoAngle[sel] + 10); break;
    case '[': servoTo(sel, servoAngle[sel] - 10); break;
    case 'w': servoSweep(sel); break;
    case 'v': servos[sel].detach(); Serial.print("Servo "); Serial.print(sel + 1); Serial.println(" released"); break;
    case 'x': servosBothTo(FURLED_ANGLE); Serial.println("Both servos parked furled at 180 - safe to power off"); break;
    case 'u': Serial.println("Unfurl: both servos -> 0"); servosBothTo(UNFURLED_ANGLE); Serial.println("Unfurled"); break;
    case 'f': Serial.println("Furl: both servos -> 180"); servosBothTo(FURLED_ANGLE); Serial.println("Furled - safe to power off"); break;
    case ',': servoStepMs = (servoStepMs == 0) ? 5 : min(servoStepMs * 2, 200); printServoSpeed(); break;
    case '.': servoStepMs = (servoStepMs <= 5) ? 0 : servoStepMs / 2;           printServoSpeed(); break;

    // Sensors
    case 'i': irWatch = !irWatch; Serial.println(irWatch ? "IR live ON" : "IR live OFF"); break;
    case 'p': printIr(); break;
    case 'l': lightWatch = !lightWatch; Serial.println(lightWatch ? "Light live ON" : "Light live OFF"); break;
    case 'k': printLight(); break;

    // Motors
    case 'g': Serial.println("A+B forward");     motorA(speed);  motorB(speed);  break;
    case 'r': Serial.println("A+B reverse");     motorA(-speed); motorB(-speed); break;
    case 'a': Serial.println("Motor A forward"); motorA(speed);  break;
    case 'z': Serial.println("Motor A reverse"); motorA(-speed); break;
    case 'b': Serial.println("Motor B forward"); motorB(speed);  break;
    case 'n': Serial.println("Motor B reverse"); motorB(-speed); break;
    case 'm': Serial.println("Motor C forward"); motorC(speed);  break;
    case 'y': Serial.println("Motor C reverse"); motorC(-speed); break;
    case 's': Serial.println("Brake all");       brakeAll();     break;
    case 'c': Serial.println("Coast all");       coastAll();     break;
    case '+': speed = min(speed + 25, 255); Serial.print("Speed: "); Serial.println(speed); break;
    case '-': speed = max(speed - 25, 0);   Serial.print("Speed: "); Serial.println(speed); break;
    case 't': autoSequence(); break;
    case 'd': diagnose();     break;

    case 'h': printHelp();    break;
    default:  Serial.print("Unknown command: "); Serial.println(cmd); printHelp();
  }
}
