# SpaceApps pinout

Board: STM32F103C8T6 "Blue Pill" (Roger Clark STM32 core).
Used by `config.h` (main sketch) and `HardwareTest/HardwareTest.ino`. Motors A and B tested working with this pinout.
On the Blue Pill, **G** = GND.

## Motor driver #1 (TB6612FNG, 2 channels)
| Driver pin | Blue Pill | Notes |
|---|---|---|
| PWMA | **PA0** | Motor A speed (PWM, timer 2) |
| AIN1 | **PB15** | Motor A direction |
| AIN2 | **PB14** | Motor A direction |
| PWMB | **PA1** | Motor B speed (PWM, timer 2) |
| BIN1 | **PB6** | Motor B direction |
| BIN2 | **PB7** | Motor B direction |
| STBY | **PB5** | HIGH = enabled, shared with driver #2 |
| VCC | 3.3 | Logic |
| VM | Battery + | 4.5–13.5 V; 10–100 µF cap across VM/GND |
| GND | G **and** battery − | |
| A01 / A02 | Motor A | Swap wires if it spins backwards |
| B01 / B02 | Motor B | Swap wires if it spins backwards |

## Motor driver #2 (TB6612FNG, channel A only)
| Driver pin | Blue Pill | Notes |
|---|---|---|
| PWMA | **PA8** | Motor C speed (PWM, timer 1) |
| AIN1 | **PB12** | Motor C direction |
| AIN2 | **PB13** | Motor C direction |
| STBY | **PB5** | Shared with driver #1 |
| VCC | 3.3 | Logic |
| VM | Battery + | |
| GND | G **and** battery − | |
| A01 / A02 | Motor C | |
| PWMB, BIN1, BIN2, B01, B02 | Not connected | Channel B unused |

## Sensors
| Part | Blue Pill | Wiring |
|---|---|---|
| IR sensor TCRT5000 (VCC, GND, OUT) | **PA5** | VCC → 3.3, GND → G, OUT → PA5. OUT reads HIGH when the door is close |
| Photoresistor 1 | **PA4** | 3.3 → LDR → PA4 → 8 kΩ → G |
| Photoresistor 2 | **PA2** | 3.3 → LDR → PA2 → 8 kΩ → G |
| Photoresistor 3 | **PB0** | 3.3 → LDR → PB0 → 8 kΩ → G |

Each photoresistor needs its own 8 kΩ resistor (4.7–22 kΩ also works). Brighter = higher reading (0–4095).

## Servos
| Servo wire | Servo 1 | Servo 2 | Notes |
|---|---|---|---|
| Orange / yellow (signal) | **PA6** | **PA7** | Both on timer 3 |
| Red (+) | Battery + (or 5V) | Battery + (or 5V) | **Not** 3.3. With two servos, battery is safer than the Blue Pill 5V pin |
| Brown / black (−) | G | G | |

## Power
| From | To |
|---|---|
| Battery + (4× AA) | VM on both drivers, red wire of both servos |
| Battery − | G (common ground) |
| Blue Pill 3.3 | VCC on both drivers, IR VCC, photoresistors |
| Blue Pill | USB or ST-Link only. **Never** connect the battery to the Blue Pill directly |

All grounds must be connected together.

## Free pins
| Pin | Can do |
|---|---|
| PA3 | Digital, analog (ADC), PWM (timer 2) |
| PB1 | Digital, analog (ADC). No PWM (timer 3 = servos) |
| PB8, PB9 | Digital, PWM (timer 4) |
| PB10, PB11 | Digital |
| PA15, PB3, PB4 | Digital, only after disabling JTAG in code (see notes) |

## Do not use
| Pin | Why |
|---|---|
| PA9, PA10 | Serial upload / USART1 |
| PA11, PA12 | USB data lines |
| PA13, PA14 | ST-Link (SWD) |
| PB2 | BOOT1 |
| PC13, PC14, PC15 | Onboard LED and clock crystal; can only supply very little current |

## Rules and notes
- **Timer 3 (PA6, PA7, PB0, PB1) belongs to the servos.** The servo library takes over that whole timer, so motor PWM must not use it. PB0 (and free PB1) are still fine for analog reads.
- **Analog pins max 3.3 V.** The ADC pins are not 5 V tolerant.
- **PB3, PB4 and PA15 are JTAG pins.** With the Serial or ST-Link upload method the Roger Clark core leaves JTAG on, so the code must call `afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);` at the start of `setup()` (HardwareTest does). That frees these pins and keeps SWD for ST-Link uploads.
- **Keep pins in sync.** Pins are defined in `config.h` (main sketch) **and** at the top of `HardwareTest/HardwareTest.ino`. Change both.
