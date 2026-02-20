#include <ICM_20948.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define AD0_VAL 1
ICM_20948_I2C myICM;

u_int8_t buffer = 200;
Adafruit_NeoPixel pixel(1, 8, NEO_GRB + NEO_KHZ800);

void setup () {
  Serial.begin(115200);
  while(!Serial);
  Wire.begin(23, 22);
  Wire.setClock(400000);
  Serial.println("hello");

  myICM.begin(Wire, AD0_VAL);
  Serial.println("ICM Configed");

  pixel.begin();
  pixel.setBrightness(20);
}

void loop() {
  if (myICM.dataReady()) {
    Serial.println("Data Recieved");
    myICM.getAGMT();
    printScaledAGMT(&myICM);
    if (myICM.accX() > 400) {
      pixel.setPixelColor(0, pixel.Color(0,255,0));
      pixel.show();
    } else if (myICM.accX() < -400) {
      pixel.setPixelColor(0, pixel.Color(255,20,147));
      pixel.show();
    } else if (myICM.accY() > 400) {
      pixel.setPixelColor(0, pixel.Color(255, 165, 0));
      pixel.show();
    } else if (myICM.accY() < -400) {
      pixel.setPixelColor(0, pixel.Color(0,255,255));
      pixel.show();
    } else if (myICM.accZ() - 1000 < -buffer) {
      pixel.setPixelColor(0, pixel.Color(255,0,0));
      pixel.show();
    } else if (myICM.accZ() - 1000 > buffer) {
      pixel.setPixelColor(0, pixel.Color(0,0,255));
      pixel.show();
    } else {
      pixel.clear();
    }
    delay(50); 
  } else {
    Serial.println("Waiting");
    delay(30);
  }
}

void printScaledAGMT(ICM_20948_I2C *sensor)
{
  Serial.print("Scaled. Acc (mg) [ ");
  printFormattedFloat(sensor->accX(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->accY(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->accZ(), 5, 2);
  Serial.print(" ], Gyr (DPS) [ ");
  printFormattedFloat(sensor->gyrX(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->gyrY(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->gyrZ(), 5, 2);
  Serial.print(" ], Mag (uT) [ ");
  printFormattedFloat(sensor->magX(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->magY(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->magZ(), 5, 2);
  Serial.print(" ], Tmp (C) [ ");
  printFormattedFloat(sensor->temp(), 5, 2);
  Serial.print(" ]");
  Serial.println();
}

void printFormattedFloat(float val, int width, int precision)
{
  char buffer[32];
  dtostrf(val, width, precision, buffer);
  Serial.print(buffer);
}
