#include <ESP32Servo.h>

#define TRIG 23
#define ECHO 22
#define SERVO_PIN 18
#define BUZZER 20

Servo myServo;

bool locked = false;
bool goLeft = true;

long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);

  if (duration == 0) return -1;
  return duration / 58;
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(90);

  ledcAttachChannel(BUZZER, 2000, 12, 2);

  Serial.begin(115200);
}

void loop() {
  long distance = getDistance();

  Serial.print("Distance: ");
  Serial.println(distance);

  bool detected = (distance != -1 && distance <= 10);

  if (detected && !locked) {
    locked = true;
    if (goLeft) {
      ledcWrite(BUZZER, 512);
      myServo.write(0);
      delay(100);
      ledcWrite(BUZZER, 0);
    } else {
      ledcWrite(BUZZER, 512);
      myServo.write(180);
      delay(100);
      ledcWrite(BUZZER, 0);
    }

    goLeft = !goLeft; // toggle for next time
  } else if (!detected) {
    locked = false;
  }

  delay(50);
}
