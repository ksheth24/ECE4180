/**
 * @file MasterMindGameplay.ino
 * @author Jason Hsiao
 * @author STUDENT NAME HERE
 * @date 12/27/2025
 * @version 1.0
 *
 * @brief Starter Code for the ISR portion (Part 1) of ECE4180 Lab 3 and 
 * example of game flow for BLE implementation using helper library
 *
 * @see Course website, Canvas, GitHub or PowerPoint slides for more details
 */

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#define LED_PIN     8     // Data pin connected to onboard RGB LED
#define NUM_LEDS    1     // Only one LED on board
#define buttonUp 6
#define buttonDown 10
#define buttonRight 11
#define buttonLeft 21
#define buttonCenter 7
#define TX 16
#define RX 17

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t pixelColors[6] = {
  pixel.Color(255, 0, 0),    // Red
  pixel.Color(0, 0, 255),    // Blue
  pixel.Color(0, 255, 0),    // Green
  pixel.Color(255, 255, 0),  // Yellow
  pixel.Color(128, 0, 128),  // Purple
  pixel.Color(255, 165, 0)   // Orange
};

#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeMaker host;
CodeBreaker player;

int color1 = 0;
int color2 = 0;
int color3 = 0;
int color4 = 0;
int currentColor = 0;

//============================================//
// TODO: Define globals like buttonStatus     //
// or timestamps for debouncing               //
//============================================//

void IRAM_ATTR navISR(void* arg) {
  int pin = (int)arg;
  if (pin == buttonUp) {
    color1 = (color1 + 1) % 6;
    currentColor = color1;
  } else if (pin == buttonDown) {
    color2 = (color2 + 1) % 6;
    currentColor = color2;
  } else if (pin == buttonLeft) {
    color3 = (color3 + 1) % 6;
    currentColor = color3;
  } else {
    color4 = (color4 + 1) % 6;
    currentColor = color4;
  }
  pixel.setPixelColor(0, pixelColors[currentColor]);
  // pixel.show();
  player.move.playerGuess[0] = color1;
  player.move.playerGuess[1] = color2;
  player.move.playerGuess[2] = color3;
  player.move.playerGuess[3] = color4;
  player.printGuess();
}

void IRAM_ATTR submit() {
  Serial.println("Submitted");

  player.notify();
  host.move_callBack();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting stuff");

  player.setup();
  host.setup();

  // Dealer generates secret code and lets player know when to start
  host.generateCode();    // host sets up the code
  host.throwResultsFlag(true);   // Let Player know that they can do something 
  
  //============================================//
  // TODO: Set-up your interrupts               //
  //============================================//
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

  pixel.begin();
  pixel.setBrightness(20);
}

void loop() {
  // if (player.move.turn){
  //   Serial.println("Player can go now");
  // } else {
  //   Serial.println("Game has been reset");
  // }

  //============================================//
  // TODO: Comment out and replace this line    //
  // with ISR-based button handler              //
  // while (!player.makeGuess());
  //============================================//

  pixel.show();
  delay(100);
}

//============================================//
// TODO: Define and handle your interrupts    //
//============================================//

// Mimic the implementation of throwing the results flag
bool CodeMaker::throwResultsFlag(bool ongoing){
  player.move.turn = ongoing;
  return true;
}

// Read Player's move and do something
void CodeMaker::move_callBack(){
  // Make a local copy (which is the same when you use BLE)
  uint8_t playersGuess[4];
  for (size_t i = 0; i < 4; ++i) {
    playersGuess[i] = player.move.playerGuess[i];  // Make a non-volatile copy
  }

  // Fill buffer with results from players guess
  uint8_t results[2];
  host.checkGuess(results, playersGuess);
  host.printCode(); // Print actual code for reference
  // Serial.printf("Results: %d | %d \n", results[0], results[1]);
  
  if (results[0] == 4) {
    host.endGame(); // Don't worry about this for the main BLE parts, you'll implement this for extra credit
    host.throwResultsFlag(false);
  } else {
    host.throwResultsFlag(true);
  }
}
