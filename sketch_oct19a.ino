const int PIR_PIN    = 2;
const int REED_PIN   = 3;
const int LED_PIR    = 4;   // غيّره حسب توصيلك
const int BUZZER_PIN = 5;

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED_PIR, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("PIR bysakhan... esta nna 30 sanya");
  delay(30000);                     // وقت تسخين الـ PIR
  Serial.println("Gahez!");
}

void loop() {
  bool motion     = digitalRead(PIR_PIN) == HIGH;
  bool windowOpen = digitalRead(REED_PIN) == HIGH;

  // الـ LED بيتحكم فيه الـ PIR
  digitalWrite(LED_PIR, motion ? HIGH : LOW);

  // البازر بيرن لما الشباك يتفتح
  digitalWrite(BUZZER_PIN, windowOpen ? HIGH : LOW);

  Serial.print("Motion: ");
  Serial.print(motion ? "YES" : "NO");
  Serial.print(" | Window: ");
  Serial.println(windowOpen ? "OPEN" : "CLOSED");

  delay(50);
}