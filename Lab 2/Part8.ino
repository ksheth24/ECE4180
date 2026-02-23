#include <Wire.h>
#include <ICM_20948.h>
#include <Adafruit_MPR121.h>
#include <Goldelox_Serial_4DLib.h>
#include <Goldelox_Const4D.h>

#define DisplaySerial Serial1



#define INTR 20
#define AD0_VAL 1
ICM_20948_I2C myICM;

Adafruit_MPR121 cap = Adafruit_MPR121();
Goldelox_Serial_4DLib Display(&DisplaySerial);

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

int j = 0;
String s = "";
String sNew = "";
bool correct = false;
String answer = "147";
String ansNew = "LeftRightFor";

void setup() {
  Serial.begin(115200);

  Wire.begin(23, 22);
  Wire.setClock(400000);

  myICM.begin(Wire, AD0_VAL);
  Serial.println("ICM Configed");


  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found");
    while (1);
  }

  cap.setAutoconfig(true);

  Serial.println("MPR121 ready");
  pinMode(INTR, OUTPUT);
  
  DisplaySerial.begin(9600, SERIAL_8N1, RX, TX);
  Display.TimeLimit4D = 500;
  Display.gfx_Cls();
}

void loop() {
  digitalWrite(INTR, HIGH);
  int touched = cap.touched();
  for (int i = 0; i < 12; i++) {
    if (touched & _BV(i)) {
      if (i == 10) {
        if (s.equals(answer) && sNew.equals(ansNew))
      }
      if (i == 11) {
        s = s.substring(0, s.length() - 1);
      } else {
        s += i;
      } 
      Serial.print(" Key ");
      Serial.print(i);
      Serial.println(" touched");
    }
  }
  if (myICM.dataReady()) {
    Serial.println("Data Recieved");
    myICM.getAGMT();
    if (myICM.accX() > 400) {
      sNew += "Left";
    } else if (myICM.accX() < -400) {
      sNew += "Right";
    } else if (myICM.accY() > 400) {
      sNew += "For";
    } else if (myICM.accY() < -400) {
      sNew += "Back";
    } else if (myICM.accZ() - 1000 < -200) {
      sNew += "Down";
    } else if (myICM.accZ() - 1000 > 200) {
      sNew += "Up";
    }
  }
  Display.gfx_Cls();
Display.gfx_MoveTo(10, 50);   // x=10px from left, y=50px from top
Display.print(s);
Display.gfx_MoveTo(10, 70);   // x=10px from left, y=50px from top
Display.print(sNew);
  delay(100);
}
