// SpaceApps
// Two DC motors driven by a TB6612FNG dual H-bridge.
// Pin map assumes an Arduino Uno / Nano (PWM on 3, 5, 6, 9, 10, 11).

// ---- Motor A ----
const int PWMA = 5;   // speed (PWM)
const int AIN1 = 4;   // direction
const int AIN2 = 7;   // direction

// ---- Motor B ----
const int PWMB = 6;   // speed (PWM)
const int BIN1 = 8;   // direction
const int BIN2 = 12;  // direction

// ---- Standby ----
const int STBY = 9;   // HIGH = driver enabled, LOW = driver asleep

// speed: -255 (full reverse) .. 0 (stop) .. 255 (full forward)
void setMotor(int pwmPin, int in1, int in2, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    // Both LOW = coast. (Both HIGH would be a short brake.)
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
  analogWrite(pwmPin, abs(speed));
}

void motorA(int speed) { setMotor(PWMA, AIN1, AIN2, speed); }
void motorB(int speed) { setMotor(PWMB, BIN1, BIN2, speed); }

void brakeAll() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); analogWrite(PWMA, 0);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); analogWrite(PWMB, 0);
}

void setup() {
  Serial.begin(115200);

  int pins[] = {PWMA, AIN1, AIN2, PWMB, BIN1, BIN2, STBY};
  for (int p : pins) pinMode(p, OUTPUT);

  motorA(0);
  motorB(0);
  digitalWrite(STBY, HIGH);  // wake the driver

  Serial.println("SpaceApps: TB6612FNG ready");
}

// Simple test: each motor forward, then reverse, then stop.
void loop() {
  Serial.println("Forward");
  motorA(180); motorB(180);
  delay(2000);

  Serial.println("Brake");
  brakeAll();
  delay(1000);

  Serial.println("Reverse");
  motorA(-180); motorB(-180);
  delay(2000);

  Serial.println("Coast");
  motorA(0); motorB(0);
  delay(2000);
}
