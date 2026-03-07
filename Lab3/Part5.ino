/**
 * @file NimBLE_Server.ino
 * @author H2zero
 * @author Trevor Goo
 * @author Jason Hsiao
 * @author STUDENT NAME HERE
 * @date 12/27/2025
 * @version 1.0
 *
 * @brief Starter Code for the WiFi portion of ECE4180 Lab 3 (Part 5)
 * specifically ESP32 as a Soft Access Point
 * @see Course website, Canvas, GitHub or PowerPoint slides for more details
 */

#include <WiFi.h>

#include <Adafruit_NeoPixel.h>

#define LED_PIN     8     // Data pin connected to onboard RGB LED
#define NUM_LEDS    1     // Only one LED on board

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t pixelColors[6] = {
  pixel.Color(255, 0, 0),    // Red
  pixel.Color(0, 0, 255),    // Blue
  pixel.Color(0, 255, 0),    // Green
  pixel.Color(255, 255, 0),  // Yellow
  pixel.Color(128, 0, 128),  // Purple
  pixel.Color(255, 165, 0)   // Orange
};

/*********************************
TODO: Generate an ssid (ESP32-<your ESP number>)
and password
**********************************/
const char* ssid     = "ESP32-147";
const char* password = "rockyiscool";

//set web server port number to 80
WiFiServer server(80);

//var to store the HTTP request data 
String header;

// output state to store status of HTML and GPIO 
String output7State = "red";

const int output7 = 7;

void setup() {
  Serial.begin(115200);

  pixel.begin();
  pixel.setBrightness(20);

  /*****************************
  1.5 TODO: setup NeoPixel output
  ****************************/
  pixel.show();

  Serial.println("\nSetting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  /**********************************
  2. TODO: Print out the IP address of the ESP32.Read this documentation to get the IP address.

  ESP32 acts as a Wifi AP that constantly broadcasts an html. Stations that connect to the server and visit the correlating IP address can get access to this HTML.

  Note that ESP32 is acting as a SoftAP (Software Access Point). 

  https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html
  **************************************/
  
  Serial.print("AP IP address: ");  

  IPAddress IP = WiFi.softAPIP();// How do you access the IP
  while (!IP) {
    Serial.println("Populate and print out IPAddress");
    delay(5000);
  }
  Serial.println(IP);

  server.begin();
}


void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {   // If a new client connects,
    Serial.println("New Client.");   
    String currentLine = "";  // make a String to hold incoming data from client

    while (client.connected()) {   // loop while the client's connected
      if (client.available()) {    // if there's bytes to read from client
        char c = client.read();    // read a byte, then
        header += c;              //append the byte to header var
        if (c == '\n') {          // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers ALWAYS start with a response code (e.g. HTTP/1.1 200 OK) and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            //make them look at the requests they are recieving 
            // turns the GPIOs on and off
            Serial.println(header);
            /***********************************
            3. TODO: PRINT OUT THE header variable (given at top).
            3.5 TODO: PARSE through the header data and update the output7state variable, and GPIO output.
            *************************************/
            if (header.indexOf("GET /7/red") >= 0) {
              output7State = "red";
              pixel.setPixelColor(0, pixelColors[0]);
              pixel.show();
            } else if (header.indexOf("GET /7/blue") >= 0) {
              output7State = "blue";
              pixel.setPixelColor(0, pixelColors[1]);
              pixel.show();
            } else if (header.indexOf("GET /7/green") >= 0) {
              output7State = "green";
              pixel.setPixelColor(0, pixelColors[2]);
              pixel.show();
            } else if (header.indexOf("GET /7/yellow") >= 0) {
              output7State = "yellow";
              pixel.setPixelColor(0, pixelColors[3]);
              pixel.show();
            } else if (header.indexOf("GET /7/purple") >= 0) {
              output7State = "purple";
              pixel.setPixelColor(0, pixelColors[4]);
              pixel.show();
            } else {
              output7State = "orange";
              pixel.setPixelColor(0, pixelColors[5]);
              pixel.show();
            }
            
            //(THIS IS THE HTML BEING BROADCASTED TO STATION)
            // DISPLAY the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS for on/off buttons 
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            client.println("<h1>" + String(ssid) + "</h1>");
            
            /***********************************
            4. TODO: change "Default Header" to the name of the ssid
            *************************************/

            
            /***********************************
            5. TODO: Make more buttons that correspond to your states
            *************************************/
            client.println("<p>GPIO 7 - State " + output7State + "</p>");
            
            if (output7State=="red") {
              client.println("<p><a href=\"/7/blue\"><button class=\"button\"style=\"background-color: red; color: white; border: none;\">CHANGE</button></a></p>");
            } 
            if (output7State=="blue") {
              client.println("<p><a href=\"/7/green\"><button class=\"button\"style=\"background-color: blue; color: white; border: none;\">CHANGE</button></a></p>");
            }  
            if (output7State=="green") {
              client.println("<p><a href=\"/7/yellow\"><button class=\"button\"style=\"background-color: green; color: white; border: none;\">CHANGE</button></a></p>");
            }  
            if (output7State=="yellow") {
              client.println("<p><a href=\"/7/purple\"><button class=\"button\"style=\"background-color: yellow; color: white; border: none;\">CHANGE</button></a></p>");
            }  
            if (output7State=="purple") {
              client.println("<p><a href=\"/7/orange\"><button class=\"button\"style=\"background-color: purple; color: white; border: none;\">CHANGE</button></a></p>");
            }  
            if (output7State=="orange") {
              client.println("<p><a href=\"/7/red\"><button class=\"button\"style=\"background-color: orange; color: white; border: none;\">CHANGE</button></a></p>");
            }  
            

            

            // Put the rest of your buttons here!

            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";

    // Close connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
