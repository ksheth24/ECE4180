#include <Arduino.h>
#define BUTTON_PIN 7

// --- Include whatever FreeRTOS libraries you might need ---

#include <FreeRTOS.h>
#include <Task.h>
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;


#include <Adafruit_NeoPixel.h>

#define LED_PIN 8        // or whatever your board uses
#define NUM_PIXELS 1     // usually 1 for these labs

Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

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


uint8_t possibilities[ARRAY_SIZE][VECTOR_SIZE];
bool active[ARRAY_SIZE];

int currentGuessIdx;
uint8_t turns;
bool gameRunning;

float classifierConfidence[3] = {0, 0, 0}; // [descending, low_numbers, two_pairs] - Model is in different order than style selection
uint8_t dealerStyle; // Randomly generated dealer style
uint8_t currentDealerStyle; // What the ML will recognize as the dealer's style

// -------------------------------------------------------------------------
// CORE 1: AI & ML TASKS
// -------------------------------------------------------------------------

// Task A: Constant ML Inference (Triggered by data)
void populateArray(uint8_t (*array)[VECTOR_SIZE], uint32_t size) {
  uint8_t counters[VECTOR_SIZE] = {0};

  for (uint32_t i = 0; i < size; i++) {
    active[i] = true;
    // Copy current vector
    memcpy(array[i], counters, VECTOR_SIZE);

    counters[0]++;
    for (uint8_t k = 0; k < VECTOR_SIZE - 1; k++) {
      if (counters[k] >= BASE) {
        counters[k] = 0;
        counters[k + 1]++;
      }
    }
  }
}f    int uniqueCount = 0;
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

    // TODO: Populate a float array of features with the following order:
    // code[0], code[1], code[2], code[3], uniqueCount, isDescending, maxV
    float features[7];
    features[0] = code[0];
    features[1] = code[1];
    features[2] = code[2];
    features[3] = code[3];
    features[4] = uniqueCount;
    features[5] = isDescending;
    features[6] = maxV;

    // Wrap the features for the EON compiler
    signal_t signal;
    numpy::signal_from_buffer(features, 7, &signal);

    // Run inference
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) return;
}

// Button ISR (Required)
void IRAM_ATTR handleButtonPress() {
  Serial.println("yo");
}

// Task B: AI Guess Making (Triggered by Button)
void aiGuessTask(void *pvParameters) {
  populateArray

  prePrune(currentDealerStyle);


  turns = 0;

  while (turns < 10) {
      int guessIdx = getBestGuess();
      uint8_t* guess = possibilities[guessIdx];

      uint8_t result[2];

      

      Serial.printf("Guess: %d %d %d %d\n",
          guess[0], guess[1], guess[2], guess[3]);
      Serial.printf("Right: %d %d %d %d\n",
          actual[0], actual[1], actual[2], actual[3]);
      Serial.printf("Results: %d | %d\n\n",
          result[0], result[1]);

      // solved
      if (result[0] == 4) {
          Serial.printf("I solved it in %d turns! Somethin' light. :)\n", turns);
          delay(1000);
          break;
      }

      // eliminate impossible codes
      prune(guess, result);

      turns++;
    }
}

void populateArray(uint8_t (*array)[VECTOR_SIZE], uint32_t size) {
  uint8_t counters[VECTOR_SIZE] = {0};

  for (uint32_t i = 0; i < size; i++) {
    active[i] = true;
    // Copy current vector
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

void populateArray(uint8_t (*array)[VECTOR_SIZE], uint32_t size) {
  uint8_t counters[VECTOR_SIZE] = {0};

  for (uint32_t i = 0; i < size; i++) {
    active[i] = true;
    // Copy current vector
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

int getBestGuess() {
    int bestGuessIdx = -1;
    int minMaxRemaining = 2000;

    Serial.println("Calculating best guess...");
    uint32_t start = millis();

    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (i == 0) {
            int activeCount = 0;
            for(int k=0; k<ARRAY_SIZE; k++) if(active[k]) activeCount++;
            Serial.printf("Active codes remaining: %d\n", activeCount);
        }
        // Heartbeat print every 200 iterations so you know it hasn't crashed
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
        } // If it's a tie, prefer a code that could actually be the secret
        else if (maxForThisGuess == minMaxRemaining && active[i] && !active[bestGuessIdx]) {
            bestGuessIdx = i;
        }
    }
    
    Serial.printf("\nDone! Took %d ms. Best Index: %d\n", (millis() - start), bestGuessIdx);
    return bestGuessIdx;
}

void prePrune(uint8_t predictedStyle) {
    // TODO: Prune the array of possibilities to remove anything that doesn't match your recognized style
    // Similar to prune, but we're now removing things that don't match the given style
    // Similar logic as classifying the dealer's style from last part
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
    dealer.setup();
    // TODO: Initialize Synchronization Stuff,like Semaphores and Mutexes

    // TODO: Initialize your button with an ISR (you will need to configure an ISR with this RTOS)
    pinMode(BUTTON_PIN, INPUT);    // external hardware debounce

    // attachInterrupt(BUTTON_PIN, handleButtonPress, RISING);
    
    // TODO: Pin Tasks to Cores
    // Core 1: AI and ML (Higher priority for pruning)
    xTaskCreatePinnedToCore(
      mlInferenceTask, 
      "Task2", 
      10000, 
      NULL, 
      1, 
      &Task2, 
      1);
    delay(500);

    xTaskCreatePinnedToCore(
      aiGuessTask, 
      "Task3", 
      10000, 
      NULL, 
      1, 
      &Task3, 
      1);
    delay(500);
    // Core 0: BLE
    xTaskCreatePinnedToCore(
      bleGameplayTask, 
      "Task1", 
      10000, 
      NULL, 
      1, 
      &Task1, 
      0);
    delay(500);

  pixel.begin();
  pixel.show();
}

void loop() {
    // Empty: Everything is managed by FreeRTOS tasks
    vTaskDelete(NULL); 
}
