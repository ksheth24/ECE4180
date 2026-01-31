int* GPIO_OUTPUT_ENABLE = (int*) 0x60091024;
int* GPIO_OUTPUT_SET = (int*) 0x60091008;
int* GPIO_OUTPUT_CLEAR = (int*) 0x6009100C;
int* IO_MUX_GPIO4 = (int*) 0x60090014;
int* IO_MUX_GPIO7 = (int*) 0x60090020;
int* GPIO_INPUT = (int*) 0x6009103C;

void setup() {
  *GPIO_OUTPUT_ENABLE |= (1<<7);
  *IO_MUX_GPIO4 |= 0x280;
  *IO_MUX_GPIO7 |= 0x1000;
  Serial.begin(115200);
  Serial.print("Hello");
}

void loop() {
  if ((*GPIO_INPUT & (1<<4)) != 0) {
    Serial.print("Button pressed.");
    *GPIO_OUTPUT_SET = (1<<7);
  } else {
    *GPIO_OUTPUT_CLEAR = (1<<7);
  }
}
