#include <Adafruit_NeoPixel.h>

#define POT 3
#define LED_PIN 22
#define AI1 4
#define AI2 5
#define SPEAKER 17
int time_sec = 0;
Adafruit_NeoPixel pixel(1, 8, NEO_GRB + NEO_KHZ800);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ledcAttach(LED_PIN, 2000, 12);
  pinMode(AI1, OUTPUT);
  pinMode(AI2, OUTPUT);
  ledcAttach(SPEAKER, 2000, 12);
  pixel.begin();
  pixel.setBrightness(20);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(AI1, LOW);
  digitalWrite(AI2, LOW);
  bool finished = false;
  ledcWrite(LED_PIN, 2048);
  if (analogRead(POT) > 3200) {
    pixel.setPixelColor(0, pixel.Color(0,0,255));
    pixel.show();
  } else {
    pixel.setPixelColor(0, pixel.Color(255,0,0));
    pixel.show();
  }
  if (time_sec > 0) {
    int potValue = analogRead(POT);  
    while (potValue > 3200 && time_sec > 0) {
      potValue = analogRead(POT);
      digitalWrite(AI1, HIGH);
      digitalWrite(AI2, LOW);
      time_sec--;
      pixel.setPixelColor(0, pixel.Color(255,255,0));
      pixel.show();
      delay(1);
      while (potValue < 100) {
        digitalWrite(AI1, LOW);
        digitalWrite(AI2, LOW);
        pixel.setPixelColor(0, pixel.Color(255,0,0));
        pixel.show();
        delay(1);
      }
    }
    while (analogRead(POT) > 3200) {
      digitalWrite(AI1, LOW);
      digitalWrite(AI2, LOW);
      ledcWrite(SPEAKER, 1024);
      pixel.setPixelColor(0, pixel.Color(0,255,0));
      pixel.show();
      // soeaker code
    }
    ledcWrite(SPEAKER, 0);
  } else if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    if (!s.equals("")) {
      time_sec = s.toInt() * 1000;
      Serial.println(time_sec);
    }
  } 
}
