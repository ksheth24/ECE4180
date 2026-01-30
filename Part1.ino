#define BUTTON 5
#define LED 18

int count = 0;
  void setup() {
    // put your setup code here, to run once:
    pinMode(BUTTON, INPUT_PULLDOWN);
    pinMode(LED, OUTPUT);
  }


  void loop() {
      if (digitalRead(BUTTON)) {
        digitalWrite(LED, HIGH);
      } else {
        digitalWrite(LED, LOW);
      }
  }
