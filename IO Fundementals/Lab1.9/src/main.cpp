#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const int PIN_LED   = 8;
const int PIN_BTN   = 1; 
const int BUZZER    = 10;

const int AIN1 = 4;
const int AIN2 = 5;
const int PWMA = 6;

Adafruit_NeoPixel pixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

const int BEEP_LOW_HZ = 2000;  
const int BEEP_GO_HZ  = 3500;
const int BEEP_MS     = 200; 

bool lastBtn = HIGH;
unsigned long lastChange = 0;
const unsigned long DEBOUNCE_MS = 40;


void ledOff() { 
  pixel.setPixelColor(0, 0); 
  pixel.show();
  Serial.println("LED off");
}

void setLED(uint8_t r, uint8_t g, uint8_t b) {
  Serial.printf("LED set to %d,%d,%d\n", r, g, b);
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void flashColor(uint8_t r, uint8_t g, uint8_t b, unsigned long ms, unsigned onMs = 100, unsigned offMs = 100) {
  Serial.printf("Flashing %d,%d,%d for %d ms\n", r, g, b, ms);
  unsigned long start = millis();
  while (millis() - start < ms) {
    setLED(r, g, b);
    delay(onMs);
    ledOff();
    unsigned long remain = (millis() - start);
    if (remain >= ms) break;
    delay(offMs);
  }
}

// ---- Bit-bang beep ----
void beepHz(int hz, int ms) {
  Serial.printf("Beeping %d Hz for %d ms\n", hz, ms);
  pinMode(BUZZER, OUTPUT);
  unsigned long cycles = (unsigned long)((long)ms * hz / 1000);
  int half_us = 500000 / hz; // half-period
  for (unsigned long i = 0; i < cycles; i++) {
    digitalWrite(BUZZER, HIGH);
    delayMicroseconds(half_us);
    digitalWrite(BUZZER, LOW);
    delayMicroseconds(half_us);
  }
}


void motorForward(uint8_t duty) {
  Serial.printf("Motor forward at duty %d\n", duty);
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, duty);
}

void motorOff() {
  Serial.println("Motor off");
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 0);
}

void runCountdown() {
  Serial.println("Starting countdown!");
  const uint8_t d1 = 85;
  const uint8_t d2 = 170;
  const uint8_t d3 = 240;

  motorForward(d1);
  beepHz(BEEP_LOW_HZ, BEEP_MS);
  flashColor(255, 0, 0, 1000);

  motorForward(d2);
  beepHz(BEEP_LOW_HZ, BEEP_MS);
  flashColor(255, 0, 0, 1000);

  motorForward(d3);
  beepHz(BEEP_LOW_HZ, BEEP_MS);
  flashColor(255, 0, 0, 1000);

  motorForward(255);
  beepHz(BEEP_GO_HZ, BEEP_MS);
  flashColor(0, 255, 0, 1000);

  motorOff();
  ledOff();
}

void setup() {
  Serial.begin(115200);
  delay(1500); 
  Serial.println("Starting setup...");

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  motorOff();

  pinMode(PIN_BTN, INPUT_PULLDOWN);

  pixel.begin();
  ledOff();
  delay(1000);

  Serial.println("Setup complete");
}

void loop() {
  bool lvl = digitalRead(PIN_BTN);
  unsigned long now = millis();
  if (lvl != lastBtn && (now - lastChange) > DEBOUNCE_MS) {
    lastChange = now;
    lastBtn = lvl;
    if (lvl == HIGH) {
      Serial.println("Button Pressed");
      runCountdown();
    }
  }
}
