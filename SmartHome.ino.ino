#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Servo.h>
#include <I2CKeyPad.h>

// ==================== PIN CONFIGURATION ====================
#define PIR_PIN       2
#define REED_PIN      3
#define OVERRIDE_PIN  4
#define BUZZER_PIN    5
#define OUTSIDE_LED   6
#define INSIDE_LED    7
#define FAN_PIN        8     // through 1k resistor to 2N2222 base
#define SERVO_PIN      9
#define SECURITY_PIN   10
#define LDR_PIN        A0
#define DHT_PIN        A1   
#define POT_PIN        A2
#define DHT_TYPE       DHT11

// ==================== I2C ADDRESSES ====================
#define LCD_ADDR       0x27  // or 0x3F, check with an I2C scanner
#define KEYPAD_ADDR    0x20  // PCF8574 with A0/A1/A2 to GND

// ==================== OBJECTS ====================
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);
Servo doorServo;

// ==================== KEYPAD 4x4 (PCF8574, keypad pins 1-8 -> P0-P7) ====================
I2CKeyPad keypad(KEYPAD_ADDR);
// 16 keys + N (no key) + F (fail). Fix the order here if keys come out wrong.
char keymap[19] = "123A456B789C*0#DNF";
char lastKey = 0;

// ==================== SENSORS ====================
DHT dht(DHT_PIN, DHT_TYPE);

// ==================== SETTINGS ====================
const char SECRET[] = "1234";            // door code
const byte MAX_CODE_LEN = 8;

const int DARK_ON_LEVEL = 25;            // LDR reading <= this -> inside light ON
const int DARK_OFF_LEVEL = 35;           // LDR reading >= this -> inside light OFF
const bool LDR_INVERTED = false;         // set true if the reading goes UP in the dark

const int TEMP_LIMIT_MIN = 20;           // potentiometer range
const int TEMP_LIMIT_MAX = 40;
const float FAN_HYSTERESIS = 1.0;        // fan turns off 1 C below the limit

const int DOOR_CLOSED_ANGLE = 0;
const int DOOR_OPEN_ANGLE = 90;
const unsigned long DOOR_OPEN_TIME = 5000;      // door stays open 5 s

const byte MAX_WRONG_ATTEMPTS = 3;
const unsigned long WRONG_BEEP_TIME = 1000;     // buzzer after a wrong code
const unsigned long LOCKOUT_TIME = 10000;       // lockout after 3 wrong codes
const unsigned long CODE_TIMEOUT = 8000;        // clears a half-typed code

const unsigned long DHT_INTERVAL = 2100;
const unsigned long LCD_INTERVAL = 300;
const unsigned long DEBOUNCE_DELAY = 50;

// ==================== SYSTEM VARIABLES ====================
int lightValue = 0;

float temperatureC = NAN;
float humidity = NAN;
int tempLimit = 30;

bool motionDetected = false;
bool windowOpen = false;
bool securityActive = false;
bool alarmOn = false;

bool windowAlarm = false;   // latched: stays on until the correct code is entered
bool windowAck = false;     // code entered while window still open
bool roomDark = false;
bool fanOn = false;

bool insideLightOn = false;
bool outsideLightOn = false;

// Lighting modes:
// 0 = Automatic
// 1 = Manual ON
// 2 = Manual OFF
byte lightingMode = 0;

// Button states
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long lastDHTRead = 0;
unsigned long lastLCDUpdate = 0;

// Door and code
bool doorOpen = false;
unsigned long doorOpenedAt = 0;
char codeBuffer[MAX_CODE_LEN + 1] = "";
byte codeLen = 0;
unsigned long lastKeyTime = 0;
byte wrongAttempts = 0;
bool wrongBeepActive = false;
unsigned long wrongBeepStart = 0;
bool lockoutActive = false;
unsigned long lockoutStart = 0;

// Temporary LCD message
char messageText[17] = "";
unsigned long messageStart = 0;
unsigned long messageDuration = 0;

// Button debounce
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;

unsigned long lastDHTRead = 0;
unsigned long lastLCDUpdate = 0;

// ==================== LCD HELPERS ====================
void printLCDLine(byte row, const char* message) {
  lcd.setCursor(0, row);
  byte i = 0;
  while (message[i] != '\0' && i < 16) {
    lcd.print(message[i]);
    i++;
  }
  while (i < 16) {
    lcd.print(' ');
    i++;
  }
}

void showMessage(const char* text, unsigned long duration) {
  strncpy(messageText, text, 16);
  messageText[16] = '\0';
  messageStart = millis();
  messageDuration = duration;
}

bool messageActive() {
  return messageDuration > 0 && (millis() - messageStart < messageDuration);
}

// ==================== READ DHT11 ====================
void updateDHT() {
  unsigned long currentTime = millis();

  if (now - lastDHTRead >= DHT_INTERVAL) return;
    lastDHTRead = now;

    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    if (!isnan(newHumidity) && !isnan(newTemperature)) {
      humidity = newHumidity;
      temperatureC = newTemperature;
    } else {
      Serial.println("Error reading DHT11!");
    }
  }

// ==================== FAN (POTENTIOMETER LIMIT) ====================
void updateFan() {
  tempLimit = map(analogRead(POT_PIN), 0, 1023, TEMP_LIMIT_MIN, TEMP_LIMIT_MAX);

  if (!isnan(temperatureC)) {
    if (temperatureC >= tempLimit) {
      fanOn = true;
    } else if (temperatureC <= tempLimit - FAN_HYSTERESIS) {
      fanOn = false;
    }
  }

  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);
}

// ==================== LIGHTING CONTROL ====================
void updateLighting() {
  lightValue = analogRead(LDR_PIN);
  int v = LDR_INVERTED ? (1023 - lightValue) : lightValue;

  if (v <= DARK_ON_LEVEL) roomDark = true;
  else if (v >= DARK_OFF_LEVEL) roomDark = false;

  switch (lightingMode) {
    case 0: insideLightOn = roomDark; break;
    case 1: insideLightOn = true;     break;
    case 2: insideLightOn = false;    break;
  }

  digitalWrite(INSIDE_LED, insideLightOn ? HIGH : LOW);
}

// ==================== OVERRIDE BUTTON ====================
void updateOverrideButton() {
  bool reading = digitalRead(OVERRIDE_PIN);
  unsigned long now = millis();

  if (reading != lastButtonReading) {
    lastDebounceTime = now;
  }

  if (now - lastDebounceTime >= DEBOUNCE_DELAY) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {   // INPUT_PULLUP: pressed = LOW
        lightingMode++;
        if (lightingMode > 2) lightingMode = 0;

        Serial.print("Lighting mode: ");
        if (lightingMode == 0) Serial.println("AUTOMATIC");
        else if (lightingMode == 1) Serial.println("MANUAL ON");
        else Serial.println("MANUAL OFF");
      }
    }
  }

  lastButtonReading = reading;
}

// ==================== SECURITY SYSTEM ====================
void updateSecurity() {
  motionDetected = (digitalRead(PIR_PIN) == HIGH);     // HIGH = motion
  windowOpen = (digitalRead(REED_PIN) == HIGH);        // HIGH = open
  securityActive = (digitalRead(SECURITY_PIN) == LOW); // LOW = security on

  // Window alarm latches: only the correct code stops it
  if (securityActive && windowOpen && !windowAck) windowAlarm = true;
  if (!windowOpen) windowAck = false;   // re-arm once the window is closed

  alarmOn = windowAlarm || (securityActive && motionDetected);

  // Outside LED follows the PIR (original behaviour)
  outsideLightOn = motionDetected;
  digitalWrite(OUTSIDE_LED, outsideLightOn ? HIGH : LOW);

  // Buzzer: security alarm, wrong code beep, or lockout
  bool buzzerOn = alarmOn || wrongBeepActive || lockoutActive;
  digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
}

// ==================== DOOR ====================
void openDoor() {
  doorServo.write(DOOR_OPEN_ANGLE);
  doorOpen = true;
  doorOpenedAt = millis();
  Serial.println("Door OPEN");
}

void closeDoor() {
  doorServo.write(DOOR_CLOSED_ANGLE);
  doorOpen = false;
  Serial.println("Door CLOSED");
}

void clearCode() {
  codeLen = 0;
  codeBuffer[0] = '\0';
}

void checkCode() {
  if (strcmp(codeBuffer, SECRET) == 0) {
    wrongAttempts = 0;
    if (windowAlarm) {
      windowAlarm = false;          // correct code silences the alarm
      windowAck = true;
      showMessage("ALARM OFF", 1500);
    } else {
      openDoor();
      showMessage("CODE OK", 1500);
    }
  } else {
    wrongAttempts++;
    wrongBeepActive = true;
    wrongBeepStart = millis();
    showMessage("WRONG CODE", 1500);
    Serial.println("Wrong code");

    if (wrongAttempts >= MAX_WRONG_ATTEMPTS) {
      lockoutActive = true;
      lockoutStart = millis();
      Serial.println("Keypad LOCKED");
    }
  }
  clearCode();
}

void updateKeypad() {
  char key = 0;
  if (keypad.isPressed()) {
    char c = keypad.getChar();
    if (c != 'N' && c != 'F' && c != lastKey) key = c;
    lastKey = c;
  } else {
    lastKey = 0;
  }

  if (!key) {
    if (codeLen > 0 && millis() - lastKeyTime >= CODE_TIMEOUT) {
      clearCode();
    }
    return;
  }

  lastKeyTime = millis();
  if (lockoutActive) return;   // ignore keys during lockout

  if (key == '*') {
    clearCode();
    showMessage("CODE CLEARED", 1000);
  } else if (key == '#') {
    checkCode();
  } else if (key >= '0' && key <= '9' && codeLen < MAX_CODE_LEN) {
    codeBuffer[codeLen++] = key;
    codeBuffer[codeLen] = '\0';
  }
}

void updateDoorTimers() {
  unsigned long now = millis();

  if (doorOpen && now - doorOpenedAt >= DOOR_OPEN_TIME) {
    closeDoor();
  }

  if (wrongBeepActive && now - wrongBeepStart >= WRONG_BEEP_TIME) {
    wrongBeepActive = false;
  }

  if (lockoutActive && now - lockoutStart >= LOCKOUT_TIME) {
    lockoutActive = false;
    wrongAttempts = 0;
  }
}

// ==================== LCD DISPLAY ====================
void updateLCD() {
  unsigned long now = millis();
  if (now - lastLCDUpdate < LCD_INTERVAL) return;
  lastLCDUpdate = now;

  char line[17];

  // Line 1: status, in priority order
  if (codeLen > 0) {
    byte i = 0;
    const char* prefix = "Code: ";
    while (prefix[i] != '\0') { line[i] = prefix[i]; i++; }
    for (byte k = 0; k < codeLen && i < 16; k++) line[i++] = '*';
    line[i] = '\0';
    printLCDLine(0, line);
  } else if (messageActive()) {
    printLCDLine(0, messageText);
  } else if (lockoutActive) {
    printLCDLine(0, "KEYPAD LOCKED");
  } else if (doorOpen) {
    printLCDLine(0, "DOOR OPEN");
  } else if (alarmOn) {
    printLCDLine(0, windowAlarm ? "ALARM:ENTER CODE" : "ALARM: MOTION!");
  } else if (motionDetected) {
    printLCDLine(0, "MOTION DETECTED");
  } else if (windowOpen) {
    printLCDLine(0, "WINDOW: OPEN");
  } else if (securityActive) {
    printLCDLine(0, "SECURITY: ON");
  } else {
    printLCDLine(0, "SECURITY: OFF");
  }

  // Line 2: T = temperature, H = humidity, L = fan limit
  if (!isnan(temperatureC) && !isnan(humidity)) {
    snprintf(line, sizeof(line), "T%2d H%2d L%2d %s",
             (int)(temperatureC + 0.5),
             (int)(humidity + 0.5),
             tempLimit,
             fanOn ? "FAN" : "   ");
    printLCDLine(1, line);
  } else {
    printLCDLine(1, "Reading DHT11...");
  }
}

// ==================== SERIAL DEBUG ====================
void printDebug() {
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug < 1000) return;
  lastDebug = millis();

  Serial.print("Sec:");   Serial.print(securityActive ? "ON" : "OFF");
  Serial.print(" PIR:");  Serial.print(motionDetected ? "MOTION" : "NONE");
  Serial.print(" Win:");  Serial.print(windowOpen ? "OPEN" : "CLOSED");
  Serial.print(" Alarm:"); Serial.print(alarmOn ? "ON" : "OFF");
  Serial.print(" LDR:");  Serial.print(lightValue);
  Serial.print(" T:");    Serial.print(temperatureC);
  Serial.print(" H:");    Serial.print(humidity);
  Serial.print(" Limit:"); Serial.print(tempLimit);
  Serial.print(" Fan:");  Serial.println(fanOn ? "ON" : "OFF");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);

  // Inputs
  pinMode(PIR_PIN, INPUT);
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);
  pinMode(SECURITY_PIN, INPUT_PULLUP);

  // Outputs
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(OUTSIDE_LED, OUTPUT);
  pinMode(INSIDE_LED, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);


  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(OUTSIDE_LED, LOW);
  digitalWrite(INSIDE_LED, LOW);
  digitalWrite(FAN_PIN, LOW);

  doorServo.attach(SERVO_PIN);
  doorServo.write(DOOR_CLOSED_ANGLE);

  // I2C LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  keypad.begin();
  keypad.loadKeyMap(keymap);

  // DHT11
  dht.begin();

  printLCDLine(0, "SMART HOME");
  printLCDLine(1, "Starting...");
  delay(2000);

  Serial.println("======================");
  Serial.println("SMART HOME SYSTEM");
  Serial.println("System initialized!");
  Serial.println("======================");

  Serial.println("Lighting: AUTOMATIC");
  Serial.println("Security: Check switch");
  Serial.println("DHT11: D11");
  Serial.println("Fan: Reserved D8");
  Serial.println("Servo: Reserved D9");

  // Allow the first DHT11 reading after startup
  lastDHTRead = millis() - DHT_INTERVAL;
}

// ==================== MAIN LOOP ====================
void loop() {
  updateDHT();
  updateFan();
  updateSecurity();
  updateOverrideButton();
  updateLighting();
  updateKeypad();
  updateDoorTimers();
  updateLCD();
  printDebug();
}
