<ESP32Servo.h>

Servo myServo

void setup() {
  // put your setup code here, to run once:
  myServo.attach(8);
  pinMode(5, INPUT_PULLUP);
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
}

void loop() {
  // put your main code here, to run repeatedly:5
  if (!digitalRead(5)) {

  }
  if (!digitalRead(18)) {
    
  }
  if (!digitalRead(19)) {
    
  }
  if (!digitalRead(21)) {
    
  }
  myservo.write(90);
}
