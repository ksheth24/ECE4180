/*
    ESP-NOW Broadcast Slave
    Lucas Saavedra Vaz - 2024

    This sketch demonstrates how to receive broadcast messages from a master device using the ESP-NOW protocol.
*/
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "ESP32_NOW.h"

#include "WiFi.h"

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

#include <Adafruit_NeoPixel.h>

#include <esp_mac.h>
#include <vector>

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6

/* Classes */

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:

  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
      : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  ~ESP_NOW_Peer_Class() {}

  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  void onReceive(const uint8_t *data, size_t len, bool broadcast) {

    Serial.printf("Received a message from master " MACSTR " (%s)\n",
                  MAC2STR(addr()),
                  broadcast ? "broadcast" : "unicast");

    Serial.printf("Message: %.*s\n", len, (char *)data);

    if (len == 0) return;

    switch ((char)data[0]) {
      case 'U':
        Serial.println("Up");
        color1 = (color1 + 1) % 6;
        currentColor = color1;
        break;

      case 'D':
        Serial.println("Down");
        color2 = (color2 + 1) % 6;
        currentColor = color2;
        break;

      case 'R':
        Serial.println("Right");
        color4 = (color4 + 1) % 6;
        currentColor = color4;
        break;

      case 'L':
        color3 = (color3 + 1) % 6;
        currentColor = color3;
        Serial.println("Left");
        break;

      case 'C':
        Serial.println("Center");
        Serial.println("Submitted");
        player.notify();
        host.move_callBack();
        break;
    }
    pixel.setPixelColor(0, pixelColors[currentColor]);
  // pixel.show();
    player.move.playerGuess[0] = color1;
    player.move.playerGuess[1] = color2;
    player.move.playerGuess[2] = color3;
    player.move.playerGuess[3] = color4;
    player.printGuess();
  }
};

/* Global Variables */

// List of all masters
std::vector<ESP_NOW_Peer_Class *> masters;

/* Callbacks */

// Called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {

  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {

    Serial.printf("Unknown peer " MACSTR " sent broadcast\n",
                  MAC2STR(info->src_addr));

    Serial.println("Registering peer as master");

    ESP_NOW_Peer_Class *new_master =
        new ESP_NOW_Peer_Class(info->src_addr,
                               ESPNOW_WIFI_CHANNEL,
                               WIFI_IF_STA,
                               nullptr);

    if (!new_master->add_peer()) {
      Serial.println("Failed to register new master");
      delete new_master;
      return;
    }

    masters.push_back(new_master);

    Serial.printf("Master registered " MACSTR " (total: %lu)\n",
                  MAC2STR(new_master->addr()),
                  (unsigned long)masters.size());

  } else {

    log_v("Received unicast from " MACSTR, MAC2STR(info->src_addr));
    log_v("Ignoring message");

  }
}

/* Main */

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  player.setup();
  host.setup();

  // Dealer generates secret code and lets player know when to start
  host.generateCode();    // host sets up the code
  host.throwResultsFlag(true);   // Let Player know that they can do something 
  pixel.begin();
  pixel.setBrightness(20);

  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.println("ESP-NOW Broadcast Slave");
  Serial.println("WiFi parameters:");

  Serial.println("Mode: STA");
  Serial.println("MAC Address: " + WiFi.macAddress());

  Serial.printf("Channel: %u\n", ESPNOW_WIFI_CHANNEL);

  if (!ESP_NOW.begin()) {

    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Rebooting in 5 seconds...");

    delay(5000);
    ESP.restart();
  }

  Serial.printf("ESP-NOW version: %d\n", ESP_NOW.getVersion());
  Serial.printf("Max data length: %d\n", ESP_NOW.getMaxDataLen());

  ESP_NOW.onNewPeer(register_new_master, nullptr);

  Serial.println("Setup complete. Waiting for broadcast...");
}

void loop() {

  static unsigned long last_debug = 0;

  if (millis() - last_debug > 10000) {

    last_debug = millis();

    Serial.printf("Registered masters: %lu\n",
                  (unsigned long)masters.size());

    for (size_t i = 0; i < masters.size(); i++) {

      if (masters[i]) {

        Serial.printf("Master %lu: " MACSTR "\n",
                      (unsigned long)i,
                      MAC2STR(masters[i]->addr()));
      }
    }
  }
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
