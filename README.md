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
| `panel_servo.h` | Slow, non-blocking servo moves |
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
| BOOT | 2 s of outside light averaged and logged (servo assumed at 0°) |
| STOW_SERVO | Servo slowly to the in-box angle (0°, so already there) |
| WAIT_IN_BOX | Door within ~5 cm **and** light < 50% of outside, for 2 s; inside light logged |
| WAIT_DOOR_OPEN | IR no longer sees the door for 1 s |
| DOOR_SETTLE | 5 s timer |
| DRIVE_OUT | Drives slowly; light above the inside/outside midpoint for 2 s, then 5 s more. Safety stop after 20 s |
| STABILIZE | Stands still 3 s |
| PARK_SERVO | Servo slowly back to 0° (start/end angle) |
| READY_TO_UNFURL | Next step to build: unfurl panels |

## Hardware test (`HardwareTest/HardwareTest.ino`)
A separate sketch to test all hardware on its own: 3 motors, 2 servos, the IR sensor and 4 light sensors. It uses the pins in [`pinout.md`](pinout.md). Open it in the Arduino IDE, upload it, and type commands in the Serial Monitor (115200 baud, Newline):

| Key | Action | Key | Action |
|---|---|---|---|
| `f` | Motors A+B forward | `r` | Motors A+B reverse |
| `a` | Motor A forward | `z` | Motor A reverse |
| `b` | Motor B forward | `n` | Motor B reverse |
| `m` | Motor C forward | `u` | Motor C reverse |
| `s` | Brake all | `c` | Coast all |
| `+` | Faster | `-` | Slower |
| `t` | Auto test sequence | `d` | Diagnose (all 3 full speed, PWM bypassed) |
| `o` | Switch selected servo (1 ↔ 2) | | |
| `0`–`180` + Enter | Selected servo to that angle | `w` | Selected servo sweep 0–180–0 |
| `]` | Selected servo +10° | `[` | Selected servo −10° |
| `v` | Release selected servo (go limp) | `x` | Park **both** servos at 0° (before powering off) |
| `,` | Servos slower | `.` | Servos faster |
| `i` | IR live readout on/off | `p` | IR reading once |
| `l` | Light live readout on/off (L1–L4 + average) | `k` | Light reading once |
| `h` | Help | | |

The main sketch (`config.h`) does **not** use the new pinout yet.

## Servo rule
Every program ends with the servo at **0°**, so at power-up the code assumes it is at 0° (`SERVO_PARK_ANGLE` in config.h). The mission makes sure it is at 0° before entering the box and at the end. In HardwareTest, press `x` before powering off. If a run is cut short (reset or power off) with the servo elsewhere, it snaps to 0° at the next power-up.
