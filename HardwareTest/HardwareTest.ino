// HardwareTest - standalone test for all SpaceApps hardware: motors, servo, IR sensor, light sensor.
// Open this file on its own in the Arduino IDE and upload it.
// Serial Monitor: 115200 baud, line ending "Newline".
//
// Pins MUST match ../config.h (Arduino can't include files from a parent folder).

#include <Servo.h>

// ---- Servo ----
// Must NOT be on timer 2 (PA0/PA1 motor PWM): the Servo library takes over the whole timer.
// PA6 = TIM3_CH1.
const int SERVO_PIN = PA6;
Servo servo;
int servoAngle = 90;

// ---- TCRT5000 IR sensor (same as config.h) ----
const int IR_PIN = PA5;     // OUT: LOW = something close
bool irWatch = false;       // live IR readout on/off
unsigned long lastIrPrint = 0;

// ---- LDR light sensor (same as config.h) ----
// 3.3V -> LDR -> PA4 -> 8k -> GND  (brighter = higher value, 0..4095)
const int LDR_PIN = PA4;
bool lightWatch = false;    // live light readout on/off
unsigned long lastLightPrint = 0;

// ---- Motor pins (same as config.h) ----
const int PWMA = PA0;
const int AIN1 = PB12;
const int AIN2 = PB13;
const int PWMB = PA1;
const int BIN1 = PB14;
const int BIN2 = PB15;
const int STBY = PA8;

int speed = 100;  // 0..255, change with + / -

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

void brakeAll() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); writeSpeed(PWMA, 0);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); writeSpeed(PWMB, 0);
}

void pwmPinsMode() {
#if defined(ARDUINO_ARCH_STM32F1)
  pinMode(PWMA, PWM); pinMode(PWMB, PWM);
#else
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
#endif
}

// ---------------- Tests ----------------
void autoSequence() {
  Serial.println("Motor A forward");  motorA(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("Motor A reverse");  motorA(-speed); delay(1500); brakeAll(); delay(500);
  Serial.println("Motor B forward");  motorB(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("Motor B reverse");  motorB(-speed); delay(1500); brakeAll(); delay(500);
  Serial.println("Both forward");     motorA(speed);  motorB(speed);  delay(1500); brakeAll(); delay(500);
  Serial.println("Both reverse");     motorA(-speed); motorB(-speed); delay(1500); brakeAll();
  Serial.println("Sequence done");
}

// Full speed with PWM bypassed: tests wiring/driver without PWM.
void diagnose() {
  Serial.println("DIAGNOSE: both full speed forward 3 s, PWM bypassed");
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  digitalWrite(PWMA, HIGH); digitalWrite(PWMB, HIGH);
  delay(3000);
  digitalWrite(PWMA, LOW);  digitalWrite(PWMB, LOW);
  brakeAll();
  pwmPinsMode();  // back to PWM for the other commands
  Serial.println("  Spun?     -> wiring OK, PWM is the problem");
  Serial.println("  No spin?  -> check VM, GND, STBY, VCC, motor wires");
}

// ---------------- Servo ----------------
void servoTo(int angle) {
  servoAngle = constrain(angle, 0, 180);
  if (!servo.attached()) servo.attach(SERVO_PIN);
  servo.write(servoAngle);
  Serial.print("Servo -> "); Serial.print(servoAngle); Serial.println(" deg");
}

void servoSweep() {
  Serial.println("Servo sweep 0 -> 180 -> 0");
  if (!servo.attached()) servo.attach(SERVO_PIN);
  for (int a = 0; a <= 180; a += 5)  { servo.write(a); delay(30); }
  for (int a = 180; a >= 0; a -= 5)  { servo.write(a); delay(30); }
  servoTo(90);
}

// ---------------- IR sensor ----------------
void printIr() {
  int d = digitalRead(IR_PIN);
  Serial.print("IR  OUT="); Serial.print(d);
  Serial.println(d == LOW ? "   CLOSE (door)" : "   far");
}

// ---------------- Light sensor ----------------
void printLight() {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(LDR_PIN);
  int v = sum / 16;
  Serial.print("LIGHT "); Serial.print(v);
  Serial.print("  ");
  for (int i = 0; i < v / 205; i++) Serial.print('#');  // 0..20 char bar
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("=== SpaceApps HardwareTest ===");
  Serial.println("Motors:");
  Serial.println("  f  both forward       r  both reverse");
  Serial.println("  a  motor A forward    z  motor A reverse");
  Serial.println("  b  motor B forward    n  motor B reverse");
  Serial.println("  s  stop (brake)       c  stop (coast)");
  Serial.println("  +  faster             -  slower");
  Serial.println("  t  auto test sequence");
  Serial.println("  d  diagnose (full speed, PWM bypassed)");
  Serial.println("Servo (PA6):");
  Serial.println("  0..180  type an angle + Enter, e.g. 45");
  Serial.println("  ]  +10 deg            [  -10 deg");
  Serial.println("  w  sweep 0-180-0      v  release (go limp)");
  Serial.println("IR sensor (OUT PA5):");
  Serial.println("  i  live readout on/off   p  print once");
  Serial.println("Light sensor (LDR PA4):");
  Serial.println("  l  live readout on/off   k  print once");
  Serial.println("  h  this help");
  Serial.print("Speed: "); Serial.print(speed);
  Serial.print("   Servo: "); Serial.print(servoAngle); Serial.println(" deg");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  int pins[] = {AIN1, AIN2, BIN1, BIN2, STBY};
  for (int p : pins) pinMode(p, OUTPUT);
  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT_ANALOG);
#if defined(ARDUINO_ARCH_STM32)
  analogReadResolution(12);  // official core defaults to 10-bit
#endif
  pwmPinsMode();

#if defined(ARDUINO_ARCH_STM32)
  analogWriteFrequency(20000);
#endif

  motorA(0); motorB(0);
  digitalWrite(STBY, HIGH);

  servoTo(90);  // centre the servo
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

  // A number = servo angle
  if (isDigit(Serial.peek())) {
    servoTo(Serial.parseInt());
    return;
  }

  char cmd = Serial.read();
  if (cmd == '\n' || cmd == '\r' || cmd == ' ') return;

  switch (cmd) {
    case ']': servoTo(servoAngle + 10); break;
    case '[': servoTo(servoAngle - 10); break;
    case 'w': servoSweep(); break;
    case 'v': servo.detach(); Serial.println("Servo released"); break;
    case 'i': irWatch = !irWatch; Serial.println(irWatch ? "IR live ON" : "IR live OFF"); break;
    case 'p': printIr(); break;
    case 'l': lightWatch = !lightWatch; Serial.println(lightWatch ? "Light live ON" : "Light live OFF"); break;
    case 'k': printLight(); break;
    case 'f': Serial.println("Both forward");    motorA(speed);  motorB(speed);  break;
    case 'r': Serial.println("Both reverse");    motorA(-speed); motorB(-speed); break;
    case 'a': Serial.println("Motor A forward"); motorA(speed);  break;
    case 'z': Serial.println("Motor A reverse"); motorA(-speed); break;
    case 'b': Serial.println("Motor B forward"); motorB(speed);  break;
    case 'n': Serial.println("Motor B reverse"); motorB(-speed); break;
    case 's': Serial.println("Brake");           brakeAll();     break;
    case 'c': Serial.println("Coast");           motorA(0); motorB(0); break;
    case '+': speed = min(speed + 25, 255); Serial.print("Speed: "); Serial.println(speed); break;
    case '-': speed = max(speed - 25, 0);   Serial.print("Speed: "); Serial.println(speed); break;
    case 't': autoSequence(); break;
    case 'd': diagnose();     break;
    case 'h': printHelp();    break;
    default:  Serial.print("Unknown command: "); Serial.println(cmd); printHelp();
  }
}
