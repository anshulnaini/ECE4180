#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;



void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7); 
  dac.begin(0x60);
}

void loop() {
  for (int i = 0; i < 360; i++) {
    float rad = i * PI / 180.0; 
    float s   = sin(rad);
    int value = 2048 + (int)(2047 * s);

    dac.setVoltage(value, false);
    delayMicroseconds(100);   
  }
}
