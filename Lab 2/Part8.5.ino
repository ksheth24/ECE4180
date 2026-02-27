#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>

Adafruit_MPR121 cap = Adafruit_MPR121();
HardwareSerial SenderSerial(1);

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#define INTR 20


String s = "";

void setup() {
  // put your setup code here, to run once:
  SenderSerial.begin(9600, SERIAL_8N1, 16, 17);
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
  // put your main code here, to run repeatedly:
  digitalWrite(INTR, HIGH);
  int touched = cap.touched();
  for (int i = 0; i < 12; i++) {
    if (touched & _BV(i)) {
      if (i == 10) {
        SenderSerial.println(s);
      }
      if (i == 11) {
        s = s.substring(0, s.length() - 1);
      } else {
        s += i;
      } 
      Serial.print(" Key ");
      Serial.print(i);
      Serial.println(" touched");
      Serial.println(s);
    }
  }

}
