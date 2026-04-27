#include <ESP32Servo.h>

#define TRIG 8
#define ECHO 7
#define SERVO_PIN 21
#define BUZZER 5
#define TX 43
#define RX 44
#define IR 37

Servo myServo;

volatile bool detected      = false;
volatile bool enemyDetected = false;
volatile bool isTriangle    = false;  // ← new, separates "something seen" from "triangle seen"

long getDistance();
String waitForResponse(unsigned long timeoutMs);

void servoTask(void *pvParameters) {
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  int  servoAngle = 0;
  bool goLeft     = true;

  for (;;) {
    if (!isTriangle) {  // ← only pause on triangle, circle keeps sweeping
      if (goLeft) {
        servoAngle++;
        if (servoAngle >= 180) goLeft = false;
      } else {
        servoAngle--;
        if (servoAngle <= 0) goLeft = true;
      }
      myServo.write(servoAngle);
    }
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

void sensorTask(void *pvParameters) {
  for (;;) {
    long distance = getDistance();

    Serial.print("Distance: ");
    Serial.println(distance);

    detected = (distance > 0 && distance <= 24);

    if (detected) {
      // Don't keep spamming the camera if we already know it's a triangle
      if (isTriangle) {
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }

      Serial1.println("active");
      Serial.println("Sent: active");

      String response = waitForResponse(2000);
      Serial.print("Received: ");
      Serial.println(response);

      isTriangle    = (response == "triangle");
      enemyDetected = (response == "triangle");  // buzzer/IR only on triangle too

    } else {
      // Nothing in range — reset everything
      detected      = false;
      isTriangle    = false;
      enemyDetected = false;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void loudAhhShiz(void *pvParameters) {
  for (;;) {
    if (enemyDetected) {
      ledcWrite(BUZZER, 128);
      digitalWrite(IR, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
      ledcWrite(BUZZER, 0);
      digitalWrite(IR, LOW);
    } else {
      ledcWrite(BUZZER, 0);
      digitalWrite(IR, LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

String waitForResponse(unsigned long timeoutMs) {
  String result = "";
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    if (Serial1.available()) {
      result = Serial1.readStringUntil('\n');
      result.trim();
      if (result.length() > 0) return result;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  return "";
}

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
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX, TX);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(IR, OUTPUT);

  ledcAttachChannel(BUZZER, 2000, 12, 2);

  xTaskCreatePinnedToCore(servoTask,   "ServoTask",   3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(sensorTask,  "SensorTask",  6144, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(loudAhhShiz, "LoudAhhShiz", 3072, NULL, 2, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}
