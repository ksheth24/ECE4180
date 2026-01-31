#include <Adafruit_NeoPixel.h>
#define BUTTON 5
#define BUTTON2 19

Adafruit_NeoPixel pixel(1, 8, NEO_GRB + NEO_KHZ800);

int buttonState1;
int lastButtonState1 = LOW;
int buttonState2;
int lastButtonState2 = LOW;


unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 5;

int count = 0;
  void setup() {
    // put your setup code here, to run once:
    pinMode(BUTTON, INPUT_PULLDOWN);
    pinMode(BUTTON2, INPUT_PULLDOWN);
    pixel.begin();
    pixel.setBrightness(20);
    Serial.begin(115200);
  }

  void loop() {
    int reading1 = digitalRead(BUTTON);
    int reading2 = digitalRead(BUTTON2);

    if (reading1 != lastButtonState1) {
      lastDebounceTime1 = millis();
    }

    if (reading2 != lastButtonState2) {
      lastDebounceTime2 = millis();
    } 

    if ((millis() - lastDebounceTime1) > debounceDelay) {
      if (reading1 != buttonState1) {
        buttonState1 = reading1;
        if (buttonState1 == HIGH) {
          count++;
        }
      }
    }

    if ((millis() - lastDebounceTime2) > debounceDelay) {
      if (reading2 != buttonState2) {
        buttonState2 = reading2;
        if (buttonState2 == HIGH) {
          count += 3;
        }
      }
    }

    if (count % 4 == 0) {
      pixel.setPixelColor(0, pixel.Color(255,0,0));
      pixel.show();
    }
    if (count % 4 == 1) {
      pixel.setPixelColor(0, pixel.Color(0,255,0));
      pixel.show();
    }
    if (count % 4 == 2) {
      pixel.setPixelColor(0, pixel.Color(0,0,255));
      pixel.show();
    }
    if (count % 4 == 3) {
      pixel.setPixelColor(0, pixel.Color(255,255,0));
      pixel.show();
    }

    lastButtonState1 = reading1;
    lastButtonState2 = reading2;
  }
