/**
 * @file main.cpp
 * @brief ECE4180 Lab 3 - FreeRTOS + BLE Mastermind Server (AI Codebreaker)
 *
 * CORE 0: bleGameplayTask  — keeps BLE alive, monitors connection health
 * CORE 1: aiGuessTask      — runs Knuth minimax, sends guesses over BLE
 *          mlInferenceTask — classifies dealer style in background, updates prePrune bias
 *
 * SYNCHRONIZATION:
 *   btnSemaphore   — binary, given by ISR, taken by aiGuessTask to start a game
 *   connectedSem   — binary, given by onConnect, taken by aiGuessTask to gate BLE send
 *   feedbackQueue  — queue of uint8_t[2], written by onWrite, read by aiGuessTask
 *   codeQueue      — queue of uint8_t[4], written by aiGuessTask on win, read by mlInferenceTask
 *   styleMutex     — guards currentDealerStyle read/write across cores
 */

#include <Arduino.h>

// -------------------------------------------------------------------------
// FreeRTOS
// -------------------------------------------------------------------------
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

TaskHandle_t hBleTask;
TaskHandle_t hAiTask;
TaskHandle_t hMlTask;

// -------------------------------------------------------------------------
// NeoPixel (optional status LED)
// -------------------------------------------------------------------------
#include <Adafruit_NeoPixel.h>
#define LED_PIN    8
#define NUM_PIXELS 1
Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// -------------------------------------------------------------------------
// Mastermind
// -------------------------------------------------------------------------
#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeMaker dealer;   // used by server to score guesses (local copy / BLE receive)
CodeBreaker player; // not used for logic here, kept for library compatibility

// -------------------------------------------------------------------------
// BLE
// -------------------------------------------------------------------------
#include <NimBLEDevice.h>

static NimBLEServer*         pServer         = nullptr;
static NimBLECharacteristic* pCharacteristic = nullptr;

// -------------------------------------------------------------------------
// Button
// -------------------------------------------------------------------------
#define BUTTON_PIN 7

// -------------------------------------------------------------------------
// AI / ML globals
// -------------------------------------------------------------------------
uint8_t  possibilities[ARRAY_SIZE][VECTOR_SIZE];
bool     active[ARRAY_SIZE];

uint8_t  turns        = 0;
bool     gameRunning  = false;

// Classifier confidence: index 0=descending, 1=low_numbers, 2=two_pairs
float    classifierConfidence[3] = {0.0f, 0.0f, 0.0f};
uint8_t  currentDealerStyle     = 255; // 255 = unknown (no bias applied)

// -------------------------------------------------------------------------
// Synchronization primitives
// -------------------------------------------------------------------------
static SemaphoreHandle_t btnSemaphore  = nullptr; // ISR  → aiGuessTask
static SemaphoreHandle_t connectedSem  = nullptr; // BLE connect → aiGuessTask
static SemaphoreHandle_t styleMutex    = nullptr; // guards currentDealerStyle

// feedbackQueue: onWrite pushes uint8_t[2] {black, white}; aiGuessTask pops
static QueueHandle_t     feedbackQueue = nullptr;

// codeQueue: aiGuessTask pushes winning code uint8_t[4]; mlInferenceTask pops
static QueueHandle_t     codeQueue     = nullptr;

// =========================================================================
// HELPER: populateArray
//   Fills possibilities[][] with every BASE^VECTOR_SIZE combination
// =========================================================================
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

// =========================================================================
// HELPER: prePrune
//   Removes possibilities that don't match the predicted dealer style.
//   style 0 = descending, style 1 = low numbers (max <= 2), style 2 = two_pairs
//   style 255 = unknown, skip pruning
// =========================================================================
void prePrune(uint8_t predictedStyle) {
    if (predictedStyle == 255) return; // no bias yet

    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (!active[i]) continue;

        uint8_t* c = possibilities[i];

        int uniqueCount = 0;
        int maxV        = 0;
        bool isDescending = (c[0] >= c[1] && c[1] >= c[2] && c[2] >= c[3]);

        for (int k = 0; k < 4; k++) {
            if (c[k] > maxV) maxV = c[k];
            bool unique = true;
            for (int j = k - 1; j >= 0; j--) {
                if (c[j] == c[k]) { unique = false; break; }
            }
            if (unique) uniqueCount++;
        }

        if      (predictedStyle == 0 && !isDescending)   active[i] = false;
        else if (predictedStyle == 1 && maxV > 2)         active[i] = false;
        else if (predictedStyle == 2 && uniqueCount != 2) active[i] = false;
    }
}

// =========================================================================
// HELPER: prune
//   Eliminates codes inconsistent with the last guess + feedback.
// =========================================================================
void prune(uint8_t* lastGuess, uint8_t* results) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (!active[i]) continue;

        uint8_t reference[2];
        dealer.compare(reference, possibilities[i], lastGuess);

        if (reference[0] != results[0] || reference[1] != results[1]) {
            active[i] = false;
        }
    }
}

// =========================================================================
// HELPER: getBestGuess
//   Knuth minimax — picks the guess that minimises worst-case remaining set.
// =========================================================================
int getBestGuess() {
    int bestIdx          = -1;
    int minMaxRemaining  = 0x7FFFFFFF;

    int activeCount = 0;
    for (int k = 0; k < ARRAY_SIZE; k++) if (active[k]) activeCount++;
    Serial.printf("[AI] Active codes: %d  Running minimax...\n", activeCount);

    uint32_t t0 = millis();

    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (i % 200 == 0) Serial.print(".");

        int scoreCount[5][5] = {};

        for (int j = 0; j < ARRAY_SIZE; j++) {
            if (!active[j]) continue;
            uint8_t check[2];
            dealer.compare(check, possibilities[i], possibilities[j]);
            scoreCount[check[0]][check[1]]++;
        }

        int maxForThis = 0;
        for (int b = 0; b <= 4; b++)
            for (int w = 0; w <= 4; w++)
                if (scoreCount[b][w] > maxForThis)
                    maxForThis = scoreCount[b][w];

        if (maxForThis < minMaxRemaining) {
            minMaxRemaining = maxForThis;
            bestIdx         = i;
        } else if (maxForThis == minMaxRemaining && active[i] && !active[bestIdx]) {
            bestIdx = i; // prefer a still-possible guess on ties
        }
    }

    Serial.printf("\n[AI] Minimax done in %d ms. Best idx: %d\n",
                  (int)(millis() - t0), bestIdx);
    return bestIdx;
}

// =========================================================================
// BLE SERVER CALLBACKS
// =========================================================================
class ServerCallbacks : public NimBLEServerCallbacks {

    void onConnect(NimBLEServer* pSrv, NimBLEConnInfo& connInfo) override {
        Serial.printf("[BLE] Client connected: %s\n",
                      connInfo.getAddress().toString().c_str());
        pSrv->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);

        // Unblock aiGuessTask — BLE is ready
        xSemaphoreGive(connectedSem);
    }

    void onDisconnect(NimBLEServer* pSrv, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("[BLE] Client disconnected (reason %d) — restarting advertising\n",
                      reason);

        // Drain connectedSem so aiGuessTask blocks again until next connection
        xSemaphoreTake(connectedSem, 0);

        NimBLEDevice::startAdvertising();
    }

} serverCallbacks;

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {

    void onRead(NimBLECharacteristic* pChr, NimBLEConnInfo& connInfo) override {
        Serial.printf("[BLE] onRead: %s\n", pChr->getUUID().toString().c_str());
    }

    // Client (CodeMaker) writes back {black, white} feedback here
    void onWrite(NimBLECharacteristic* pChr, NimBLEConnInfo& connInfo) override {
        if (pChr->getValue().length() < 2) {
            Serial.println("[BLE] onWrite: short payload, ignoring");
            return;
        }

        uint8_t results[2];
        results[0] = pChr->getValue()[0]; // black pegs
        results[1] = pChr->getValue()[1]; // white pegs

        Serial.printf("[BLE] Feedback received — Black: %d  White: %d\n",
                      results[0], results[1]);

        // Push to feedbackQueue (aiGuessTask is blocking on xQueueReceive)
        // Use portMAX_DELAY so we never silently drop feedback
        xQueueSend(feedbackQueue, results, portMAX_DELAY);
    }

    void onStatus(NimBLECharacteristic* pChr, int code) override {
        Serial.printf("[BLE] Notify status: %d (%s)\n",
                      code, NimBLEUtils::returnCodeToString(code));
    }

    void onSubscribe(NimBLECharacteristic* pChr,
                     NimBLEConnInfo&       connInfo,
                     uint16_t              subValue) override {
        Serial.printf("[BLE] Client %s subscribed (subValue=%d)\n",
                      connInfo.getAddress().toString().c_str(), subValue);
    }

} chrCallbacks;

// =========================================================================
// BUTTON ISR
//   Runs on IRAM. Gives btnSemaphore — never touches Serial or heap.
// =========================================================================
void IRAM_ATTR handleButtonPress() {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(btnSemaphore, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken); // yield immediately if needed
}

// =========================================================================
// TASK A: mlInferenceTask  (Core 1, lower priority than AI)
//   Blocks on codeQueue. Each time aiGuessTask solves a game it enqueues the
//   winning code. This task classifies it and updates currentDealerStyle.
// =========================================================================
void mlInferenceTask(void* pvParameters) {
    Serial.println("[ML] Inference task started");

    uint8_t wonCode[VECTOR_SIZE];

    while (true) {
        // Block until a solved code arrives
        if (xQueueReceive(codeQueue, wonCode, portMAX_DELAY) != pdTRUE) continue;

        Serial.printf("[ML] Running inference on code: %d %d %d %d\n",
                      wonCode[0], wonCode[1], wonCode[2], wonCode[3]);

        // --- Feature extraction ---
        int  uniqueCount  = 0;
        int  maxV         = 0;
        bool isDescending = (wonCode[0] >= wonCode[1] &&
                             wonCode[1] >= wonCode[2] &&
                             wonCode[2] >= wonCode[3]);

        for (int i = 0; i < 4; i++) {
            if (wonCode[i] > maxV) maxV = wonCode[i];
            bool unique = true;
            for (int j = i - 1; j >= 0; j--) {
                if (wonCode[j] == wonCode[i]) { unique = false; break; }
            }
            if (unique) uniqueCount++;
        }

        float features[7] = {
            (float)wonCode[0],
            (float)wonCode[1],
            (float)wonCode[2],
            (float)wonCode[3],
            (float)uniqueCount,
            (float)isDescending,
            (float)maxV
        };

        // --- EON / Edge Impulse inference ---
        signal_t signal;
        numpy::signal_from_buffer(features, 7, &signal);

        ei_impulse_result_t result = {0};
        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

        if (res != EI_IMPULSE_OK) {
            Serial.printf("[ML] Classifier error: %d\n", res);
            continue;
        }

        // --- Update shared confidence + style under mutex ---
        xSemaphoreTake(styleMutex, portMAX_DELAY);

        // Model label order may differ — adjust indices to match your trained model
        // Assumed: 0=descending, 1=low_numbers, 2=two_pairs
        float bestConf  = -1.0f;
        uint8_t bestIdx = 255;

        for (uint8_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            classifierConfidence[i] = result.classification[i].value;
            Serial.printf("[ML] Class %s: %.3f\n",
                          result.classification[i].label,
                          result.classification[i].value);
            if (result.classification[i].value > bestConf) {
                bestConf = result.classification[i].value;
                bestIdx  = i;
            }
        }

        currentDealerStyle = bestIdx;
        Serial.printf("[ML] New dealer style prediction: %d (conf=%.3f)\n",
                      currentDealerStyle, bestConf);

        xSemaphoreGive(styleMutex);
    }
}

// =========================================================================
// TASK B: aiGuessTask  (Core 1, higher priority)
//   Waits for button press, then runs one full game:
//     1. Populate + prePrune with latest style
//     2. Pick best guess (Knuth minimax)
//     3. Send over BLE via notify
//     4. Block on feedbackQueue for client response
//     5. Prune, repeat until solved or 10 turns
//     6. Enqueue winning code for mlInferenceTask
// =========================================================================
void aiGuessTask(void* pvParameters) {
    Serial.println("[AI] Guess task started — press button to begin");

    while (true) {
        // ── Wait for button press ──────────────────────────────────────────
        xSemaphoreTake(btnSemaphore, portMAX_DELAY);
        Serial.println("[AI] Button pressed — starting game");

        // ── Wait for BLE connection (non-destructive peek) ─────────────────
        // Take + immediately give back so the semaphore stays "given"
        // for subsequent iterations while client remains connected.
        xSemaphoreTake(connectedSem, portMAX_DELAY);
        xSemaphoreGive(connectedSem);

        // ── Drain any stale feedback from a previous game ──────────────────
        uint8_t stale[2];
        while (xQueueReceive(feedbackQueue, stale, 0) == pdTRUE) {}

        // ── Snapshot style under mutex ─────────────────────────────────────
        xSemaphoreTake(styleMutex, portMAX_DELAY);
        uint8_t styleSnapshot = currentDealerStyle;
        xSemaphoreGive(styleMutex);

        // ── Initialize search space ────────────────────────────────────────
        populateArray(possibilities, ARRAY_SIZE);
        prePrune(styleSnapshot);

        Serial.printf("[AI] Style bias: %d  Starting search...\n", styleSnapshot);

        bool solved = false;
        turns = 0;

        while (turns < 10) {
            int guessIdx = getBestGuess();
            if (guessIdx < 0) {
                Serial.println("[AI] No valid guess — search space exhausted");
                break;
            }

            uint8_t* guess = possibilities[guessIdx];

            Serial.printf("[AI] Turn %d — Guess: %d %d %d %d\n",
                          turns + 1, guess[0], guess[1], guess[2], guess[3]);

            // ── Send guess over BLE ────────────────────────────────────────
            pCharacteristic->setValue((const uint8_t*)guess, VECTOR_SIZE);
            pCharacteristic->notify();

            // ── Block for feedback ─────────────────────────────────────────
            // 30-second timeout guards against a disconnected client
            uint8_t result[2];
            if (xQueueReceive(feedbackQueue, result, pdMS_TO_TICKS(30000)) != pdTRUE) {
                Serial.println("[AI] Feedback timeout — aborting game");
                break;
            }

            Serial.printf("[AI] Feedback — Black: %d  White: %d\n",
                          result[0], result[1]);

            if (result[0] == VECTOR_SIZE) {
                Serial.printf("[AI] *** Solved in %d turn(s)! ***\n", turns + 1);
                solved = true;

                // Send winning code to ML task for style update
                xQueueSend(codeQueue, guess, 0); // non-blocking; drop if full
                break;
            }

            // Prune and continue
            prune(guess, result);
            turns++;
        }

        if (!solved) {
            Serial.println("[AI] Failed to solve within turn limit.");
        }

        Serial.println("[AI] Game over. Press button to play again.");
    }
}

// =========================================================================
// TASK C: bleGameplayTask  (Core 0)
//   Keeps BLE stack alive and monitors connection health.
//   All actual BLE send/receive is driven by callbacks + aiGuessTask.
// =========================================================================
void bleGameplayTask(void* pvParameters) {
    Serial.println("[BLE] BLE task started");

    while (true) {
        // Check connection health every 5 s
        vTaskDelay(pdMS_TO_TICKS(5000));

        if (pServer->getConnectedCount() == 0) {
            // Not connected — make sure advertising is running
            if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
                Serial.println("[BLE] Not advertising — restarting");
                NimBLEDevice::startAdvertising();
            }
        }
    }
}

// =========================================================================
// SETUP
// =========================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("[SETUP] Booting...");

    // ── NeoPixel ───────────────────────────────────────────────────────────
    pixel.begin();
    pixel.setPixelColor(0, pixel.Color(0, 0, 32)); // dim blue = booting
    pixel.show();

    // ── Mastermind objects ─────────────────────────────────────────────────
    player.setup();
    dealer.setup();

    // ── Synchronization primitives ─────────────────────────────────────────
    btnSemaphore  = xSemaphoreCreateBinary();
    connectedSem  = xSemaphoreCreateBinary();
    styleMutex    = xSemaphoreCreateMutex();
    feedbackQueue = xQueueCreate(8,  sizeof(uint8_t[2]));
    codeQueue     = xQueueCreate(4,  sizeof(uint8_t[VECTOR_SIZE]));

    configASSERT(btnSemaphore);
    configASSERT(connectedSem);
    configASSERT(styleMutex);
    configASSERT(feedbackQueue);
    configASSERT(codeQueue);

    // ── Button + ISR ───────────────────────────────────────────────────────
    pinMode(BUTTON_PIN, INPUT); // assumes external hardware debounce
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                    handleButtonPress, RISING);

    // ── BLE setup ──────────────────────────────────────────────────────────
    NimBLEDevice::init("Mastermind-Server");

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    NimBLEService* pService = pServer->createService("2006");

    pCharacteristic = pService->createCharacteristic(
        "0001",
        NIMBLE_PROPERTY::READ  |
        NIMBLE_PROPERTY::WRITE |   // client writes feedback here
        NIMBLE_PROPERTY::NOTIFY    // server notifies guesses here
    );
    pCharacteristic->setCallbacks(&chrCallbacks);

    pService->start();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->setName("Mastermind-Player1"); // must match client DEVICE_NAME scan target
    pAdv->addServiceUUID(pService->getUUID());
    pAdv->enableScanResponse(true);
    pAdv->start();

    Serial.println("[SETUP] BLE advertising started");

    // ── Pin tasks to cores ──────────────────────────────────────────────────
    //
    // Core 1: ML inference (priority 1) + AI guess (priority 2)
    //   AI is higher priority so minimax isn't starved by background inference.
    //
    // Core 0: BLE keepalive (priority 1)
    //   NimBLE's own tasks also run on Core 0.

    xTaskCreatePinnedToCore(
        mlInferenceTask,
        "ML_Inference",
        10000,      // stack in words
        NULL,
        1,          // priority
        &hMlTask,
        1);         // Core 1
    delay(200);

    xTaskCreatePinnedToCore(
        aiGuessTask,
        "AI_Guess",
        10000,
        NULL,
        2,          // higher priority than ML
        &hAiTask,
        1);         // Core 1
    delay(200);

    xTaskCreatePinnedToCore(
        bleGameplayTask,
        "BLE_Task",
        4096,
        NULL,
        1,
        &hBleTask,
        0);         // Core 0
    delay(200);

    pixel.setPixelColor(0, pixel.Color(0, 32, 0)); // dim green = ready
    pixel.show();

    Serial.println("[SETUP] All tasks launched. Press button to start a game.");
}

// =========================================================================
// LOOP — intentionally empty; FreeRTOS owns execution from here
// =========================================================================
void loop() {
    vTaskDelete(NULL);
}
