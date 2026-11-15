# SpaceApps

Rover firmware for an STM32F103C8T6 "Blue Pill". It detects that it is inside a box, waits for the door to open, drives out (full speed, 2.5 s), stops, and unfurls its solar panels (both servos 180° → 0°; 180° = closed). When a sandstorm comes (any one light sensor dark for 2 s) it furls (both servos back to 180°), and unfurls again once that same sensor sees light for 10 s.

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
| BOOT | 2 s of outside light (average of L1 rear + L2 side, front sensor L3 ignored) logged; servos assumed furled (both at 180°) |
| STOW_SERVO | Both servos slowly to furled (already there) |
| WAIT_IN_BOX | Door within ~2 cm (IR, set with the module's potentiometer) **and** L1 + L2 light < 50% of outside, for 2 s; front sensor L3 ignored |
| WAIT_DOOR_OPEN | IR no longer sees the door for 1 s. After this the IR sensor is ignored (it is close to the ground and could false-trigger while driving) |
| DOOR_SETTLE | 5 s timer |
| DRIVE_OUT | Full speed for 2.5 s (`DRIVE_SPEED`, `DRIVE_TIME_MS` in config.h), then brakes |
| STABILIZE | Stands still 3 s; logs each sensor's surface light (sandstorm reference) |
| PARK_SERVO | Both servos slowly back to furled (start/end position) |
| UNFURL | Both servos slowly 180° → 0° |
| UNFURLED | Sandstorm: **any one** sensor below 30% of its own surface light for 2 s. `f` in the Serial Monitor forces it |
| FURL | Both servos slowly 0° → 180° (safe to power off) |
| FURLED | Sandstorm over: **the sensor that went dark** is above 60% of its surface light for 10 s, then unfurls again (loops). After a manual furl only `u` unfurls |

### Status LED (onboard, PC13)
| State | LED |
|---|---|
| BOOT | Fast blink |
| STOW_SERVO, PARK_SERVO, UNFURL, FURL | Fast flicker (servos moving) |
| WAIT_IN_BOX | 1 short blink per second |
| WAIT_DOOR_OPEN | Double blink |
| DOOR_SETTLE | Medium blink (5 s countdown) |
| DRIVE_OUT | Solid on |
| STABILIZE | Triple blink |
| UNFURLED | Short heartbeat every 2 s |
| FURLED | Slow: 1 s on, 1 s off |

Patterns are in `LED_PATTERNS` at the top of `SpaceApps.ino`.

A loose light sensor floats at about 1000–2000. The dark threshold (30% of a lit reading of about 3000, so about 900) sits below that, so a loose sensor can't fake a sandstorm. Keep it that way when tuning `STORM_DARK_FRACTION`.

A reading **below 50** is treated as invalid (loose wire, short to ground, broken sensor; real darkness reads about 400). Invalid sensors are left out of the box check and the sandstorm check, and are marked `!` in the serial log (`LDR_MIN_VALID` in config.h). If the sensor that triggered a furl goes invalid, the rover stays furled until `u`.

## Hardware test (`HardwareTest/HardwareTest.ino`)
A separate sketch to test all hardware on its own: 3 motors, 2 servos, the IR sensor and 3 light sensors. It uses the pins in [`pinout.md`](pinout.md). Open it in the Arduino IDE, upload it, and type commands in the Serial Monitor (115200 baud, Newline):

| Key | Action | Key | Action |
|---|---|---|---|
| `g` | Motors A+B forward | `r` | Motors A+B reverse |
| `a` | Motor A forward | `z` | Motor A reverse |
| `b` | Motor B forward | `n` | Motor B reverse |
| `m` | Motor C forward | `y` | Motor C reverse |
| `s` | Brake all | `c` | Coast all |
| `+` | Faster | `-` | Slower |
| `t` | Auto test sequence | `d` | Diagnose (all 3 full speed, PWM bypassed) |
| `o` | Switch selected servo (1 ↔ 2) | | |
| `0`–`180` + Enter | Selected servo to that angle | `w` | Selected servo sweep furled → unfurled → furled |
| `]` | Selected servo +10° | `[` | Selected servo −10° |
| `v` | Release selected servo (go limp) | `x` | Park **both** servos furled at 180° (before powering off) |
| `u` | Unfurl: **both** servos → 0° | `f` | Furl: **both** servos → 180° |
| `,` | Servos slower | `.` | Servos faster |
| `i` | IR live readout on/off | `p` | IR reading once |
| `l` | Light live readout on/off (L1 rear, L2 side, L3 front, average, box = L1+L2) | `k` | Light reading once |
| `h` | Help | | |

## Servo rule
Every program ends **furled**: both servos at **180°** (both are mounted mirrored, `SERVO_REVERSED` in config.h). At power-up the code assumes they are there. The mission makes sure the panel is furled before entering the box and at the end. In HardwareTest, press `x` before powering off. If a run is cut short (reset or power off) with the servos elsewhere, they snap to furled at the next power-up.
