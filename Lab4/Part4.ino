#include <Arduino.h>
#define BUTTON_PIN 7

// --- Include whatever FreeRTOS libraries you might need ---
#if CONFIG_FREERTOS_UNICORE
#define TASK_RUNNING_CORE 0
#else
#define TASK_RUNNING_CORE 1
#endif


// --- Declare Semaphore and Mutex Stuff ---


// --- Define MasterMind stuff ---
#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeMaker dealer;
CodeBreaker player;


// --- Define BLE stuff ---
#include <NimBLEDevice.h>
static NimBLEServer* pServer;

NimBLECharacteristic* pCharacteristic = nullptr;

// --- Define AI/ML stuff ---
uint8_t* code = 0;


// -------------------------------------------------------------------------
// CORE 1: AI & ML TASKS
// -------------------------------------------------------------------------

// Task A: Constant ML Inference (Triggered by data)
void mlInferenceTask(void *pvParameters) {
    
}

// Button ISR (Required)
void IRAM_ATTR handleButtonPress() {
  Serial.println("yo");
}

// Task B: AI Guess Making (Triggered by Button)
void aiGuessTask(void *pvParameters) {
    
}

// -------------------------------------------------------------------------
// CORE 0: BLE & WEB TASK
// Add whatever helper functions you might need
// -------------------------------------------------------------------------

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
    Serial.printf("%s : onWrite(), value: %d, %d\n",
                  pCharacteristic->getUUID().toString().c_str(),
                  pCharacteristic->getValue()[0], pCharacteristic->getValue()[1]);
    Serial.printf("Feedback recieved! Black: %d, White: %d\n", pCharacteristic->getValue()[0], pCharacteristic->getValue()[1]);
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

void bleGameplayTask(void *pvParameters) {
    Serial.println("Sent.");
    player.notify();
    pCharacteristic->setValue((const uint8_t*)player.move.playerGuess, sizeof(player.move.playerGuess));
    pCharacteristic->notify();
    submitted = false;
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);   // allow serial to attach

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("Player1");


    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    //============================================//
    // TODO: Configure your Characteristics here  //
    // Example below        
    NimBLEService* pBoardService = pServer->createService("2006");                      //
    pCharacteristic = pBoardService->createCharacteristic("0001", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    pCharacteristic->setCallbacks(&chrCallbacks);
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
    
    // TODO: Set-up CodeMaker and CodeBreaker objects 
    player.setup();
    host.setup();
    // TODO: Initialize Synchronization Stuff,like Semaphores and Mutexes

    // TODO: Initialize your button with an ISR (you will need to configure an ISR with this RTOS)
    pinMode(BUTTON_PIN, INPUT);    // external hardware debounce

    attachInterrupt(BUTTON_PIN, buttonISR, RISING);
    
    // TODO: Pin Tasks to Cores
    // Core 1: AI and ML (Higher priority for pruning)
    xTaskCreatePinnedToCore(
      mlInferenceTask, 
      "ml", 
      10000, 
      NULL, 
      1, 
      &Task2, 
      1);
    delay(500);

    xTaskCreatePinnedToCore(
      aiGuessTask, 
      "ai", 
      10000, 
      NULL, 
      1, 
      &Task3, 
      1);
    delay(500);
    // Core 0: BLE
    xTaskCreatePinnedToCore(
      bleGameplayTask, 
      "ble", 
      10000, 
      NULL, 
      1, 
      &Task1, 
      0);
    delay(500);

}

void loop() {
    // Empty: Everything is managed by FreeRTOS tasks
    vTaskDelete(NULL); 
}
