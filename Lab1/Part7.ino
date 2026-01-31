#define LED_PIN 4
#define LED_CH  0

void setup() {
  ledcAttach(LED_PIN, 2000, 12);
}

void loop() {
  for (int i = 0; i < 1000; i += 10) {
    ledcWrite(LED_CH, i);
    delay(500);
  }
}
