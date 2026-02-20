#include <Adafruit_MCP23X17.h>

#define BUTTON 7
#define LED 9

#define CS 4
#define MOSI 5
#define MISO 6
#define SCK 7

Adafruit_MCP23X17 mcp;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("CONFIG");

  if(!mcp.begin_SPI(CS, SCK, MISO, MOSI, 0)) {
    Serial.println("Error.");
    while(1);
  }

  mcp.pinMode(BUTTON, INPUT_PULLUP);
  mcp.pinMode(LED, OUTPUT);
}

void loop() {
  if (!mcp.digitalRead(BUTTON)) {
    mcp.digitalWrite(LED, HIGH);
    Serial.println("Button press");
  } else {
    mcp.digitalWrite(LED, LOW);
  }
  
}
