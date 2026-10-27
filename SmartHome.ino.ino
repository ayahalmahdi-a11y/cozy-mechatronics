#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ==================== LCD ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==================== PIN CONFIGURATION ====================
#define PIR_PIN       2
#define REED_PIN      3
#define OVERRIDE_PIN  4
#define BUZZER_PIN    5
#define OUTSIDE_LED   6
#define INSIDE_LED    7

// Reserved for future components:
// D8  -> Fan
// D9  -> Servo
// A1  -> LM35
// A2  -> Potentiometer
// Keypad + PCF8574 -> Not installed yet

#define SECURITY_PIN  10
#define DHT_PIN       11
#define DHT_TYPE      DHT11
#define LDR_PIN       A0

// ==================== SENSORS ====================
DHT dht(DHT_PIN, DHT_TYPE);

// ==================== SETTINGS ====================
const int LIGHT_THRESHOLD = 400;

const unsigned long DHT_INTERVAL = 2100;
const unsigned long LCD_INTERVAL = 500;
const unsigned long DEBOUNCE_DELAY = 50;

// ==================== SYSTEM VARIABLES ====================
int lightValue = 0;

float temperatureC = NAN;
float humidity = NAN;

bool motionDetected = false;
bool windowOpen = false;
bool securityActive = false;
bool alarmOn = false;

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

// ==================== LCD LINE FUNCTION ====================
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

// ==================== READ DHT11 ====================
void updateDHT() {
  unsigned long currentTime = millis();

  if (currentTime - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = currentTime;

    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    if (!isnan(newHumidity) && !isnan(newTemperature)) {
      humidity = newHumidity;
      temperatureC = newTemperature;

      Serial.print("Temperature: ");
      Serial.print(temperatureC);
      Serial.print(" C | Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
    } else {
      Serial.println("Error reading DHT11!");
    }
  }
}

// ==================== LIGHTING CONTROL ====================
void updateLighting() {
  lightValue = analogRead(LDR_PIN);

  // Lower readings generally mean darker conditions
  // with the planned LDR voltage divider.
  bool darkRoom = (lightValue < LIGHT_THRESHOLD);

  switch (lightingMode) {
    case 0:  // Automatic mode
      insideLightOn = darkRoom;
      break;

    case 1:  // Manual ON
      insideLightOn = true;
      break;

    case 2:  // Manual OFF
      insideLightOn = false;
      break;
  }

  digitalWrite(INSIDE_LED, insideLightOn ? HIGH : LOW);

  // Outside light follows motion
  outsideLightOn = motionDetected;
  digitalWrite(OUTSIDE_LED, outsideLightOn ? HIGH : LOW);
}

// ==================== LIGHT OVERRIDE BUTTON ====================
void updateOverrideButton() {
  bool buttonReading = digitalRead(OVERRIDE_PIN);
  unsigned long currentTime = millis();

  if (buttonReading != lastButtonReading) {
    lastDebounceTime = currentTime;
  }

  if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY) {
    if (buttonReading != stableButtonState) {
      stableButtonState = buttonReading;

      // INPUT_PULLUP: pressed = LOW
      if (stableButtonState == LOW) {
        lightingMode++;

        if (lightingMode > 2) {
          lightingMode = 0;
        }

        Serial.print("Lighting mode: ");

        if (lightingMode == 0) {
          Serial.println("AUTOMATIC");
        } else if (lightingMode == 1) {
          Serial.println("MANUAL ON");
        } else {
          Serial.println("MANUAL OFF");
        }
      }
    }
  }

  lastButtonReading = buttonReading;
}

// ==================== SECURITY SYSTEM ====================
void updateSecurity() {
  // PIR: HIGH means motion detected
  motionDetected = (digitalRead(PIR_PIN) == HIGH);

  // Reed switch: LOW = closed, HIGH = open
  windowOpen = (digitalRead(REED_PIN) == HIGH);

  // Toggle switch: LOW = security enabled
  securityActive = (digitalRead(SECURITY_PIN) == LOW);

  // Alarm activates only when security is enabled
  // and motion or an open window is detected
  alarmOn = securityActive && (motionDetected || windowOpen);

  // Control buzzer
  digitalWrite(BUZZER_PIN, alarmOn ? HIGH : LOW);

  // Outside LED follows motion
  outsideLightOn = motionDetected;
  digitalWrite(OUTSIDE_LED, outsideLightOn ? HIGH : LOW);

  // Serial Monitor diagnostics
  static unsigned long lastDebug = 0;

  if (millis() - lastDebug >= 500) {
    lastDebug = millis();

    Serial.print("Security: ");
    Serial.print(securityActive ? "ON" : "OFF");

    Serial.print(" | PIR: ");
    Serial.print(motionDetected ? "MOTION" : "NO MOTION");

    Serial.print(" | Window: ");
    Serial.print(windowOpen ? "OPEN" : "CLOSED");

    Serial.print(" | Alarm: ");
    Serial.println(alarmOn ? "ON" : "OFF");
  }
}

// ==================== LCD DISPLAY ====================
void updateLCD() {
  unsigned long currentTime = millis();

  if (currentTime - lastLCDUpdate < LCD_INTERVAL) {
    return;
  }

  lastLCDUpdate = currentTime;

  // First line: security and alarm status
  if (alarmOn) {
    if (windowOpen) {
      printLCDLine(0, "ALARM: WINDOW!");
    } else {
      printLCDLine(0, "ALARM: MOTION!");
    }
  } else if (motionDetected) {
    printLCDLine(0, "MOTION DETECTED");
  } else if (windowOpen) {
    printLCDLine(0, "WINDOW: OPEN");
  } else if (securityActive) {
    printLCDLine(0, "SECURITY: ON");
  } else {
    printLCDLine(0, "SECURITY: OFF");
  }

  // Second line: temperature and humidity
  if (!isnan(temperatureC) && !isnan(humidity)) {
    char displayLine[17];

    snprintf(
      displayLine,
      sizeof(displayLine),
      "T:%2dC H:%2d%%",
      (int)(temperatureC + 0.5),
      (int)(humidity + 0.5)
    );

    printLCDLine(1, displayLine);
  } else {
    printLCDLine(1, "Reading DHT11...");
  }
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

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(OUTSIDE_LED, LOW);
  digitalWrite(INSIDE_LED, LOW);

  // I2C LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();

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
  updateSecurity();
  updateOverrideButton();
  updateLighting();
  updateLCD();
}