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
  if (!io.begin()) {
    Serial.println("I2C Expander not detected");
    while (1);
  }
  io.pinMode(buttonUp, INPUT);
  io.pinMode(buttonDown, INPUT);
  io.pinMode(buttonRight, INPUT);
  io.pinMode(buttonLeft, INPUT);
  io.pinMode(buttonCenter, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (io.digitalRead(buttonUp) == LOW && io.digitalRead(buttonLeft) == LOW) {
    Serial.print("Top Left");
  } else if (io.digitalRead(buttonUp) == LOW && io.digitalRead(buttonRight) == LOW) {
    Serial.print("Top Right");
  } else if (io.digitalRead(buttonDown) == LOW && io.digitalRead(buttonLeft) == LOW) {
    Serial.print("Bottom Left");
  } else if (io.digitalRead(buttonDown) == LOW && io.digitalRead(buttonRight) == LOW) {
    Serial.print("Bottom Right");
  } else if (io.digitalRead(buttonUp) == LOW) {
    Serial.print("Up");
  } else if (io.digitalRead(buttonDown) == LOW) {
    Serial.print("Down");
  } else if (io.digitalRead(buttonRight) == LOW) {
    Serial.print("Right");
  } else if (io.digitalRead(buttonLeft) == LOW) {
    Serial.print("Left");
  } else if (io.digitalRead(buttonCenter) == LOW) {
    Serial.print("Center");
  } 
  

}
