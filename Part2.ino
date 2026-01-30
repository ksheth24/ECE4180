#define BUTTON 5
#define BUTTON2 19

int count = 0;
  void setup() {
    // put your setup code here, to run once:
    pinMode(BUTTON, INPUT_PULLDOWN);
    pinMode(BUTTON2, INPUT_PULLDOWN);
    pinMode(LED, OUTPUT);
  }


  void loop() {
    #ifdef RGB_BUILTIN
      if (digitalRead(BUTTON)) {
        count++;
      }
      if (digitalRead(BUTTON2)) {
        count += 3;
      }
      if (count % 4 == 0) {
        rgbLedWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);
      }
      if (count % 4 == 1) {
        rgbLedWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);
      }
      if (count % 4 == 2) {
        rgbLedWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
      }
      if (count % 4 == 3) {
        digitalWrite(RGB_BUILTIN, RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0);
      }
    #endif
  }
