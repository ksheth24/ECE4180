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

void setup() {
  Serial.begin(115200);
  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(buttonRight, INPUT);
  pinMode(buttonLeft, INPUT);
  pinMode(buttonCenter, INPUT);

  uLCDSerial.begin(9600, SERIAL_8N1, RX, TX);
  // check if you need to add this
  //Display.TimeLimit4D = 500;
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xD7);
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
    // draw filled circle
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xCF);

    // set x1
    uLCDSerial.write(x >> 8);
    uLCDSerial.write(x & 0xFF);

    //set y1
    uLCDSerial.write(y >> 8);
    uLCDSerial.write(y & 0xFF);

    // radius
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x0F);

    // color
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x1F);

    // Display.gfx_CircleFilled(x, y, 5, BLUE);
  } else if (color == 1) {
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xCF);

    // set x1
    uLCDSerial.write(x >> 8);
    uLCDSerial.write(x & 0xFF);

    //set y1
    uLCDSerial.write(y >> 8);
    uLCDSerial.write(y & 0xFF);

    // radius
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x0F);

    // color
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xFF);
    // Display.gfx_CircleFilled(x, y, 5, WHITE);
  } else {
     uLCDSerial.write(0xFF);
    uLCDSerial.write(0xCF);

    // set x1
    uLCDSerial.write(x >> 8);
    uLCDSerial.write(x & 0xFF);

    //set y1
    uLCDSerial.write(y >> 8);
    uLCDSerial.write(y & 0xFF);

    // radius
    uLCDSerial.write(0x00);
    uLCDSerial.write(0x0F);

    // color
    uLCDSerial.write(0xFF);
    uLCDSerial.write(0xE0);
    // Display.gfx_CircleFilled(x, y, 5, YELLOW);
  }
  delay(100);
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xD7);
}
