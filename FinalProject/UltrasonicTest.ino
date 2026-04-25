#include <ESP32Servo.h>

#define TRIG 8
#define ECHO 7

#define SERVO_PIN 21
#define BUZZER 5
#define TX 43
#define RX 44

#define IR 37
Servo myServo;

volatile bool detected = false;

void servoTask(void *pvParameters) {
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  int servoAngle = 0;
  bool goLeft = true;

  for (;;) {
    if (goLeft) {
      servoAngle++;
      if (servoAngle >= 180) goLeft = false;
    } else {
      servoAngle--;
      if (servoAngle <= 0) goLeft = true;
    }

    myServo.write(servoAngle);
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

void sensorTask(void *pvParameters) {
  for (;;) {
    long distance = getDistance();

    Serial.print("Distance: ");
    Serial.println(distance);

    detected = (distance != -1 && distance <= 8);

    if (detected) {
      // Send "active" and wait for response
      Serial1.println("active");
      Serial.println("Sent: active");

      String response = waitForResponse(2000); // 2 second timeout
      Serial.print("Received: ");
      Serial.println(response);

      if (response == "circle") {
        // Trigger buzzer and laser only for circle
        ledcWrite(BUZZER, 128);
        digitalWrite(IR, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        ledcWrite(BUZZER, 0);
        digitalWrite(IR, LOW);
      }
      // If "triangle" or anything else, do nothing

    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Waits up to timeoutMs for a response on Serial1
String waitForResponse(unsigned long timeoutMs) {
  String result = "";
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    if (Serial1.available()) {
      result = Serial1.readStringUntil('\n');
      result.trim(); // Remove \r\n whitespace
      if (result.length() > 0) return result;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  return ""; // Timeout, empty response
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
  Serial1.begin(115200, SERIAL_8N1, RX, TX); // RX=44, TX=43

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(IR, OUTPUT);

  ledcAttachChannel(BUZZER, 2000, 12, 2);

  xTaskCreatePinnedToCore(servoTask,  "ServoTask",  2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 2048, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}
