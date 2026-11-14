# SpaceApps

Arduino sketch that drives two DC motors with a TB6612FNG motor driver.

## Hardware
- Arduino Uno / Nano (the pin map assumes one of these)
- TB6612FNG dual motor driver breakout
- 2× DC motors
- Motor battery, 4.5–13.5 V

## Wiring

| TB6612FNG | Connect to | Notes |
|---|---|---|
| VM   | Motor battery + | 4.5–13.5 V; put a 10–100 µF cap across VM/GND |
| VCC  | Arduino 5V | Logic supply |
| GND  | Arduino GND **and** battery − | All grounds must be common |
| STBY | D9  | HIGH = enabled |
| PWMA | D5  | PWM, Motor A speed |
| AIN1 | D4  | Motor A direction |
| AIN2 | D7  | Motor A direction |
| PWMB | D6  | PWM, Motor B speed |
| BIN1 | D8  | Motor B direction |
| BIN2 | D12 | Motor B direction |
| A01 / A02 | Motor A terminals | Swap them if the motor spins backwards |
| B01 / B02 | Motor B terminals | Swap them if the motor spins backwards |

Limits: about 1.2 A continuous (3.2 A peak) per channel.
