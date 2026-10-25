// ---------- Pin Definitions ----------
#define IR_PIN      13
#define BUZZER_PIN  4
#define TRIG_PIN    9
#define ECHO_PIN    7
#define RED_LED     2   // غيّرها لو وصلت الـ Red LED على بن تاني

// ---------- Variables ----------
long duration;
float distance;

void setup()
{
  Serial.begin(9600);
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
}

// دالة قياس المسافة بالـ Ultrasonic
float getDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  if (duration == 0) return 999;             // مفيش echo = بعيد جدا
  return duration * 0.034 / 2;               // cm
}

// دالة الـ beep + flash
void beepFlash(int interval)
{
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(interval);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED, LOW);
  delay(interval);
}

void loop()
{
  int ir_status = digitalRead(IR_PIN);
  distance = getDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Priority 1: IR (الأولوية الأعلى)
  if (ir_status == LOW)
  {
    Serial.println("IR: Object <= 10cm");
    digitalWrite(BUZZER_PIN, HIGH);   // صوت متواصل
    digitalWrite(RED_LED, HIGH);      // LED ثابت
  }
  // Priority 2: Ultrasonic < 20 cm
  else if (distance < 20)
  {
    beepFlash(100);   // سريع
  }
  // Priority 3: Ultrasonic 20 - 50 cm
  else if (distance <= 50)
  {
    beepFlash(400);   // بطيء
  }
  // Priority 4: > 50 cm
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_LED, LOW);
  }
}