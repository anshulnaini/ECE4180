#include <Arduino.h>

// Pin definitions for ESP32-C6
const int potPin = 0;   // ADC input (GPIO0)
const int AIN1   = 4;   // TB6612 direction
const int AIN2   = 5;   // TB6612 direction
const int PWMA   = 6;   // PWM output pin (choose any free GPIO)

void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
}

void loop() {
  int raw = analogRead(potPin);    // 0..4095
  int offset = raw - 1666;          // Center at ~0

  int speed = abs(offset);
  int duty = map(speed, 0, 2047, 0, 255);  // scale to 0..255
  duty = constrain(duty, 0, 255);

  if (offset > 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    Serial.println("Forward");
  } else if (offset < 0) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    Serial.println("Backward");
  } else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
  }

  analogWrite(PWMA, duty);  // ESP32-C6 Arduino core handles PWM here

  delay(10);
}
