/**
 * @file NimBLE_Client.ino
 * @author H2zero
 * @author Jason Hsiao
 * @author STUDENT NAME HERE
 * @date 12/27/2025
 * @version 1.0
 *
 * @brief Starter Code for the non-switching BLE Client portion of ECE4180 Lab 3
 * (Part 4)
 * @see Course website, Canvas, GitHub or PowerPoint slides for more details
 */

#include <Arduino.h>
#include <NimBLEDevice.h>

#define CODEMAKER
#include <ECE4180MasterMind.h>

CodeMaker host;

//============================================//
// TODO: Set-up UUIDs and Mastermind objects  //
//============================================//
#define DEVICE_NAME "Satvik_is_hot"     // Replace this with something unique or else everyone in lab will be annoyed with you
#define SERVICE_UUID "2006"             // Same goes for this
#define CHAR_UUID "0001"                // This one doesn't matter as much, as long as the Service and Device name are unique

bool isConnected = false;
bool waitingForFeedback = false; // Is it the Dealer's turn to type?


static const NimBLEAdvertisedDevice* advDevice;
static bool                          doConnect  = false;
static uint32_t                      scanTimeMs = 5000; /** scan time in milliseconds, 0 = scan forever */

/** Now we can read/write/subscribe the characteristics of the services we are interested in */
NimBLERemoteService*        pSvc = nullptr;
NimBLERemoteCharacteristic* pChr = nullptr;
NimBLERemoteDescriptor*     pDsc = nullptr;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override { Serial.printf("Connected\n"); }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }

} clientCallbacks;

/** Define a class to handle the callbacks when scan events are received */
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
            Serial.printf("Found Our Service\n");
            /** stop scan before connecting */
            NimBLEDevice::getScan()->stop();
            /** Save the device reference in a global for the client to use*/
            advDevice = advertisedDevice;
            /** Ready to connect now */
            doConnect = true;
        }
    }

    /** Callback to process the results of the completed scan or restart it */
    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.printf("Scan Ended, reason: %d, device count: %d; Restarting scan\n", reason, results.getCount());
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
} scanCallbacks;

/** Notification / Indication receiving handler callback */
void handleMove(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    // std::string str  = (isNotify == true) ? "Notification" : "Indication";
    // str             += " from ";
    // str             += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    // str             += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    // str             += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    // str             += ", Value = " + std::string((char*)pData, length);
    // Serial.printf("%s\n", str.c_str());
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
    if (pChr != nullptr) {
        pChr->writeValue(results, 2);
    }
    if (results[0] == 4) {
        host.endGame(); // Don't worry about this for the main BLE parts, you'll implement this for extra credit
    }
    //======================================//
    // TODO: HANDLE MOVE FROM DEALER HERE   //
    //======================================//
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer() {
    NimBLEClient* pClient = nullptr;

    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getCreatedClientCount()) {
        /**
         *  Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient) {
            if (!pClient->connect(advDevice, false)) {
                Serial.printf("Reconnect failed\n");
                return false;
            }
            Serial.printf("Reconnected client\n");
        } else {
            /**
             *  We don't already have a client that knows this device,
             *  check for a client that is disconnected that we can use.
             */
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    /** No client to reuse? Create a new one. */
    if (!pClient) {
        if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
            Serial.printf("Max clients reached - no more connections available\n");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.printf("New client created\n");

        pClient->setClientCallbacks(&clientCallbacks, false);
        /**
         *  Set initial connection parameters:
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 150 * 10ms = 1500ms timeout
         */
        pClient->setConnectionParams(12, 12, 0, 150);

        /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
        pClient->setConnectTimeout(5 * 1000);

        if (!pClient->connect(advDevice)) {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            Serial.printf("Failed to connect, deleted client\n");
            return false;
        }
    }

    if (!pClient->isConnected()) {
        if (!pClient->connect(advDevice)) {
            Serial.printf("Failed to connect\n");
            return false;
        }
    }

    Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

    

    pSvc = pClient->getService(SERVICE_UUID);
    if (pSvc) {
        pChr = pSvc->getCharacteristic(CHAR_UUID);
    }

    if (pChr) {
        //======================================//
        //   TODO: Subscribe for indications    //
        //======================================//
        if(pChr->canNotify()) {
            pChr->subscribe(true, handleMove);
            Serial.println("Subscribed to notifications");
        }
    } else {
        Serial.printf("4180 service not found.\n");
    }

    Serial.printf("Done with this device!\n");
    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.printf("Starting NimBLE Client\n");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init(DEVICE_NAME);

    /** Optional: set the transmit power */
    NimBLEDevice::setPower(3); /** 3dbm */
    NimBLEScan* pScan = NimBLEDevice::getScan();

    /** Set the callbacks to call when scan events occur, no duplicates */
    pScan->setScanCallbacks(&scanCallbacks, false);

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(100);
    pScan->setWindow(100);

    /**
     * Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pScan->setActiveScan(true);

    /** Start scanning for advertisers */
    pScan->start(scanTimeMs);
    Serial.printf("Scanning for peripherals\n");
    
    //======================================//
    // TODO: Set-up your code-maker here    //
    //======================================//
    host.setup();
    host.generateBiasedCode(static_cast<uint>(random(0,3)));    // host sets up the code
  

}

void loop() {
    /** Loop here until we find a device we want to connect to */
    delay(10);

    if (doConnect) {
        doConnect = false;
        /** Found a device we want to connect to, do it now */
        if (connectToServer()) {
            Serial.printf("Success! we should now be getting notifications, scanning for more!\n");
        } else {
            Serial.printf("Failed to connect, starting scan\n");
            NimBLEDevice::getScan()->start(scanTimeMs, false, true);
        }
    }

    
}
