#include <Arduino.h>
   

void setup() {
  ledcAttach(5, 2000, 12);
  ledcWrite(5, 2048);
}

void loop() {
  
}
