#include <Goldelox_Serial_4DLib.h>
#include <Goldelox_Const4D.h>

#define buttonUp 6
#define buttonDown 10
#define buttonRight 11
#define buttonLeft 21
#define buttonCenter 7

#define DisplaySerial Serial1

#define TX 16
#define RX 17


Goldelox_Serial_4DLib Display(&DisplaySerial);

int x = 64;
int y = 64;
int color = 0;
bool previous = false;

void setup() {
  Serial.begin(115200);
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(buttonRight, INPUT);
  pinMode(buttonLeft, INPUT);
  pinMode(buttonCenter, INPUT);

  DisplaySerial.begin(9600, SERIAL_8N1, RX, TX);
  Display.TimeLimit4D = 500;
  Display.gfx_Cls();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(buttonUp) == LOW && digitalRead(buttonLeft) == LOW) {
    Serial.println("Top Left");
    x -= 5;
    y -= 5;
  } else if (digitalRead(buttonUp) == LOW && digitalRead(buttonRight) == LOW) {
    Serial.println("Top Right");
    x += 5;
    y -= 5;
  } else if (digitalRead(buttonDown) == LOW && digitalRead(buttonLeft) == LOW) {
    Serial.println("Bottom Left");
    x -= 5;
    y += 5;
  } else if (digitalRead(buttonDown) == LOW && digitalRead(buttonRight) == LOW) {
    Serial.println("Bottom Right");
    x += 5;
    y += 5;
  } else if (digitalRead(buttonUp) == LOW) {
    Serial.println("Up");
    y -= 5;
  } else if (digitalRead(buttonDown) == LOW) {
    Serial.println("Down");
    y += 5;
  } else if (digitalRead(buttonRight) == LOW) {
    Serial.println("Right");
    x += 5;
  } else if (digitalRead(buttonLeft) == LOW) {
    Serial.println("Left");
    x -= 5;
  } else if (digitalRead(buttonCenter) == LOW) {
    Serial.println("Center");
    x=64;
    y=64;
  }
  if (x > 0 && x < 127 && y > 0 && y < 127) {
    previous = false;
  } else if (!previous) {
    previous = true;
    color = (color + 1) % 3;
  }
  if (x < 0) {
    x = 0;
    previous = true;
  }
  if (x > 127) {
    x = 127;
    previous = true;
  }
  if (y < 0) {
    y = 0;
    previous = true;
  }
  if (y > 127) {
    y = 127;
    previous = true;
  }
  if (color == 0) {
    Display.gfx_CircleFilled(x, y, 5, BLUE);
  } else if (color == 1) {
    Display.gfx_CircleFilled(x, y, 5, WHITE);
  } else {
    Display.gfx_CircleFilled(x, y, 5, YELLOW);
  }
  delay(100);
  Display.gfx_Cls();
}
