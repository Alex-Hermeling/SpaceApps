# SpaceApps

Rover firmware for an STM32F103C8T6 "Blue Pill". It detects that it is inside a box, waits for the door to open, drives slowly out, then stops ready to unfurl its solar panels.

## Hardware
- STM32F103C8T6 Blue Pill (official STMicroelectronics core or Roger Clark core)
- TB6612FNG dual motor driver + 2× DC motors, 4× AA on VM
- LDR (photoresistor) + 8 kΩ resistor (4.7–22 kΩ works)
- TCRT5000 IR reflective sensor module pointing at the door

## Files
| File | Contents |
|---|---|
| `SpaceApps.ino` | Mission state machine |
| `config.h` | Pins, thresholds, timings |
| `motors.h` | TB6612FNG motor control |
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
| LDR | 3.3 → LDR → **PA4** → 8 kΩ → G | Brighter = higher reading |
| TCRT5000 VCC / GND | 3.3 / G | |
| TCRT5000 OUT | **PA5** | HIGH when the door is close on this module (`IR_CLOSE_LEVEL` in config.h); set range with the module's potentiometer |

The TCRT5000 only sees about 1–2 cm reliably. Mount it close to the door, and put white tape on the door where the sensor points.

### Servo
| Servo wire | Connect to | Notes |
|---|---|---|
| Orange / yellow (signal) | **PA6** | Timer 3; must not share timer 2 with the motor PWM pins |
| Red (+) | Blue Pill **5V** (small SG90 on USB) or 4× AA + | Bigger servos: use the battery or a 5V regulator |
| Brown / black (−) | **G** | Common ground |

Pins kept free: PA9/PA10 (serial upload), PA11/PA12 (USB), PA13/PA14 (SWD), PC13 (LED), PB2 (BOOT1).

Never connect the battery to the Blue Pill. Power it from USB or an ST-Link.

## Mission states
| State | Moves on when |
|---|---|
| BOOT | 2 s of outside light averaged and logged |
| WAIT_IN_BOX | Door within ~5 cm **and** light < 50% of outside, for 2 s; inside light logged |
| WAIT_DOOR_OPEN | IR no longer sees the door for 1 s |
| DOOR_SETTLE | 5 s timer |
| DRIVE_OUT | Drives slowly; light above the inside/outside midpoint for 2 s, then 5 s more. Safety stop after 20 s |
| STABILIZE | Stands still 3 s |
| READY_TO_UNFURL | Next step to build: unfurl panels |

## Hardware test (`HardwareTest/HardwareTest.ino`)
A separate sketch to test the motors, servo, IR sensor and light sensor on their own. Open it in the Arduino IDE, upload it, and type commands in the Serial Monitor (115200 baud):

| Key | Action | Key | Action |
|---|---|---|---|
| `f` | Both forward | `r` | Both reverse |
| `a` | Motor A forward | `z` | Motor A reverse |
| `b` | Motor B forward | `n` | Motor B reverse |
| `s` | Brake | `c` | Coast |
| `+` | Faster | `-` | Slower |
| `t` | Auto test sequence | `d` | Diagnose (full speed, PWM bypassed) |
| `0`–`180` + Enter | Servo to that angle | `w` | Servo sweep 0–180–0 |
| `]` | Servo +10° | `[` | Servo −10° |
| `v` | Release servo (go limp) | `h` | Help |
| `,` | Servo slower | `.` | Servo faster |
| `i` | IR live readout on/off | `p` | IR reading once |
| `l` | Light live readout on/off | `k` | Light reading once |

Its pins are copied from `config.h`, so keep them the same if you change them.
