#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#define LED_PIN     8
#define NUM_LEDS    1
#define BUTTON1_PIN  4
#define BUTTON2_PIN  5

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

int colorState = 0;

int lastButton1 = LOW;
int lastButton2 = LOW;

void setColor();

void setup() {
  pixel.begin();
  pixel.setBrightness(20);
  pinMode(BUTTON1_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON2_PIN, INPUT_PULLDOWN);

  setColor();

  Serial.begin(115200);
}

void loop() {
  int button1Value = digitalRead(BUTTON1_PIN);
  int button2Value = digitalRead(BUTTON2_PIN);
  if (button1Value == HIGH && lastButton1 == LOW) {
    colorState = (colorState + 1) % 4;
    setColor();
    delay(200);
  } else if (button2Value == HIGH && lastButton2 == LOW) {
    colorState = (colorState + 3) % 4;
    setColor();
    delay(200);
  }
  lastButton1 = button1Value;
  lastButton2 = button2Value;
}




void setColor() {
  switch (colorState) {
    case 0:
      pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red
      break;
    case 1:
      pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // Green
      break;
    case 2:
      pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // Blue
      break;
    default:
      pixel.setPixelColor(0, pixel.Color(255, 255, 0)); // Yellow
      break;
  }
  pixel.show();
}
