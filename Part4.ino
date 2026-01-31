#include <Wire.h>
#include <SparkFun_I2C_Expander_Arduino_Library.h>

SFE_PCA95XX io; 

int buttonUp = 0;
int buttonDown = 1;
int buttonRight = 2;
int buttonLeft = 3;
int buttonCenter = 4;

void setup() {
  // put your setup code here, to run once:
  Wire.begin(21, 22);
  Serial.begin(115200);
  io.pinMode(buttonUp, INPUT);
  io.pinMode(buttonDown, INPUT);
  io.pinMode(buttonRight, INPUT);
  io.pinMode(buttonLeft, INPUT);
  io.pinMode(buttonCenter, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (io.digitalRead(buttonUp) == LOW) {
    Serial.print("Up");
  }
  if (io.digitalRead(buttonDown) == LOW) {
    Serial.print("Down");
  }
  if (io.digitalRead(buttonRight) == LOW) {
    Serial.print("Right");
  }
  if (io.digitalRead(buttonLeft) == LOW) {
    Serial.print("Left");
  }
  if (io.digitalRead(buttonCenter) == LOW) {
    Serial.print("Center");
  } 
  

}
