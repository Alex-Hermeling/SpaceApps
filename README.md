# SpaceApps

Rover firmware for an STM32F103C8T6 "Blue Pill". It detects that it is inside a box, waits for the door to open, drives out (full speed, 2.5 s), stops, and unfurls its solar panels (both servos 180° → 0°; 180° = closed). It then stays unfurled: it never refurls on its own.

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
| STABILIZE | Stands still 3 s |
| PARK_SERVO | Makes sure both servos are furled (already there) |
| UNFURL | Both servos slowly 180° → 0° |
| UNFURLED | **Final state.** Stays unfurled. `f` in the Serial Monitor furls by hand (for packing up) |
| FURL | Manual only (`f`): both servos slowly 0° → 180° |
| FURLED | Safe to power off. `u` unfurls again |

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

A reading **below 50** is treated as invalid (loose wire, short to ground, broken sensor; real darkness reads about 400). Invalid sensors are left out of the box check and are marked `!` in the serial log (`LDR_MIN_VALID` in config.h).

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
| `e` | Read STBY pin (PB5): HIGH = drivers enabled, LOW = standby | | |
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
At power-up the code assumes the panel is **furled**: both servos at **180°** (both are mounted mirrored, `SERVO_REVERSED` in config.h). The mission ends **unfurled** and never refurls, so **furl it by hand before the next run**: type `f` in the main sketch's Serial Monitor, or press `x` in HardwareTest. If a run is cut short (reset or power off) with the servos elsewhere, they snap to furled at the next power-up.
