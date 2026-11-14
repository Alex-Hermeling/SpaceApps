// SpaceApps
// Two DC motors driven by a TB6612FNG dual H-bridge.
// Board: STM32F103C8T6 "Blue Pill" (STM32duino core). 3.3V logic.
// Avoided pins: PA9/PA10 (serial upload), PA11/PA12 (USB),
// PA13/PA14 (SWD / ST-Link), PC13 (onboard LED), PB2 (BOOT1).

// ---- Motor A ----
const int PWMA = PA0;   // speed (PWM, TIM2_CH1)
const int AIN1 = PB12;  // direction
const int AIN2 = PB13;  // direction

// ---- Motor B ----
const int PWMB = PA1;   // speed (PWM, TIM2_CH2)
const int BIN1 = PB14;  // direction
const int BIN2 = PB15;  // direction

// ---- Standby ----
const int STBY = PA8;   // HIGH = driver enabled, LOW = driver asleep

// Write a 0..255 duty cycle to a PWM pin.
// The Roger Clark core (ARDUINO_ARCH_STM32F1) needs the pin in PWM mode and
// uses 16-bit duty (default timer overflow 65535), so scale 0..255 -> 0..65535.
void writeSpeed(int pin, int duty) {
#if defined(ARDUINO_ARCH_STM32F1)
  pwmWrite(pin, duty * 257);
#else
  analogWrite(pin, duty);
#endif
}

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
  writeSpeed(pwmPin, abs(speed));
}

void motorA(int speed) { setMotor(PWMA, AIN1, AIN2, speed); }
void motorB(int speed) { setMotor(PWMB, BIN1, BIN2, speed); }

void brakeAll() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH); writeSpeed(PWMA, 0);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH); writeSpeed(PWMB, 0);
}

void setup() {
  Serial.begin(115200);

  int pins[] = {AIN1, AIN2, BIN1, BIN2, STBY};
  for (int p : pins) pinMode(p, OUTPUT);

#if defined(ARDUINO_ARCH_STM32F1)
  // Roger Clark core: OUTPUT mode disconnects the timer, so PWM pins need PWM mode.
  pinMode(PWMA, PWM);
  pinMode(PWMB, PWM);
#else
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
#endif

  // 20 kHz PWM is above hearing range, so the motors don't whine.
  // (TB6612FNG supports up to 100 kHz.) Only the official STMicroelectronics
  // core has analogWriteFrequency(); the Roger Clark core uses its default PWM.
#if defined(ARDUINO_ARCH_STM32)
  analogWriteFrequency(20000);
#endif

  motorA(0);
  motorB(0);
  digitalWrite(STBY, HIGH);  // wake the driver

  Serial.println("SpaceApps: TB6612FNG ready");
}

// ---- Distance driving (time-based, no encoders) ----
const int   DRIVE_SPEED  = 180;   // 0..255
// How far the robot travels per second at DRIVE_SPEED.
// GUESS - calibrate: set CALIBRATE to true, measure how far it goes in 2 s,
// then set CM_PER_SEC = measured_cm / 2.0
const float CM_PER_SEC   = 25.0;
const bool  CALIBRATE    = false;
const float TARGET_CM    = 30.0;

void driveDistance(float cm) {
  unsigned long ms = (unsigned long)(cm / CM_PER_SEC * 1000.0);
  Serial.print("Driving "); Serial.print(cm); Serial.print(" cm (");
  Serial.print(ms); Serial.println(" ms)");
  motorA(DRIVE_SPEED); motorB(DRIVE_SPEED);
  delay(ms);
  brakeAll();
}

bool done = false;

void loop() {
  if (done) return;  // run once; press RESET on the Blue Pill to go again

  Serial.println("Starting in 3 s...");
  delay(3000);

  if (CALIBRATE) {
    Serial.println("Calibration: driving 2 s - measure the distance");
    motorA(DRIVE_SPEED); motorB(DRIVE_SPEED);
    delay(2000);
    brakeAll();
  } else {
    driveDistance(TARGET_CM);
  }

  Serial.println("Done");
  done = true;
}
