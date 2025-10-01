#include <Arduino.h>
#include <ESP32Servo.h>

static const int PIN_SERVO = 0;
static const int PIN_DIP0  = 4;
static const int PIN_DIP1  = 5;
static const int PIN_DIP2  = 6;
static const int PIN_DIP3  = 7;

Servo myServo;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_DIP0, INPUT_PULLDOWN);
  pinMode(PIN_DIP1, INPUT_PULLDOWN);
  pinMode(PIN_DIP2, INPUT_PULLDOWN);
  pinMode(PIN_DIP3, INPUT_PULLDOWN);

  myServo.setPeriodHertz(50);
  myServo.attach(PIN_SERVO, 500, 2500);
}

void loop() {
  if (digitalRead(PIN_DIP0) == HIGH) {
    Serial.println("DIP0 is HIGH");
    myServo.write(0);
  } else if (digitalRead(PIN_DIP1) == HIGH) {
    Serial.println("DIP1 is HIGH");
    myServo.write(45);
  } else if (digitalRead(PIN_DIP2) == HIGH) {
    Serial.println("DIP2 is HIGH");
    myServo.write(90);
  } else if (digitalRead(PIN_DIP3) == HIGH) {
    Serial.println("DIP3 is HIGH");
    myServo.write(135);
  }
  delay(20);
}
