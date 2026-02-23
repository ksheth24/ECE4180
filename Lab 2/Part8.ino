#include <Wire.h>
#include <Adafruit_MPR121.h>

#define INTR 20

Adafruit_MPR121 cap = Adafruit_MPR121();
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif


void setup() {
  Serial.begin(115200);

  Wire.begin(23, 22);
  Wire.setClock(400000);

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found");
    while (1);
  }

  cap.setAutoconfig(true);

  Serial.println("MPR121 ready");
  pinMode(INTR, OUTPUT);
}

void loop() {
  digitalWrite(INTR, HIGH);
  int touched = cap.touched();
  for (int i = 0; i < 12; i++) {
    if (touched & _BV(i)) {
      Serial.print(" Key ");
      Serial.print(i);
      Serial.println(" touched");
    }
  }
  delay(100);
}
