#include <ESP32Servo.h>

#define TRIG 8
#define ECHO 7
NEW SKETCH

#define SERVO_PIN 21
#define BUZZER 5

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
      ledcWrite(BUZZER, 128);
      vTaskDelay(pdMS_TO_TICKS(100));
      ledcWrite(BUZZER, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
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

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  ledcAttachChannel(BUZZER, 2000, 12, 2);

  xTaskCreatePinnedToCore(servoTask,  "ServoTask",  2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 2048, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}
