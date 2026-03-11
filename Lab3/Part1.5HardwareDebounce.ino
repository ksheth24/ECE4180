#define BUTTON_PIN 6
#define LED_PIN 5

void IRAM_ATTR buttonISR() {
    digitalWrite(LED_PIN, HIGH);   // LED turns on when interrupt occurs
    Serial.println("yo");
}

void setup() {
      Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);    // external hardware debounce

    attachInterrupt(BUTTON_PIN, buttonISR, RISING);
}

void loop() {
  digitalWrite(LED_PIN, LOW);
}
