/**
 * @file NimBLE_Server.ino
 * @author H2zero
 * @author Jason Hsiao
 * @author STUDENT NAME HERE
 * @date 12/27/2025
 * @version 1.0
 *
 * @brief Starter Code for the BLE Server portion of ECE4180 Lab 3 (Part 3 & 4)
 * This part is re-usable for both parts, as long as you flash this code to both
 * MCUs (though you might need to change the device name).
 * @see Course website, Canvas, GitHub or PowerPoint slides for more details
 */


#include <Arduino.h>
#include <NimBLEDevice.h>

#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeBreaker player;


//==============================================//
// TODO: Define constants here                  //
//==============================================//
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

#define buttonUp 6
#define buttonDown 10
#define buttonLeft 21
#define buttonRight 11
#define buttonCenter 7

int color1 = 0;
int color2 = 0;
int color3 = 0;
int color4 = 0;
int currentColor = 0;

static NimBLEServer* pServer;

NimBLECharacteristic* pCharacteristic = nullptr;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());

    /**
         *  We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments.
         */
    pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    Serial.printf("Client disconnected - start advertising\n");
    NimBLEDevice::startAdvertising();
  }

} serverCallbacks;

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    Serial.printf("%s : onRead(), value: %s\n",
                  pCharacteristic->getUUID().toString().c_str(),
                  pCharacteristic->getValue().c_str());
  }

  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    Serial.printf("%s : onWrite(), value: %s\n",
                  pCharacteristic->getUUID().toString().c_str(),
                  pCharacteristic->getValue().c_str());
                  // Retrieve the feedback written by the Codemaker (Client)
            std::string val = pCharacteristic->getValue();
    //==============================================//
    // TODO: Read the response from Codemaker here  //
    //==============================================//
  }

  /**
     *  The value returned in code is the NimBLE host return code.
     */
  void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
    Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
  }

  /** Peer subscribed to notifications/indications */
  void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
    std::string str = "Client ID: ";
    str += connInfo.getConnHandle();
    str += " Address: ";
    str += connInfo.getAddress().toString();
    if (subValue == 0) {
      str += " Unsubscribed to ";
    } else if (subValue == 1) {
      str += " Subscribed to notifications for ";
    } else if (subValue == 2) {
      str += " Subscribed to indications for ";
    } else if (subValue == 3) {
      str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID());

    Serial.printf("%s\n", str.c_str());
  }
} chrCallbacks;

void IRAM_ATTR navISR(void* arg) {
  int pin = (int)arg;
  if (pin == buttonUp) {
    color1 = (color1 + 1) % 6;
    currentColor = color1;
  } else if (pin == buttonDown) {
    color2 = (color2 + 1) % 6;
    currentColor = color2;
  } else if (pin == buttonLeft) {
    color3 = (color3 + 1) % 6;
    currentColor = color3;
  } else {
    color4 = (color4 + 1) % 6;
    currentColor = color4;
  }
  pixel.setPixelColor(0, pixelColors[currentColor]);
  player.move.playerGuess[0] = color1;
  player.move.playerGuess[1] = color2;
  player.move.playerGuess[2] = color3;
  player.move.playerGuess[3] = color4;
}

void IRAM_ATTR submit() {
  Serial.println("Submitted");

  player.notify();
  pCharacteristic->setValue((const uint8_t*)player.move.playerGuess, sizeof(player.move.playerGuess));
  pCharacteristic->notify();
}


void setup(void) {
  Serial.begin(115200);
  Serial.printf("Starting NimBLE Server\n");

  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(buttonRight, INPUT);
  pinMode(buttonLeft, INPUT);
  pinMode(buttonCenter, INPUT);

  attachInterruptArg(buttonUp, navISR, (void*)buttonUp, FALLING);
  attachInterruptArg(buttonDown, navISR, (void*)buttonDown, FALLING);
  attachInterruptArg(buttonLeft, navISR, (void*)buttonLeft, FALLING);
  attachInterruptArg(buttonRight, navISR, (void*)buttonRight, FALLING);
  attachInterrupt(buttonCenter, submit, FALLING);

  /** Initialize NimBLE and set the device name */
  NimBLEDevice::init("Player1");


  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  //============================================//
  // TODO: Configure your Characteristics here  //
  // Example below        
  NimBLEService* pBoardService = pServer->createService("2006");                      //
  pCharacteristic = pBoardService->createCharacteristic("0001", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  // Will require extra steps                   //
  //============================================//
  

  /** Start the services when finished creating all Characteristics and Descriptors */
  pBoardService->start();

  /** Create an advertising instance and add the services to the advertised data */
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("Mastermind-Player1");  // TODO: Set a unique name here
  pAdvertising->addServiceUUID(pBoardService->getUUID());

  /**
     *  If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();

  Serial.printf("Advertising Started\n");

  //======================================//
  // TODO: Set-up your code-breaker here  //
  //======================================//
  player.setup();
}

void loop() {
  /** Loop here and send notifications to connected peers */
  
  //======================================//
  // TODO: Game Logic Here                //
  // Example of how to use BLE below      //
  // Comment this out if needed           //
  // uint8_t message[] = "Good Luck everybody!! - Jason";
  // while (1) {
  //   pCharacteristic->setValue(message, sizeof(message) - 1);
  //   pCharacteristic->notify();
  //   delay(1000);
  // }
  // //======================================//
  player.printGuess();
  pixel.show();
  delay(100);
}
