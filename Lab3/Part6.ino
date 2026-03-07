
/**
 * @file NimBLE_Server.ino
 * @author H2zero
 * @author Trevor Goo
 * @author STUDENT NAME HERE
 * @date 12/27/2025
 * @version 1.0
 *
 * @brief Starter Code for the WiFi portion of ECE4180 Lab 3 (Part 6)
 * specifically ESP32 as an Internet Station
 * @see Course website, Canvas, GitHub or PowerPoint slides for more details
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h> // need to download Arduino_JSON lib
#include <WiFiClientSecure.h>

/*******************************
TODO: Setup and connect to Wifi hotspot. HIGHLY recommend you change from default name
************************************/
//for iPhone ssid: settings -> general -> About to change ssid of hotspot
//for Android: https://www.apple.com/
const char* ssid = "Sheth";
const char* password = "123456789"; 

// World Time API URL
// **************************
// Do not change these three lines! The esp32 will attempt to request data to this link (API key-less) every 5 seconds. You should expect a successful response every one out of five requests.
const char* serverUrl = "https://time.now/developer/api/timezone/America/New_York";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;  // every 5 seconds

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");

  /*******************************
  TODO: Edit the while loop condition to check if you have connected to the Wifi hotspot. EXIT the loop once you have connected. 
  Hint: Check the Wifi.status() and reference the enums!
  ************************************/
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected! localIP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient http;
      http.begin(serverUrl);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

      /*************************
      TODO: change the header to your ESP32 number
      http.addHeader("ESP32-1", "ESP32-WorldTimeLab");
      *************************/
      http.addHeader("ESP32-147", "ESP32-WorldTimeLab");

      /*************************
      TODO: Generate a GET request with the http object. Check http object lib.
      What does the GET request return? 
      https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp
      *************************/
      int httpCode = http.GET();

      /*************************
      TODO: What is a "successful" http code? 
      Change the if condition to check for a succesful code.
      *************************/
      if (httpCode == 200) {
        String payload = http.getString(); //get the payload of the http object
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        Serial.print("Payload: ");
        Serial.println(payload);

        /***********************************
        TODO: Parse through payload via JSON obj. Use/download Aduino_JSON lib to parse through the payload. Extract the date time field. Print out the date and time on seperate lines. 
        **************************************/
        JSONVar obj = JSON.parse(payload);
        if (JSON.typeof(obj) != "undefined") {
          String s = obj["datetime"];
          int i = s.indexOf("T");
          Serial.println(s.substring(0, i));
          Serial.println(s.substring(i + 1));
        } else {
          Serial.println("Failed to parse JSON");
        }
      } else {
        Serial.print("Failed, http repsonse error code: ");
        Serial.println(httpCode);
        String payload = http.getString();
        Serial.println(httpCode);
      }
      http.end();
    } else {
      Serial.println("WiFi disconnected");
    }
    lastTime = millis();
  }
}
