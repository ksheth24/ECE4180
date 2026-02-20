#include <Wire.h>
#include <Adafruit_MCP4725.h>

#define OUT 4

Adafruit_MCP4725 dac;

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(OUT, OUTPUT);

  Wire.begin(23, 22);
  dac.begin(0x60, &Wire);
  Serial.print("DAC Init");
}

void loop() {
  for (int i = 0; i < 360; i++) {
    uint16_t dacVal = 1650*sin(radians(i)) + 1650;
    dac.setVoltage(dacVal, false);
    delay(200);
  }
}
