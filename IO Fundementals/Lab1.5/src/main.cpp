#include <Arduino.h>

#define MIC_ADC_PIN 1 
#define LED_PIN     10

const int THRESH = 650;
const int MAXLVL = 850;

int v = 0;

void setup() {
  analogReadResolution(12);
  pinMode(LED_PIN, OUTPUT);
  analogWriteFrequency(LED_PIN, 5000);
  analogWriteResolution(LED_PIN, 8);
  analogWrite(LED_PIN, 0);
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  v = analogRead(MIC_ADC_PIN);
  Serial.println(v);
  
  int duty = 0;
  if (v > THRESH) {
    duty = map(v, THRESH, MAXLVL, 0, 255);
  } else {
    duty = 0;
  }

  if (duty < 0) duty = 0;
  if (duty > 255) duty = 255;
  analogWrite(LED_PIN, duty);
  delay(5);
}
