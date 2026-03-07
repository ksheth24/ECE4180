#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#define LED_PIN     8     // Data pin connected to onboard RGB LED
#define NUM_LEDS    1     // Only one LED on board

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t pixelColors[6] = {
  pixel.Color(255, 0, 0),    // Red
  pixel.Color(0, 0, 255),    // Blue
  pixel.Color(0, 255, 0),    // Green
  pixel.Color(255, 255, 0),  // Yellow
  pixel.Color(128, 0, 128),  // Purple
  pixel.Color(255, 165, 0)   // Orange
};


#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeMaker host;
CodeBreaker player;

int color1 = 0;
int color2 = 0;
int color3 = 0;
int color4 = 0;
int currentColor = 0;

// Callback function when data is received
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {

  String message = "";

  for (int i = 0; i < len; i++) {
    message += ((char)data[i]);
  }
  Serial.print("Message received: ");
  Serial.print(message);
  Serial.println();
  if (message.indexOf("Up") >= 0) {
    color1 = (color1 + 1) % 6;
    currentColor = color1; 
  } else if (message.indexOf("Down") >= 0) { 
    color2 = (color2 + 1) % 6;
    currentColor = color2;
  } else if (message.indexOf("Left") >= 0) {
    color3 = (color3 + 1) % 6;
    currentColor = color3;
  } else if (message.indexOf("Center") >= 0) {
    Serial.println("Submitted");
    player.notify();
    host.move_callBack();
  } else if (message.indexOf("Right") >= 0) {
    color4 = (color4 + 1) % 6;
    currentColor = color4;
  }
  pixel.setPixelColor(0, pixelColors[currentColor]);
  // pixel.show();
  player.move.playerGuess[0] = color1;
  player.move.playerGuess[1] = color2;
  player.move.playerGuess[2] = color3;
  player.move.playerGuess[3] = color4;
  player.printGuess();
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);  // Must be in station mode

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready");
  Serial.println(WiFi.macAddress());

  player.setup();
  host.setup();

  // Dealer generates secret code and lets player know when to start
  host.generateCode();    // host sets up the code
  host.throwResultsFlag(true);   // Let Player know that they can do something 

  pixel.begin();
  pixel.setBrightness(20);
}

void loop() {
  pixel.show();
  delay(100);
}

bool CodeMaker::throwResultsFlag(bool ongoing){
  player.move.turn = ongoing;
  return true;
}

// Read Player's move and do something
void CodeMaker::move_callBack(){
  // Make a local copy (which is the same when you use BLE)
  uint8_t playersGuess[4];
  for (size_t i = 0; i < 4; ++i) {
    playersGuess[i] = player.move.playerGuess[i];  // Make a non-volatile copy
  }

  // Fill buffer with results from players guess
  uint8_t results[2];
  host.checkGuess(results, playersGuess);
  host.printCode(); // Print actual code for reference
  // Serial.printf("Results: %d | %d \n", results[0], results[1]);
  
  if (results[0] == 4) {
    host.endGame(); // Don't worry about this for the main BLE parts, you'll implement this for extra credit
    host.throwResultsFlag(false);
  } else {
    host.throwResultsFlag(true);
  }
}
