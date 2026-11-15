# SpaceApps

Rover firmware for an STM32F103C8T6 "Blue Pill". It detects that it is inside a box, waits for the door to open, drives slowly out, then stops ready to unfurl its solar panels.

## Hardware
- STM32F103C8T6 Blue Pill (Roger Clark STM32 core)
- 2× TB6612FNG motor drivers: driver #1 runs motors A + B (drive wheels), driver #2 runs motor C on channel A
- 3× DC motors, 4× AA on VM
- 2× servos
- 3× LDR (photoresistor), each with an 8 kΩ resistor (4.7–22 kΩ works)
- TCRT5000 IR reflective sensor module (3-pin) pointing at the door

## Files
| File | Contents |
|---|---|
| `SpaceApps.ino` | Mission state machine |
| `config.h` | Pins, thresholds, timings |
| `motors.h` | Motors A + B (drive) and C |
| `panel_servo.h` | Slow, non-blocking servo moves (both servos together) |
| `sensors.h` | 3 light sensors (mission uses the average) and IR door sensor |

## Wiring
See **[pinout.md](pinout.md)** for every pin, power wiring, free pins and pin rules.

## Mission states
| State | Moves on when |
|---|---|
| BOOT | 2 s of outside light (average of the 3 sensors) logged; servos assumed at 0° |
| STOW_SERVO | Both servos slowly to the in-box angle (0°, so already there) |
| WAIT_IN_BOX | Door within ~5 cm **and** light < 50% of outside, for 2 s; inside light logged |
| WAIT_DOOR_OPEN | IR no longer sees the door for 1 s |
| DOOR_SETTLE | 5 s timer |
| DRIVE_OUT | Drives slowly; light above the inside/outside midpoint for 2 s, then 5 s more. Safety stop after 20 s |
| STABILIZE | Stands still 3 s |
| PARK_SERVO | Both servos slowly back to 0° (start/end angle) |
| READY_TO_UNFURL | Next step to build: unfurl panels |

## Hardware test (`HardwareTest/HardwareTest.ino`)
A separate sketch to test all hardware on its own: 3 motors, 2 servos, the IR sensor and 3 light sensors. It uses the pins in [`pinout.md`](pinout.md). Open it in the Arduino IDE, upload it, and type commands in the Serial Monitor (115200 baud, Newline):

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
| `l` | Light live readout on/off (L1–L3 + average) | `k` | Light reading once |
| `h` | Help | | |

## Servo rule
Every program ends with both servos at **0°**, so at power-up the code assumes they are at 0° (`SERVO_PARK_ANGLE` in config.h). The mission makes sure it is at 0° before entering the box and at the end. In HardwareTest, press `x` before powering off. If a run is cut short (reset or power off) with the servo elsewhere, it snaps to 0° at the next power-up.
