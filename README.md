# SpaceApps

Sketch for an STM32F103C8T6 "Blue Pill" that drives two DC motors through a TB6612FNG motor driver.

## Hardware
- STM32F103C8T6 Blue Pill (STM32duino core)
- TB6612FNG dual motor driver breakout
- 2× DC motors
- Motor battery, 4.5–13.5 V

## Wiring

| TB6612FNG | Blue Pill / power | Notes |
|---|---|---|
| VM   | Motor battery + | 4.5–13.5 V; put a 10–100 µF cap across VM/GND |
| VCC  | **3.3** | Logic runs at 3.3 V to match the STM32 |
| GND  | Blue Pill G **and** battery − | All grounds must be common |
| STBY | PA8  | HIGH = enabled |
| PWMA | PA0  | PWM (TIM2_CH1), Motor A speed |
| AIN1 | PB12 | Motor A direction |
| AIN2 | PB13 | Motor A direction |
| PWMB | PA1  | PWM (TIM2_CH2), Motor B speed |
| BIN1 | PB14 | Motor B direction |
| BIN2 | PB15 | Motor B direction |
| A01 / A02 | Motor A terminals | Swap them if the motor spins backwards |
| B01 / B02 | Motor B terminals | Swap them if the motor spins backwards |

Pins kept free: PA9/PA10 (serial upload), PA11/PA12 (USB), PA13/PA14 (SWD), PC13 (LED), PB2 (BOOT1).

## Powering the Blue Pill
The TB6612FNG does not supply power. Feed the Blue Pill through USB, the ST-Link's 3.3 V, or a regulated 5 V on its **5V** pin. Never connect the motor battery directly to the Blue Pill.

Limits: about 1.2 A continuous (3.2 A peak) per channel.
