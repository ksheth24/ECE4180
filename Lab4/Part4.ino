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
void bleGameplayTask(void *pvParameters) {
    
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
