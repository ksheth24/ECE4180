#include <Arduino.h>

// --- Declare Semaphore and Mutex Stuff ---
QueueHandle_t myQueue;
QueueHandle_t feedbackQueue;       // FIX: Separate queue for feedback (was reusing myQueue for two purposes)
SemaphoreHandle_t btnSem;
SemaphoreHandle_t predictionMutex; // FIX: Added mutex to protect classifierConfidence across tasks
bool press;

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
uint8_t currentGuess[4]; // FIX: Was a raw pointer (uint8_t*), now a safe fixed-size array

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

void prune(uint8_t* lastGuess, const uint8_t* results) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (!active[i]) continue;

        uint8_t reference[2];
        dealer.compare(reference, possibilities[i], lastGuess);

        if (reference[0] != results[0] || reference[1] != results[1]) {
            active[i] = false; 
        } 
    }
}

int getBestGuess() {
    int bestGuessIdx = -1;
    int minMaxRemaining = 2000;

    Serial.println("Calculating best guess...");
    uint32_t start = millis(); // FIX: Was printing millis() as elapsed time, now tracking actual elapsed

    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (i == 0) {
            int activeCount = 0;
            for(int k=0; k<ARRAY_SIZE; k++) if(active[k]) activeCount++;
            Serial.printf("Active codes remaining: %d\n", activeCount);
        }
        if (i % 200 == 0) Serial.print(".");

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
    
    Serial.printf("\nDone! Took %d ms. Best Index: %d\n", (millis() - start), bestGuessIdx);
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

    Serial.printf("Predictions (DSP: %d ms, NN: %d ms)\n", 
                  result.timing.dsp, result.timing.classification);
    
    // FIX: Protect shared classifierConfidence array with mutex, and update currentDealerStyle
    xSemaphoreTake(predictionMutex, portMAX_DELAY);
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.printf("    %s: %.2f\n", result.classification[ix].label,
                                        result.classification[ix].value);
        classifierConfidence[ix] += result.classification[ix].value;
    }
    // FIX: currentDealerStyle was never updated — now we pick the highest confidence class
    int best = 0;
    for (int i = 1; i < 3; i++) {
        if (classifierConfidence[i] > classifierConfidence[best]) best = i;
    }
    // FIX: Correct for swapped model labels (0 and 1 are swapped in the trained model)
    currentDealerStyle = (best == 0 ? 1 : best == 1 ? 0 : 2);
    xSemaphoreGive(predictionMutex);
}

void prePrune(uint8_t predictedStyle) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
      if (!active[i]) continue;

      uint8_t* code = possibilities[i];
      int uniqueCount = 0;
      // FIX: Was using >= (non-strict), should be strict > for descending to match friend's logic
      uint8_t isDescending = code[0] > code[1] && code[1] > code[2] && code[2] > code[3];
      uint8_t maxV = 0;
      bool unique = false;
      for (uint8_t k = 0; k < 4; k++) {
        unique = true;
        if (code[k] > maxV) maxV = code[k];
        for (uint8_t j = k - 1; j >= 0; j--) {
          if (code[j] == code[k]) unique = false;
        }
        if (unique) uniqueCount++;
      }

      // FIX: Labels were swapped vs what the model actually outputs
      // 0 = LOW_NUMBERS (max value <= 2), 1 = DESCENDING, 2 = TWO_PAIRS (exactly 2 unique colors)
      if (predictedStyle == 0) {
          if (maxV > 2) active[i] = false;           // LOW_NUMBERS
      } else if (predictedStyle == 1) {
          if (!isDescending) active[i] = false;       // DESCENDING
      } else if (predictedStyle == 2) {
          if (uniqueCount != 2) active[i] = false;    // TWO_PAIRS
      }
    }
}

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

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    Serial.printf("%s : onRead(), value: %s\n",
                  pCharacteristic->getUUID().toString().c_str(),
                  pCharacteristic->getValue().c_str());
  }

  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string val = pCharacteristic->getValue();
    // FIX: Only process 2-byte feedback writes, ignore anything else
    if (val.length() != 2) return;

    uint8_t fb[2];
    fb[0] = (uint8_t)val[0];
    fb[1] = (uint8_t)val[1];

    Serial.printf("Feedback received! Black: %d, White: %d\n", fb[0], fb[1]);

    // FIX: Queue size is now sizeof(uint8_t[2]) — send to feedbackQueue instead of myQueue
    xQueueSend(feedbackQueue, fb, 0);

    // FIX: prune() now uses the safe currentGuess array instead of a dangling pointer
    prune(currentGuess, fb);

    // FIX: Only Give here (signal AI that feedback arrived) — removed the broken Take/Give pattern
    xSemaphoreGive(btnSem);
  }

  void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
    Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
  }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
    std::string str = "Client ID: ";
    str += connInfo.getConnHandle();
    str += " Address: ";
    str += connInfo.getAddress().toString();
    if      (subValue == 0) str += " Unsubscribed to ";
    else if (subValue == 1) str += " Subscribed to notifications for ";
    else if (subValue == 2) str += " Subscribed to indications for ";
    else if (subValue == 3) str += " Subscribed to notifications and indications for ";
    str += std::string(pCharacteristic->getUUID());
    Serial.printf("%s\n", str.c_str());
  }
} chrCallbacks;

// -------------------------------------------------------------------------
// CORE 1: AI & ML TASKS
// -------------------------------------------------------------------------

TaskHandle_t taskMLHandle;
TaskHandle_t taskAIHandle;
TaskHandle_t taskBLEHandle;

// Task A: ML Inference — triggered when a round is won (feedback[0] == 4)
void mlInferenceTask(void *pvParameters) {
  // FIX: Removed fake warm-up loop that was biasing classifierConfidence with random data
  // before any real game data arrived. Classifier now starts cold and learns from real codes.
  for(;;) {
    uint8_t fb[2];
    // FIX: Receive from feedbackQueue (not myQueue) with correct item size
    if (xQueueReceive(feedbackQueue, fb, portMAX_DELAY)) {
      if (fb[0] == 4) {
        // A round was solved — classify the winning code to learn dealer style
        classifyDealerStyle(currentGuess);
      }
    }
  }
}

// Button ISR
// FIX: Removed delay() from ISR (crashes ESP32), replaced with timestamp debounce
volatile unsigned long lastBtnPress = 0;

void IRAM_ATTR handleButtonPress() {
  unsigned long now = millis();
  if (now - lastBtnPress < 300) return; // debounce
  lastBtnPress = now;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(btnSem, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Task B: AI Guess Making — triggered by button semaphore
// FIX: Removed the broken dual Take of btnSem + aiGuessToBLE which caused deadlock.
// Now btnSem is the single trigger (given by ISR on button press, given by BLE on feedback received).
void aiGuessTask(void *pvParameters) {
  populateArray(possibilities, ARRAY_SIZE);

  // Read predicted style safely under mutex before pruning
  xSemaphoreTake(predictionMutex, portMAX_DELAY);
  uint8_t style = currentDealerStyle;
  xSemaphoreGive(predictionMutex);

  prePrune(style);
  bool hasPruned = true;

  for(;;) {
    // Wait for button press (ISR gives btnSem) or feedback (onWrite gives btnSem)
    if (xSemaphoreTake(btnSem, portMAX_DELAY) == pdTRUE) {
      int guessIdx = getBestGuess();

      // FIX: Copy into safe array instead of storing a raw pointer
      memcpy(currentGuess, possibilities[guessIdx], 4);

      // Signal BLE task to send the guess
      xSemaphoreGive(aiGuessToBLE);
    }
  }
}

// -------------------------------------------------------------------------
// CORE 0: BLE TASK
// -------------------------------------------------------------------------
void bleGameplayTask(void *pvParameters) {
  for(;;) {
    if (xSemaphoreTake(aiGuessToBLE, portMAX_DELAY) == pdTRUE) {
      player.notify();
      // FIX: Send VECTOR_SIZE bytes (4), not sizeof(int) which is platform-dependent
      pSrvChar->setValue((const uint8_t*)currentGuess, VECTOR_SIZE);
      pSrvChar->notify();
      Serial.printf("Sent guess -> [%d %d %d %d]\n",
                    currentGuess[0], currentGuess[1], currentGuess[2], currentGuess[3]);
    }
  }
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);

    NimBLEDevice::init("Player1");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
  
    NimBLEService* pBoardService = pServer->createService("2006");
    pSrvChar = pBoardService->createCharacteristic("0001",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    pSrvChar->setCallbacks(&chrCallbacks);
    pBoardService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName("Mastermind-Bot");
    pAdvertising->addServiceUUID(pBoardService->getUUID());
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();
    Serial.printf("Advertising Started\n");
    
    player.setup();
    dealer.setup();

    // FIX: btnSem is now a binary semaphore used as a signal (not a mutex)
    btnSem         = xSemaphoreCreateBinary();
    aiGuessToBLE   = xSemaphoreCreateBinary();
    predictionMutex = xSemaphoreCreateMutex();

    // FIX: Queue item size is sizeof(uint8_t[2]) = 2 bytes, not sizeof(int) = 4
    myQueue      = xQueueCreate(10, sizeof(uint8_t[2]));
    feedbackQueue = xQueueCreate(8,  sizeof(uint8_t[2]));

    pinMode(BUTTON_PIN, INPUT_PULLUP); // FIX: INPUT_PULLUP avoids floating pin false triggers
    attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING); // FIX: FALLING for INPUT_PULLUP

    xTaskCreatePinnedToCore(mlInferenceTask, "ML Inference",  16384, NULL, 1, &taskMLHandle, 1);
    xTaskCreatePinnedToCore(aiGuessTask,     "AI Guess",      16384, NULL, 1, &taskAIHandle, 1);
    xTaskCreatePinnedToCore(bleGameplayTask, "BLE Send Guess", 16384, NULL, 1, &taskBLEHandle, 0);
}

void loop() {
    vTaskDelete(NULL); 
}
