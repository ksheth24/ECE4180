#include <WiFi.h>
#include <esp_now.h>

#define buttonUp 6
#define buttonDown 10
#define buttonRight 11
#define buttonLeft 21
#define buttonCenter 7

const char* message = "Idle";
bool changed = false;

uint8_t receiverMAC[] = {0x10, 0x51, 0xDB, 0x00, 0x37, 0xD4};

void IRAM_ATTR navISR(void* arg) {
  int pin = (int)(intptr_t)arg;
  changed = true;
  if (pin == buttonUp) {
    message = "Up";
  } else if (pin == buttonDown) {
    message = "Down";
  } else if (pin == buttonLeft) {
    message = "Left";
  } else {
    message = "Right";
  }
}

void IRAM_ATTR submit() {
  changed = true;
  message = "Center";
  Serial.println("Submitted");
}

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);

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
}

void loop() {
  if (!changed) {
    message = "Idle";
  }
  esp_now_send(receiverMAC, (uint8_t *)message, strlen(message) + 1);
  Serial.print("Sent: ");
  Serial.println(message);
  changed = false;
  delay(200);
}
