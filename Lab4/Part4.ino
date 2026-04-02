#include <Arduino.h>


// --- Declare Semaphore and Mutex Stuff ---


// --- Define MasterMind stuff ---
#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

#define BUTTON_PIN 7

CodeMaker dealer;
CodeBreaker player;

//Random ass neopixel initialization
#include <Adafruit_NeoPixel.h>

#define LED_PIN 8
#define NUM_PIXELS 1

Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800); 

// --- Bluetooth Stuff ---

#include <NimBLEDevice.h>

#define DEVICE_NAME  "Player1"
#define SERVICE_UUID "2006"
#define CHAR_UUID    "0001"

NimBLEServer*         pServer  = nullptr;
NimBLECharacteristic* pSrvChar = nullptr;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());

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
    xQueueSend(myQueue, getValue(), portMAX_DELAY);
  }

  void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
    Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
  }

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


// --- Define AI/ML stuff ---

#include <DealerClassifier_inferencing.h>

#define ARRAY_SIZE 1296
#define VECTOR_SIZE 4
#define BASE 6
uint8_t possibilities[ARRAY_SIZE][VECTOR_SIZE];
bool active[ARRAY_SIZE];

int currentGuessIdx;
uint8_t turns;
bool gameRunning;

float classifierConfidence[3] = {0, 0, 0};
uint8_t dealerStyle;
uint8_t currentDealerStyle;
uint8_t* guess;

SemaphoreHandle_t btnSem;
SemaphoreHandle_t aiGuessToBLE;
QueueHandle_t myQueue;

void populateArray(uint8_t (*array)[VECTOR_SIZE], uint32_t size) {
  uint8_t counters[VECTOR_SIZE] = {0};

  for (uint32_t i = 0; i < size; i++) {
    active[i] = true;
    memcpy(array[i], counters, VECTOR_SIZE);
    counters[0]++;
    for (uint8_t k = 0; k < VECTOR_SIZE - 1; k++) {
      if (counters[k] >= BASE) {
        counters[k] = 0;
        counters[k + 1]++;
      }
    }
  }
}

void prune(uint8_t* lastGuess, uint8_t* results) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (!active[i]) continue; // Already pruned

        // "If possibilities[i] was the secret, what feedback would it give?"
        uint8_t reference[2];
        dealer.compare(reference, possibilities[i], lastGuess);

        // Remove possible answers that do not match
        if (reference[0] != results[0] || reference[1] != results[1]) {
            active[i] = false; 
        } 
    }
}

int getBestGuess() {
    int bestGuessIdx = -1;
    int minMaxRemaining = 2000;

    Serial.println("Calculating best guess...");

    Serial.println("Timer configured and started");
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (i == 0) {
            int activeCount = 0;
            for(int k=0; k<ARRAY_SIZE; k++) if(active[k]) activeCount++;
            Serial.printf("Active codes remaining: %d\n", activeCount);
        }
        if (i % 200 == 0) Serial.print("."); 

        // KNUTH'S RULE: We can guess ANY code (even inactive ones)
        // to gain maximum information. Equivalent to in Wordle when you have something
        // like _ATCH so you guess PLUMB to narrow it down even if it doesn't work
      
        int scoreCount[5][5] = {0}; 
        for (int j = 0; j < ARRAY_SIZE; j++) {
            if (!active[j]) continue;
            
            uint8_t check[2];
            dealer.compare(check, possibilities[i], possibilities[j]);
            scoreCount[check[0]][check[1]]++;
        }

        int maxForThisGuess = 0;
        for(int b=0; b<=4; b++) {
            for(int w=0; w<=4; w++) {
                if(scoreCount[b][w] > maxForThisGuess) 
                    maxForThisGuess = scoreCount[b][w];
            }
        }

        if (maxForThisGuess < minMaxRemaining) {
            minMaxRemaining = maxForThisGuess;
            bestGuessIdx = i;
        } else if (maxForThisGuess == minMaxRemaining && active[i] && !active[bestGuessIdx]) {
            bestGuessIdx = i;
        }
    }
    
    Serial.printf("\nDone! Took %.2f ms. Best Index: %d\n", millis(), bestGuessIdx);
    return bestGuessIdx;
}

void classifyDealerStyle(uint8_t* code) {
    int uniqueCount = 0;
    int isDescending = code[0] >= code[1] && code[1] >= code[2] && code[2] >= code[3];
    int maxV = 0;
    bool unique = false;
    for (int i = 0; i < 4; i++) {
      unique = true;
      if (code[i] > maxV) {
        maxV = code[i];
      }
      for (int j = i - 1; j >= 0; j--) {
        if (code[j] == code[i]) {
          unique = false;
        }
      }
      if (unique) {
        uniqueCount++;
      }
    }

    float features[7];
    features[0] = code[0];
    features[1] = code[1];
    features[2] = code[2];
    features[3] = code[3];
    features[4] = uniqueCount;
    features[5] = isDescending;
    features[6] = maxV;

    signal_t signal;
    numpy::signal_from_buffer(features, 7, &signal);

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) return;

    // Print the results (the Style Probabilities)
    Serial.printf("Predictions (DSP: %d ms, NN: %d ms)\n", 
                  result.timing.dsp, result.timing.classification);
    
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.printf("    %s: %.2f\n", result.classification[ix].label,
                                        result.classification[ix].value);
        classifierConfidence[ix] += result.classification[ix].value;
    }
}

void prePrune(uint8_t predictedStyle) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
      if (!active[i]) {
        continue;
      }
      uint8_t* code = possibilities[i];
      int uniqueCount = 0;
      int isDescending = code[0] >= code[1] && code[1] >= code[2] && code[2] >= code[3];
      int maxV = 0;
      bool unique = false;
      for (int k = 0; k < 4; k++) {
        unique = true;
        if (code[k] > maxV) {
          maxV = code[k];
        }
        for (int j = k - 1; j >= 0; j--) {
          if (code[j] == code[k]) {
            unique = false;
          }
        }
        if (unique) {
          uniqueCount++;
        }
      }
      if (predictedStyle == 0) {         
          if (!isDescending) active[i] = false;
      }
      else if (predictedStyle == 1) {    
          if (maxV > 2) active[i] = false;
      }
      else if (predictedStyle == 2) {    
          if (uniqueCount != 2) active[i] = false;
      }
    }

}

// -------------------------------------------------------------------------
// CORE 1: AI & ML TASKS
// -------------------------------------------------------------------------

TaskHandle_t taskMLHandle;
TaskHandle_t taskAIHandle;
TaskHandle_t taskBLEHandle;

// Task A: Constant ML Inference (Triggered by data)
void mlInferenceTask(void *pvParameters) {
  classifierConfidence[0] = 0;
  classifierConfidence[1] = 0;
  classifierConfidence[2] = 0;
  for (int i = 0; i < 10; i++) {
    uint8_t code[4];

    dealer.generateBiasedCode(static_cast<uint>(random(0,3)));
    dealer.getCode(code);    
    classifyDealerStyle(code);
  }
  // TODO
    for(;;) {
    uint8_t results[2];

    if (xQueueReceive(myQueue, results, portMAX_DELAY)) {
      if (results[0] == 4) {
        classifyDealerStyle(guess);
      }
    }
  }
}

// Button ISR (Required)
void IRAM_ATTR handleButtonPress() {
  xSemaphoreGiveFromISR(btnSem, NULL);
}

// Task B: AI Guess Making (Triggered by Button)
void aiGuessTask(void *pvParameters) {
  populateArray(possibilities, ARRAY_SIZE);
  prePrune(currentDealerStyle);
    for(;;) {
      if (xSemaphoreTake(btnSem, portMAX_DELAY)) {
        int guessIdx = getBestGuess();
        guess = possibilities[guessIdx];
        xSemaphoreGive(aiGuessToBLE);
      }
    }
}

// -------------------------------------------------------------------------
// CORE 0: BLE & WEB TASK
// Add whatever helper functions you might need
// -------------------------------------------------------------------------
void bleGameplayTask(void *pvParameters) {
  for(;;) {
    if(xSemaphoreTake(aiGuessToBLE, portMAX_DELAY)) {
      player.notify();
      pSrvChar->setValue((const uint8_t*)guess, sizeof(int));
      pSrvChar->notify();
    }
  }
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);   // allow serial to attach

    //BLE Initialization Stuff
    NimBLEDevice::init("Player1");

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
  
    NimBLEService* pBoardService = pServer->createService("2006");
    pSrvChar = pBoardService->createCharacteristic("0001", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    pSrvChar->setCallbacks(&chrCallbacks);

    pBoardService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName("Mastermind-Bot");
    pAdvertising->addServiceUUID(pBoardService->getUUID());

    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    Serial.printf("Advertising Started\n");
    
    // TODO: Set-up CodeMaker and CodeBreaker objects 
    player.setup();
    dealer.setup();

    // TODO: Initialize Synchronization Stuff,like Semaphores and Mutexes
    btnSem = xSemaphoreCreateBinary();
    aiGuessToBLE = xSemaphoreCreateBinary();
    myQueue = xQueueCreate(10, sizeof(int));

    // TODO: Initialize your button with an ISR (you will need to configure an ISR with this RTOS)
    pinMode(BUTTON_PIN, INPUT);
    attachInterrupt(BUTTON_PIN, handleButtonPress, RISING);
    
    // TODO: Pin Tasks to Cores
    // Core 1: AI and ML (Higher priority for pruning)
    xTaskCreatePinnedToCore(mlInferenceTask, "ML Inference", 4096, NULL, 1, &taskMLHandle, 1);
    xTaskCreatePinnedToCore(aiGuessTask, "AI Guess", 4096, NULL, 1, &taskAIHandle, 1);

    // Core 0: BLE
    xTaskCreatePinnedToCore(bleGameplayTask, "BLE Send Guess", 4096, NULL, 1, &taskBLEHandle, 0);
}

void loop() {
    // Empty: Everything is managed by FreeRTOS tasks
    vTaskDelete(NULL); 
}
