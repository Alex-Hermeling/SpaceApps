# SpaceApps

Rover firmware for an STM32F103C8T6 "Blue Pill". It detects that it is inside a box, waits for the door to open, drives slowly out, then stops ready to unfurl its solar panels.

## Hardware
- STM32F103C8T6 Blue Pill (official STMicroelectronics core or Roger Clark core)
- TB6612FNG dual motor driver + 2× DC motors, 4× AA on VM
- LDR (photoresistor) + 10 kΩ resistor
- IR distance/obstacle sensor pointing at the door

## Files
| File | Contents |
|---|---|
| `SpaceApps.ino` | Mission state machine |
| `config.h` | Pins, thresholds, timings, `TEST_MODE`, `IR_MODE` |
| `motors.h` | TB6612FNG motor control and motor tests |
| `sensors.h` | Light and IR door sensor |

## Wiring

### Motor driver
| TB6612FNG | Blue Pill / power | Notes |
|---|---|---|
| VM   | Battery + (4× AA) | Put a 10–100 µF cap across VM/GND |
| VCC  | 3.3 | Logic |
| GND  | G **and** battery − | All grounds common |
| STBY | PA8  | HIGH = enabled |
| PWMA | PA0  | Motor A speed |
| AIN1 | PB12 | Motor A direction |
| AIN2 | PB13 | Motor A direction |
| PWMB | PA1  | Motor B speed |
| BIN1 | PB14 | Motor B direction |
| BIN2 | PB15 | Motor B direction |
| A01/A02, B01/B02 | Motors | Swap a motor's wires if it spins backwards |

### Sensors
| Part | Blue Pill | Notes |
|---|---|---|
| LDR | 3.3 → LDR → **PA4** → 10 kΩ → G | Brighter = higher reading |
| IR sensor VCC / GND | 3.3 / G | |
| IR sensor OUT | **PA5** | Set `IR_MODE` in config.h (digital module or analog) |

Pins kept free: PA9/PA10 (serial upload), PA11/PA12 (USB), PA13/PA14 (SWD), PC13 (LED), PB2 (BOOT1).

Never connect the battery to the Blue Pill. Power it from USB or an ST-Link.

## Mission states
| State | Moves on when |
|---|---|
| BOOT | 2 s of outside light averaged and logged |
| WAIT_IN_BOX | Door within ~5 cm **and** light < 50% of outside, for 2 s; inside light logged |
| WAIT_DOOR_OPEN | IR no longer sees the door for 1 s |
| DOOR_SETTLE | 5 s timer |
| DRIVE_OUT | Drives slowly; light above the inside/outside midpoint for 2 s, then 1.5 s more. Safety stop after 10 s |
| STABILIZE | Stands still 3 s |
| READY_TO_UNFURL | Next step to build: unfurl panels |

## Test modes (`TEST_MODE` in config.h)
- `0`: mission
- `1`: motor test loop (forward / brake / reverse / coast)
- `2`: driver diagnose (full speed 5 s, PWM bypassed)

Serial Monitor: 115200 baud.
