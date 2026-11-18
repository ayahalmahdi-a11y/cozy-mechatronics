# cozy-mechatronics
# Smart Home Demo – MCT 331s

Smart home prototype for the Mechatronic System Integration course
(Faculty of Engineering, Ain Shams University, Fall 2026).
It integrates mechanical, electrical, and computer systems on an Arduino Uno. It handles door access with a keypad, window and motion security, automatic lighting, and temperature-based fan control.

## Features
- **Door access:** enter the code on a 4×4 keypad (`#` = confirm, `*` = clear). A correct code opens the door (servo) for 5 seconds. Default code: `1234`.
- **Wrong code protection:** a wrong code sounds the buzzer for 1 second. After 3 wrong attempts the keypad locks for 10 seconds.
- **Window alarm (latched):** when the security switch is ON and the window opens, the buzzer sounds and **stays on, even if the window is closed again, until the correct code is entered**. The LCD shows `ALARM:ENTER CODE`. If the window is still open after the code, the alarm does not restart until the window is closed and opened again.
- **Motion alarm:** with security ON, motion detected by the PIR sounds the buzzer while motion lasts (no code needed).
- **Outside light:** the outside LED turns on when the PIR detects motion.
- **Inside light:** automatic using the LDR (ON when the reading is `<= 12`, OFF when `>= 16`). The override button cycles **Automatic → Manual ON → Manual OFF**.
- **Fan:** the DHT11 measures temperature and humidity. The fan turns on when the temperature reaches the limit set by the potentiometer (20–40 °C) and turns off 1 °C below it.
- **LCD 16×2 (I2C):** line 1 shows status (security, alarm, door, code entry). Line 2 shows temperature, humidity, fan limit and fan state.

## Hardware
| Part | Notes |
|---|---|
| Arduino Uno | Main controller |
| LCD 16×2 I2C | Address `0x27` (or `0x3F`) |
| PCF8574 + 4×4 keypad | Address `0x20` (A0/A1/A2 to GND) |
| PIR HC-SR501 | Motion |
| Reed switch + magnet | Window |
| LDR + 10 kΩ | Ambient light |
| DHT11 | Temperature + humidity (replaces the LM35) |
| Potentiometer 10 kΩ | Fan temperature limit |
| Servo SG90 | Door |
| DC fan 5 V + 2N2222 + 1 kΩ + 1N4007 | Cooling |
| Active buzzer | Alarm |
| 2 × LED + 2 × 220 Ω | Outside and inside light |
| Push button + toggle switch | Light override and security mode |
| External 5 V / 2 A supply | Servo and fan only |

## Pin map (Uno)
| Pin | Function | Notes |
|---|---|---|
| D2 | PIR output | HIGH = motion |
| D3 | Reed switch | `INPUT_PULLUP`, closed = LOW, open = HIGH |
| D4 | Override button | `INPUT_PULLUP`, other leg to GND |
| D5 | Buzzer (+) | Active buzzer |
| D6 | Outside LED | Through 220 Ω |
| D7 | Inside LED | Through 220 Ω |
| D8 | Fan | Through 1 kΩ to 2N2222 base |
| D9 | Servo signal | Orange wire |
| D10 | Security switch | `INPUT_PULLUP`, LOW = security ON |
| A0 | LDR | LDR between 5V and A0, 10 kΩ from A0 to GND |
| A1 | DHT11 data | Change `DHT_PIN` to `11` if wired on D11 |
| A2 | Potentiometer | Middle pin |
| A4 / A5 | SDA / SCL | LCD and PCF8574 |

## Wiring notes

- **Power:** the servo and the fan use a separate 5 V / 2 A supply. Its GND must be **shared** with the Uno GND. Never connect the external +5 V to the Uno 5V pin.
- **Fan driver:** external +5 V → fan (+), fan (−) → 2N2222 collector, emitter → GND, D8 → 1 kΩ → base. Place the 1N4007 diode across the fan, cathode on +5 V.
- **Keypad:** the 8 keypad wires go to PCF8574 pins P0–P7 in order. If the keys come out wrong, change the order of `keymap` in the code or the order of the wires.
- **Servo:** add a 470 µF capacitor between +5 V and GND near the servo.

## Repository structure
- `code/` Arduino sketch (`smart_home.ino`)
- `cad/` DXF files for the laser-cut MDF cottage (3 mm)
- `docs/` report represent wiring and mounting plan, BOM, flow charts
- `media/` previews and demo video

## Libraries
- `LiquidCrystal I2C`
- `DHT sensor library` (Adafruit) and `Adafruit Unified Sensor`
- `I2CKeyPad` (Rob Tillaart)
- `Servo` (built in)

> The library name is case-sensitive: `#include <I2CKeyPad.h>` and the class `I2CKeyPad`.

## Model
Laser-cut MDF (3 mm) cottage with a tiled gable roof and a basement for the electronics.

## Getting started

1. Wire the circuit and test each part with the sketches in `1_component_tests.txt`.
2. Open `smart_home.ino` in the Arduino IDE.
3. Check the I2C addresses with the I2C scanner test (LCD `0x27`/`0x3F`, PCF8574 `0x20`).
4. Upload and open Serial Monitor at **9600** for diagnostics (security, PIR, window, LDR, temperature, humidity, fan).

## Settings you can change (top of `smart_home.ino`)

| Setting | Default | Meaning |
|---|---|---|
| `SECRET` | `"1234"` | Door and alarm code |
| `DARK_ON_LEVEL` | `12` | LDR reading at or below this turns the inside light on |
| `DARK_OFF_LEVEL` | `16` | LDR reading at or above this turns it off |
| `LDR_INVERTED` | `false` | Set `true` if the reading goes up in the dark |
| `DOOR_OPEN_TIME` | `5000` ms | How long the door stays open |
| `MAX_WRONG_ATTEMPTS` | `3` | Wrong codes before lockout |
| `LOCKOUT_TIME` | `10000` ms | Keypad lockout duration |
| `DHT_PIN` | `A1` | Use `11` if wired on D11 |

## Troubleshooting

- **Keypad not working:** run the I2C scanner. PCF8574 should appear at `0x20` (PCF8574A starts at `0x38`).
- **Inside light not turning on:** open Serial Monitor, read the `LDR:` value in light and in darkness, and set `DARK_ON_LEVEL` and `DARK_OFF_LEVEL` around those values.
- **PIR always reads motion:** turn the Time Delay and Sensitivity knobs to minimum, set the jumper to H, and wait 30–60 seconds after power-up.
- **Upload fails (`not in sync`):** disconnect anything on D0/D1, check the board and port in the IDE, and try another USB cable.

## Changelog

- DHT11 replaces the LM35 for temperature, and also adds humidity.
- Keypad changed from 3×4 to 4×4 using the `I2CKeyPad` library.
- Window alarm now latches and needs the correct code to stop.
- Inside light uses low LDR thresholds with hysteresis.
- Fan switches on a temperature limit set by the potentiometer.

## Team
- Ayah Alagab
- Omama Mohammed
- Alya Khalid
- Abubakr Mokhtar

## License
MIT (code) / CC BY-NC (design files)
