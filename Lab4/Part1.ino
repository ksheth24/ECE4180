#define CODEMAKER
#include <ECE4180MasterMind.h>

#include <DealerClassifier_inferencing.h>

CodeMaker dealer;
uint8_t dealerStyle;

void setup() {
  Serial.begin(9600);
  delay(2000);   // allow serial to attach
  
  dealer.setup();
  dealerStyle = static_cast<uint>(random(0,3));
  pinMode(0, INPUT_PULLDOWN);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(0)){
    dealer.generateBiasedCode(dealerStyle);
    uint8_t code[4];
    dealer.printCode();
    dealer.getCode(code);
    classifyDealerStyle(code);
    delay(1000);
  }
}

void classifyDealerStyle(uint8_t* code) {
    // TODO: Calculate the same features used in training
    // Feel free to make whatever helper functions you might need, also you can find the
    // original implementations to reverse engineer in the library under generateBiasedCode()
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
    // Wrap your features for the EON compiler
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
        Serial.printf("    %s: %.2f\n", result.classification[ix].label, 
                                        result.classification[ix].value); 
    }
}
