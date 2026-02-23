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
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xD7);
}

void loop() {
  if (automatic) {
    x += 5 * xPrev;
    y += 5 * yPrev;
  }

  if (!automatic && digitalRead(buttonUp) == LOW && digitalRead(buttonLeft) == LOW) {
    Serial.println("Top Left");
    x -= 5; y -= 5;
    xPrev = -1; yPrev = -1;
  } else if (!automatic && digitalRead(buttonUp) == LOW && digitalRead(buttonRight) == LOW) {
    Serial.println("Top Right");
    x += 5; y -= 5;
    xPrev = 1; yPrev = -1;
  } else if (!automatic && digitalRead(buttonDown) == LOW && digitalRead(buttonLeft) == LOW) {
    Serial.println("Bottom Left");
    x -= 5; y += 5;
    xPrev = -1; yPrev = 1;
  } else if (!automatic && digitalRead(buttonDown) == LOW && digitalRead(buttonRight) == LOW) {
    Serial.println("Bottom Right");
    x += 5; y += 5;
    xPrev = 1; yPrev = 1;
  } else if (!automatic && digitalRead(buttonUp) == LOW) {
    Serial.println("Up");
    y -= 5;
    xPrev = 0; yPrev = -1;
  } else if (!automatic && digitalRead(buttonDown) == LOW) {
    Serial.println("Down");
    y += 5;
    xPrev = 0; yPrev = 1;
  } else if (!automatic && digitalRead(buttonRight) == LOW) {
    Serial.println("Right");
    x += 5;
    xPrev = 1; yPrev = 0;
  } else if (!automatic && digitalRead(buttonLeft) == LOW) {
    Serial.println("Left");
    x -= 5;
    xPrev = -1; yPrev = 0;
  } else if (digitalRead(buttonCenter) == LOW) {
    Serial.println("Center");
    automatic = !automatic;
  }

  // Word pixel width (~6px per char for the built-in font)
  int wordPixelWidth;
  if ((xPrev == 1 && yPrev == 1) || (xPrev == 1 && yPrev == -1)) {
    wordPixelWidth = 11 * 6; // "Lower Right" / "Upper Right"
  } else if ((xPrev == -1 && yPrev == 1) || (xPrev == -1 && yPrev == -1)) {
    wordPixelWidth = 10 * 6; // "Lower Left" / "Upper Left"
  } else if (xPrev == 1 || xPrev == -1) {
    wordPixelWidth = 5 * 6;  // "Right" / "Left"
  } else if (yPrev == 1) {
    wordPixelWidth = 4 * 6;  // "Down"
  } else {
    wordPixelWidth = 2 * 6;  // "Up"
  }

  int xMax = 128 - wordPixelWidth;
  int yMax = 120;

  if (x > 5 && x < xMax && y > 5 && y < yMax) {
    previous = false;
  } else if (!previous) {
    previous = true;
    color = (color + 1) % 3;
    xPrev *= -1;
    yPrev *= -1;
  }

  if (x < 5)    { x = 5;    previous = true; }
  if (x > xMax) { x = xMax; previous = true; }
  if (y < 5)    { y = 5;    previous = true; }
  if (y > yMax) { y = yMax; previous = true; }

  // Move cursor
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xE4);
  uLCDSerial.write(0x00);
  uLCDSerial.write(y / 8);
  uLCDSerial.write(0x00);
  uLCDSerial.write(x / 6);

  // Write string command
  uLCDSerial.write(0x00);
  uLCDSerial.write(0x06);

  if (xPrev == 1 && yPrev == 1) {
    // "Lower Right"
    uLCDSerial.write(0x4C); // L
    uLCDSerial.write(0x6F); // o
    uLCDSerial.write(0x77); // w
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x72); // r
    uLCDSerial.write(0x20); // (space)
    uLCDSerial.write(0x52); // R
    uLCDSerial.write(0x69); // i
    uLCDSerial.write(0x67); // g
    uLCDSerial.write(0x68); // h
    uLCDSerial.write(0x74); // t
    uLCDSerial.write(0x00); // null
  } else if (xPrev == 1 && yPrev == -1) {
    // "Upper Right"
    uLCDSerial.write(0x55); // U
    uLCDSerial.write(0x70); // p
    uLCDSerial.write(0x70); // p
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x72); // r
    uLCDSerial.write(0x20); // (space)
    uLCDSerial.write(0x52); // R
    uLCDSerial.write(0x69); // i
    uLCDSerial.write(0x67); // g
    uLCDSerial.write(0x68); // h
    uLCDSerial.write(0x74); // t
    uLCDSerial.write(0x00); // null
  } else if (xPrev == -1 && yPrev == 1) {
    // "Lower Left"
    uLCDSerial.write(0x4C); // L
    uLCDSerial.write(0x6F); // o
    uLCDSerial.write(0x77); // w
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x72); // r
    uLCDSerial.write(0x20); // (space)
    uLCDSerial.write(0x4C); // L
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x66); // f
    uLCDSerial.write(0x74); // t
    uLCDSerial.write(0x00); // null
  } else if (xPrev == -1 && yPrev == -1) {
    // "Upper Left"
    uLCDSerial.write(0x55); // U
    uLCDSerial.write(0x70); // p
    uLCDSerial.write(0x70); // p
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x72); // r
    uLCDSerial.write(0x20); // (space)
    uLCDSerial.write(0x4C); // L
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x66); // f
    uLCDSerial.write(0x74); // t
    uLCDSerial.write(0x00); // null
  } else if (xPrev == 1) {
    // "Right"
    uLCDSerial.write(0x52); // R
    uLCDSerial.write(0x69); // i
    uLCDSerial.write(0x67); // g
    uLCDSerial.write(0x68); // h
    uLCDSerial.write(0x74); // t
    uLCDSerial.write(0x00); // null
  } else if (xPrev == -1) {
    // "Left"
    uLCDSerial.write(0x4C); // L
    uLCDSerial.write(0x65); // e
    uLCDSerial.write(0x66); // f
    uLCDSerial.write(0x74); // t
    uLCDSerial.write(0x00); // null
  } else if (yPrev == 1) {
    // "Down"
    uLCDSerial.write(0x44); // D
    uLCDSerial.write(0x6F); // o
    uLCDSerial.write(0x77); // w
    uLCDSerial.write(0x6E); // n
    uLCDSerial.write(0x00); // null
  } else {
    // "Up"
    uLCDSerial.write(0x55); // U
    uLCDSerial.write(0x70); // p
    uLCDSerial.write(0x00); // null
  }

  delay(100);
  uLCDSerial.write(0xFF);
  uLCDSerial.write(0xD7);
}
