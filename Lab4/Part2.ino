#define CODEMAKER
#include <ECE4180MasterMind.h>

#include <DealerClassifier_inferencing.h>

CodeMaker dealer;

#define ARRAY_SIZE 1296   // 6^4
#define VECTOR_SIZE 4
#define BASE 6

uint8_t possibilities[ARRAY_SIZE][VECTOR_SIZE];
bool active[ARRAY_SIZE];

int currentGuessIdx;
uint8_t turns;
bool gameRunning;

float classifierConfidence[3] = {0, 0, 0}; // [descending, low_numbers, two_pairs] - Model is in different order than style selection
uint8_t dealerStyle; // Randomly generated dealer style
uint8_t currentDealerStyle; // What the ML will recognize as the dealer's style

// Use this as opposed to division because it's cheaper (I think)
// Generates the tree
void populateArray(uint8_t (*array)[VECTOR_SIZE], uint32_t size) {
  uint8_t counters[VECTOR_SIZE] = {0};

  for (uint32_t i = 0; i < size; i++) {
    active[i] = true;
    // Copy current vector
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

void prune(uint8_t* lastGuess, uint8_t* results) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (!active[i]) continue; // Already pruned

        // "If possibilities[i] was the secret, what feedback would it give?"
        uint8_t reference[2];
        dealer.compare(reference, possibilities[i], lastGuess);

        // Remove possible answers that do not match
        if (reference[0] != results[0] || reference[1] != results[1]) {
            active[i] = false; 
        } 
    }
}

int getBestGuess() {
    int bestGuessIdx = -1;
    int minMaxRemaining = 2000;

    Serial.println("Calculating best guess...");
    uint32_t start = millis();

    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (i == 0) {
            int activeCount = 0;
            for(int k=0; k<ARRAY_SIZE; k++) if(active[k]) activeCount++;
            Serial.printf("Active codes remaining: %d\n", activeCount);
        }
        // Heartbeat print every 200 iterations so you know it hasn't crashed
        if (i % 200 == 0) Serial.print("."); 

        // KNUTH'S RULE: We can guess ANY code (even inactive ones)
        // to gain maximum information. Equivalent to in Wordle when you have something
        // like _ATCH so you guess PLUMB to narrow it down even if it doesn't work
        
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
        } // If it's a tie, prefer a code that could actually be the secret
        else if (maxForThisGuess == minMaxRemaining && active[i] && !active[bestGuessIdx]) {
            bestGuessIdx = i;
        }
    }
    
    Serial.printf("\nDone! Took %d ms. Best Index: %d\n", (millis() - start), bestGuessIdx);
    return bestGuessIdx;
}

void classifyDealerStyle(uint8_t* code) {
    // TODO: See Part 1
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

    // TODO: Populate a float array of features with the following order:
    // code[0], code[1], code[2], code[3], uniqueCount, isDescending, maxV
    float features[7];
    features[0] = code[0];
    features[1] = code[1];
    features[2] = code[2];
    features[3] = code[3];
    features[4] = uniqueCount;
    features[5] = isDescending;
    features[6] = maxV;

    // Wrap the features for the EON compiler
    signal_t signal;
    numpy::signal_from_buffer(features, 7, &signal);

    // Run inference
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) return;

    // Print the results (the Style Probabilities)
    Serial.printf("Predictions (DSP: %d ms, NN: %d ms)\n", 
                  result.timing.dsp, result.timing.classification);
    
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.printf("    %s: %.2f\n", result.classification[ix].label, // Fixed the map backwards problem
                                        result.classification[ix].value);
        // TODO: Update your confidences with these classifications
        classifierConfidence[ix] += result.classification[ix].value;
        
    }
}

void prePrune(uint8_t predictedStyle) {
    // TODO: Prune the array of possibilities to remove anything that doesn't match your recognized style
    // Similar to prune, but we're now removing things that don't match the given style
    // Similar logic as classifying the dealer's style from last part
    for (int i = 0; i < ARRAY_SIZE; i++) {
      if (!active[i]) {
        continue;
      }
      uint8_t* code = possibilities[i];
      int uniqueCount = 0;
      int isDescending = code[0] >= code[1] && code[1] >= code[2] && code[2] >= code[3];
      int maxV = 0;
      bool unique = false;
      for (int k = 0; k < 4; k++) {
        unique = true;
        if (code[k] > maxV) {
          maxV = code[k];
        }
        for (int j = k - 1; j >= 0; j--) {
          if (code[j] == code[k]) {
            unique = false;
          }
        }
        if (unique) {
          uniqueCount++;
        }
      }
      if (predictedStyle == 0) {         
          if (!isDescending) active[i] = false;
      }
      else if (predictedStyle == 1) {    
          if (maxV > 2) active[i] = false;
      }
      else if (predictedStyle == 2) {    
          if (uniqueCount != 2) active[i] = false;
      }
    }

}

void setup() {
    Serial.begin(9600);
    delay(2000);   // allow serial to attach
    dealer.setup();
    gameRunning = true;
    dealerStyle = static_cast<uint>(random(0,3));
}

void loop() {
  // TODO: Run the AI Model a few times to get an idea of what random code the dealer is using
  // Remember to use generateBiasedCode(style) instead of generateCode()
  for (int i = 0; i < 5; i++) {
    uint8_t code[4];

    dealer.generateBiasedCode(dealerStyle);
    dealer.getCode(code);

    classifyDealerStyle(code);
  }

  // TODO: Decide what the model thinks is the best style
  // Any reasonable approach is fine, I did running average but majority vote is also great
  int winningStyle = 0;
  if (classifierConfidence[0] > classifierConfidence[1] && classifierConfidence[0] > classifierConfidence[2]) {
    winningStyle = 0;
  } else if (classifierConfidence[1] > classifierConfidence[0] && classifierConfidence[1] > classifierConfidence[2]) {
    winningStyle = 1;
  } else {
    winningStyle = 2;
  }

  Serial.print("DEALER PROFILED: ");
  Serial.println(winningStyle == 0 ? "DESCENDING" : 
                  winningStyle == 1 ? "LOW_NUMBERS" : "TWO_PAIRS");
  delay(3000); // Delay for readability

  // I accidentally trained the model's outputs to be different from the inputs to the dealer
  currentDealerStyle = (winningStyle == 0 ? 1 : winningStyle == 1 ? 0 : 2);  // Fixes the ordering, the model outputs are backwards
  
  // TODO: Run it again several times using the pre-prune function 
  // to narrow down the list of possibilities before starting guessing
  // You should notice a significant increase in efficiency after the initial pre-prune step
  prePrune(currentDealerStyle);
  while(1);     // Blocking while so this doesn't loop endlessly

}
