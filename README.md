# cozy-mechatronics
# Smart Home Demo – MCT 331s

Smart home prototype for the Mechatronic System Integration course
(Faculty of Engineering, Ain Shams University, Fall 2026).
It integrates mechanical, electrical, and computer systems on an Arduino Uno.

## Features
- **Main door access:** a 4x3 keypad (through a PCF8574 I2C expander) opens the door with a servo, then it closes automatically after a delay.
- **Security mode:** with the switch on, an alarm sounds if the window is opened, and the exterior light turns on when the PIR detects motion near the door.
- **Automatic indoor lighting:** an LDR turns the light on in the dark, and a button overrides it manually.
- **Climate control:** an LM35 reads the temperature, and the fan turns on above a threshold set by a potentiometer.
- **LCD feedback:** a 16x2 I2C LCD shows the door status, security mode, and temperature.

## Hardware
| Part | Role |
|---|---|
| Arduino Uno | Controller |
| PCF8574 + 4x3 keypad | Door code input |
| SG90 servo | Door |
| PIR HC-SR501, reed switch | Motion and window sensors |
| LDR, LM35, 10k pot | Light, temperature, threshold knob |
| 5V fan + 2N2222 + 1N4007 | Cooling |
| Buzzer, 2 LEDs, 16x2 I2C LCD | Outputs |

## Pin map (Uno)
D2 PIR, D3 reed (INPUT_PULLUP), D4 light override button, D5 buzzer, D6 exterior LED, D7 interior LED, D8 fan transistor, D9 servo, D10 security switch, A0 LDR, A1 LM35, A2 potentiometer, A4/A5 I2C (LCD 0x27, PCF8574 0x20).

## Repository structure
- `code/` Arduino sketch (`smart_home.ino`)
- `cad/` DXF files for the laser-cut MDF cottage (3 mm)
- `docs/` wiring and mounting plan, BOM, flow charts
- `media/` previews and demo video

## Libraries
LiquidCrystal I2C, Keypad, Keypad_I2C (install from the Arduino Library Manager).

## Model
Laser-cut MDF (3 mm) cottage with a tiled gable roof and a basement for the electronics.
The DXF files have not been validated on a laser cutter yet, so cut a test joint first.

## Team
- Ayah Alagab
- Omama Mohammed
- Alya Khalid
- Abubakr Mokhtar

## License
MIT (code) / CC BY-NC (design files)
