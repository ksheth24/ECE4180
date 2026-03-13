/**
 * @file MasterMindISR.ino
 * @author Jason Hsiao
 * @date 2/15/2025
 * @version 1.0
 *
 * @brief Solution Code for the ISR portion (Part 1) of ECE4180 Lab 3
 * @see Course website, Canvas, GitHub or PowerPoint slides for more details
 */

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#define LED_PIN     8     // Data pin connected to onboard RGB LED
#define NUM_LEDS    1     // Only one LED on board

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define NR_OF_LEDS     8 * 4
#define NR_OF_ALL_BITS 24 * NR_OF_LEDS

rmt_data_t led_data[NR_OF_ALL_BITS];

int led_index = 0;
int color_index = 0;

// I set the hex values a little lower because the brightness hurts my eyes lol
int green[3] = {0x1F, 0x00, 0x00};  // Green Red Blue values
int red[3] = {0x00, 0x1F, 0x00};
int blue[3] = {0x00, 0x00, 0x1F};
int yellow[3] = {0x1F, 0x1F, 0x00};
int purple[3] = {0x00, 0x1F, 0x1F};
int orange[3] = {0x15, 0x1F, 0x00};   // Orange
int* colors[6] = {green, red, blue, yellow, purple, orange}; 

int color1 = 0;
int color2 = 0;
int color3 = 0;
int color4 = 0;


#define pixel_pin 0

#define buttonUp 6
#define buttonDown 10
#define buttonRight 11
#define buttonLeft 21
#define buttonCenter 7

#define OUTPUT_PIN 22

#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeMaker host;
CodeBreaker player;

bool enterPressed;

long last_press;

void IRAM_ATTR navISR(void* arg) {
  int pin = (int)arg;
  if (pin == buttonUp) {
    color1 = (color1 + 1) % 6;
  } else if (pin == buttonDown) {
    color2 = (color2 + 1) % 6;
  } else if (pin == buttonLeft) {
    color3 = (color3 + 1) % 6;
  } else {
    color4 = (color4 + 1) % 6;
  }
  player.move.playerGuess[0] = color1;
  player.move.playerGuess[1] = color2;
  player.move.playerGuess[2] = color3;
  player.move.playerGuess[3] = color4;
  player.printGuess();
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting stuff");

  player.setup();
  host.setup();

  // Turn off on-board NeoPixel
  pixel.setPixelColor(8, 0);
  pixel.show();

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


  //============================================//
  // TODO: Set-up the RMT                       //
  //============================================//
  if (!rmtInit(OUTPUT_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000)) {
    Serial.println("init sender failed\n");
  }

}

void loop() {
  if (player.move.turn){
    Serial.println("Player can go now");
  } else {
    Serial.println("Game has been reset");
  }

  //============================================//
  // TODO: Implement repeatedly defining the    //
  // bitstream while waiting for interrupts     //
  enterPressed = false;
  // while (!player.makeGuessBitBang());
  //============================================//

  int led, col, bit;
  int i = 0;
  for (led = 0; led < 4; led++) {
    for (col = 0; col < 3; col++) {
      for (bit = 0; bit < 8; bit++) {
        if (colors[player.move.playerGuess[led]][col] & (1 << (7 - bit))) {
          led_data[i].level0 = 1;
          led_data[i].duration0 = 8;
          led_data[i].level1 = 0;
          led_data[i].duration1 = 4;
        } else {
          led_data[i].level0 = 1;
          led_data[i].duration0 = 4;
          led_data[i].level1 = 0;
          led_data[i].duration1 = 8;
        }
        i++;
      }
    }
  }

  Serial.println("Submitted");
  player.printGuess();

  // Player lets the host know that they have made a guess
  player.notify();

  // After notify, move_callBack is called on the host side
  host.move_callBack();

  rmtWrite(OUTPUT_PIN, led_data, NR_OF_ALL_BITS, RMT_WAIT_FOR_EVER);
  delay(100);
}

//============================================//
// TODO: Define and handle your interrupts    //
//============================================//


//============================================//
// TODO: Set the NeoPixel bitstream           //
//============================================//
bool makeGuessBitBang(){

}

// Implement the BLE implementation of throwing the results flag
bool CodeMaker::throwResultsFlag(bool ongoing){
  player.move.turn = true;
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
  Serial.printf("Results: %d | %d \n", results[0], results[1]);
  
  if (results[0] == 4) {
    host.endGame(); // Don't worry about this for the main BLE parts, you'll implement this for extra credit
    host.throwResultsFlag(false);
  } else {
    host.throwResultsFlag(true);
  }
}
