#include <HardwareSerial.h>

#define buttonUp 6
#define buttonDown 10
#define buttonRight 11
#define buttonLeft 21
#define buttonCenter 7

#define DisplaySerial Serial1

#define TX 16
#define RX 17


HardwareSerial uLCDSerial(1);

uint16_t x = 64;
uint16_t y = 64;
int color = 0;
bool previous = false;
bool automatic = false;
int xPrev = 0;
int yPrev = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(buttonRight, INPUT);
  pinMode(buttonLeft, INPUT);
  pinMode(buttonCenter, INPUT);

  uLCDSerial.begin(9600, SERIAL_8N1, RX, TX);
  // Display.TimeLimit4D = 500;
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xD7);
}
void loop() {
  // put your main code here, to run repeatedly:
  if (automatic) {
    x += 5 * xPrev;
    y += 5 * yPrev;
  }
  if (!automatic && digitalRead(buttonUp) == LOW && digitalRead(buttonLeft) == LOW) {
    Serial.println("Top Left");
    x -= 5;
    y -= 5;
    xPrev = -1;
    yPrev = -1;
  } else if (!automatic && digitalRead(buttonUp) == LOW && digitalRead(buttonRight) == LOW) {
    Serial.println("Top Right");
    x += 5;
    y -= 5;
  } else if (!automatic && digitalRead(buttonDown) == LOW && digitalRead(buttonLeft) == LOW) {
    Serial.println("Bottom Left");
    x -= 5;
    y += 5;
    xPrev = -1;
    yPrev = 1;
  } else if (!automatic && digitalRead(buttonDown) == LOW && digitalRead(buttonRight) == LOW) {
    Serial.println("Bottom Right");
    x += 5;
    y += 5;
    xPrev = 1;
    yPrev = 1;
  } else if (!automatic && digitalRead(buttonUp) == LOW) {
    Serial.println("Up");
    y -= 5;
    xPrev = 0;
    yPrev = -1;
  } else if (!automatic && digitalRead(buttonDown) == LOW) {
    Serial.println("Down");
    y += 5;
    xPrev = 0;
    yPrev = 1;
  } else if (!automatic && digitalRead(buttonRight) == LOW) {
    Serial.println("Right");
    x += 5;
    xPrev = 1;
    yPrev = 0;
  } else if (!automatic && digitalRead(buttonLeft) == LOW) {
    Serial.println("Left");
    x -= 5;
    xPrev = -1;
    yPrev = 0;
  } else if (digitalRead(buttonCenter) == LOW) {
    Serial.println("Center");
    if (automatic) {
      automatic = false;
    } else {
      automatic = true;
    }
  }
  if (x > 5 && x < 122 && y > 5 && y < 122) {
    previous = false;
  } else if (!previous) {
    previous = true;
    color = (color + 1) % 3;
    xPrev *= -1;
    yPrev *= -1;
  }
  if (x < 5) {
    x = 5;
    previous = true;
  }
  if (x > 122) {
    x = 122;
    previous = true;
  }
  if (y < 5) {
    y = 0;
    previous = true;
  }
  if (y > 122) {
    y = 122;
    previous = true;
  }
  if (color == 0) {
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xCC);

    // set x1
    uLCDSerial.write(x >> 8);
    uLCDSerial.write(x & 0xFF);

    //set y1
    uLCDSerial.write(y >> 8);
    uLCDSerial.write(y & 0xFF);

    // radius
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x05);

    // color
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x1F);
  } else if (color == 1) {
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xCC);

    // set x1
    uLCDSerial.write(x >> 8);
    uLCDSerial.write(x & 0xFF);

    //set y1
    uLCDSerial.write(y >> 8);
    uLCDSerial.write(y & 0xFF);

    // radius
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x05);

    // color
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xFF);
  } else {
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xCC);

    // set x1
    uLCDSerial.write(x >> 8);
    uLCDSerial.write(x & 0xFF);

    //set y1
    uLCDSerial.write(y >> 8);
    uLCDSerial.write(y & 0xFF);

    // radius
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x05);

    // color
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xE0);
  }
  delay(100);
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xD7);
}
