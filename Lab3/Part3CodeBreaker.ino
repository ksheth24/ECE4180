#include <Arduino.h>
#include <NimBLEDevice.h>

#define CODEMAKER
#include <ECE4180MasterMind.h>

CodeMaker host;

// UUIDs must match server
#define DEVICE_NAME "Satvik_is_hot"
#define SERVICE_UUID "2006"
#define CHAR_UUID "0001"

bool isConnected = false;

static const NimBLEAdvertisedDevice* advDevice;
static bool doConnect = false;
static uint32_t scanTimeMs = 5000;

NimBLERemoteService* pSvc = nullptr;
NimBLERemoteCharacteristic* pChr = nullptr;

/* ================= CLIENT CALLBACKS ================= */

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        Serial.println("Connected");
        isConnected = true;
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        Serial.printf("Disconnected, reason = %d\n", reason);
        isConnected = false;
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
} clientCallbacks;

/* ================= SCAN CALLBACKS ================= */

class ScanCallbacks : public NimBLEScanCallbacks {

    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {

        Serial.printf("Advertised Device found: %s\n",
                      advertisedDevice->toString().c_str());

        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {

            Serial.println("Found Our Service");

            NimBLEDevice::getScan()->stop();

            advDevice = advertisedDevice;
            doConnect = true;
        }
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.printf("Scan Ended, restarting...\n");
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
} scanCallbacks;

/* ================= NOTIFICATION HANDLER ================= */

void handleMove(
    NimBLERemoteCharacteristic* pRemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify
) {

    Serial.println("Notification received");

    uint8_t playersGuess[4];

    for (int i = 0; i < 4; i++) {
        playersGuess[i] = pData[i];
    }

    Serial.printf("Guess: %d %d %d %d\n",
                  playersGuess[0],
                  playersGuess[1],
                  playersGuess[2],
                  playersGuess[3]);

    uint8_t results[2];

    host.checkGuess(results, playersGuess);

    Serial.printf("Results -> Black: %d White: %d\n",
                  results[0], results[1]);

    pChr->writeValue(results);

    pChr->canNotify();
}

/* ================= CONNECTION FUNCTION ================= */

bool connectToServer() {

    NimBLEClient* pClient = nullptr;

    if (NimBLEDevice::getCreatedClientCount()) {

        pClient = NimBLEDevice::getClientByPeerAddress(
            advDevice->getAddress());

        if (pClient) {

            if (!pClient->connect(advDevice, false)) {
                Serial.println("Reconnect failed");
                return false;
            }

            Serial.println("Reconnected client");
        }
        else {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    if (!pClient) {

        if (NimBLEDevice::getCreatedClientCount()
            >= NIMBLE_MAX_CONNECTIONS) {

            Serial.println("Max clients reached");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.println("New client created");

        pClient->setClientCallbacks(&clientCallbacks, false);

        pClient->setConnectionParams(12, 12, 0, 150);
        pClient->setConnectTimeout(5000);

        if (!pClient->connect(advDevice)) {

            NimBLEDevice::deleteClient(pClient);

            Serial.println("Failed to connect");

            return false;
        }
    }

    if (!pClient->isConnected()) {

        if (!pClient->connect(advDevice)) {
            Serial.println("Failed to connect");
            return false;
        }
    }

    Serial.printf("Connected to: %s RSSI: %d\n",
        pClient->getPeerAddress().toString().c_str(),
        pClient->getRssi());

    pSvc = pClient->getService(SERVICE_UUID);

    if (pSvc) {
        pChr = pSvc->getCharacteristic(CHAR_UUID);
    }

    if (pChr) {

        /* Subscribe to server notifications */

        pChr->subscribe(true, handleMove);

        Serial.println("Subscribed to notifications");
    }
    else {

        Serial.println("Characteristic not found");
        return false;
    }

    Serial.println("Ready!");

    return true;
}

/* ================= SETUP ================= */

void setup() {

    Serial.begin(115200);
    Serial.println("Starting NimBLE Client");

    NimBLEDevice::init(DEVICE_NAME);

    NimBLEDevice::setPower(3);

    NimBLEScan* pScan = NimBLEDevice::getScan();

    pScan->setScanCallbacks(&scanCallbacks, false);

    pScan->setInterval(100);
    pScan->setWindow(100);

    pScan->setActiveScan(true);

    pScan->start(scanTimeMs);

    Serial.println("Scanning...");

    /* Mastermind setup */

    host.setup();
    host.generateCode();
}

/* ================= LOOP ================= */

void loop() {

    delay(10);

    if (doConnect && !isConnected) {

        doConnect = false;

        if (connectToServer()) {

            Serial.println(
                "Success! Waiting for guesses...");
        }
        else {

            Serial.println("Connection failed");

            NimBLEDevice::getScan()->start(
                scanTimeMs, false, true);
        }
    }
}
