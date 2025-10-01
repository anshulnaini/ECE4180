#include <Adafruit_NeoPixel.h>

#define LED_PIN     8
#define NUM_LEDS    1

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pixel.begin();
  pixel.setBrightness(20);
  Serial.begin(115200);
}

void loop() {
  pixel.setPixelColor(0, pixel.Color(255, 0, 0));
  pixel.show();
  delay(500);
  Serial.println("Red");

  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
  delay(500);
}
