#define LED_PIN 21

void setup() {
  Serial.begin(115200);
  ledcAttach(LED_PIN, 5000, 8);
}

void loop() {
  int micLevel = abs(analogRead(3) - 1200);
  Serial.println(micLevel);

  int duty = map(micLevel, 0, 255, 0, 255);
  ledcWrite(LED_PIN, duty);
}
