#include <Arduino.h>

// --- Include whatever FreeRTOS libraries you might need ---
#if CONFIG_FREERTOS_UNICORE
#define TASK_RUNNING_CORE 0
#endif


// --- Declare Semaphore and Mutex Stuff ---


// --- Define MasterMind stuff ---
#define CODEMAKER
#define CODEBREAKER
#include <ECE4180MasterMind.h>

CodeMaker dealer;
CodeBreaker player;

#define BUTTON_PIN 5



// --- Define BLE stuff ---
void bleGameplayTask(void *pvParameters);

void IRAM_ATTR handleButtonPress() {

}

// Task B: AI Guess Making (Triggered by Button)
void aiGuessTask(void *pvParameters) {
    
}

// -------------------------------------------------------------------------
// CORE 0: BLE & WEB TASK
// Add whatever helper functions you might need
// -------------------------------------------------------------------------
void bleGameplayTask(void *pvParameters) {
    
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);   // allow serial to attach
    
    // TODO: Set-up CodeMaker and CodeBreaker objects 

    // TODO: Initialize Synchronization Stuff,like Semaphores and Mutexes

    // TODO: Initialize your button with an ISR (you will need to configure an ISR with this RTOS)
    pinMode(BUTTON_PIN, INPUT);    // external hardware debounce

    attachInterrupt(BUTTON_PIN, handleButtonPress, RISING);
    
    // TODO: Pin Tasks to Cores
    // Core 1: AI and ML (Higher priority for pruning)

    // Core 0: BLE

}

void loop() {
    // Empty: Everything is managed by FreeRTOS tasks
    vTaskDelete(NULL); 
}
