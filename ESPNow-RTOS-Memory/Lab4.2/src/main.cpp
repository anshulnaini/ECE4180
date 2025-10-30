#include <Ticker.h>
#include <Adafruit_NeoPixel.h>

#define BTN_PIN   23
#define NEO_PIN   8
#define NEO_COUNT 1

Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

Ticker secondTicker;
Ticker strobeTicker;
Ticker greenHoldTicker;

const uint8_t START_COUNT = 3;
const float   STROBE_PERIOD = 0.15;
const uint16_t RED_BRIGHT   = 50;
const uint16_t GREEN_BRIGHT = 50;
const uint16_t OFF_BRIGHT   = 0;

enum RunState { IDLE, COUNTDOWN, SHOW_GREEN };
volatile RunState state = IDLE;

volatile bool secondTickFlag = false;
volatile bool strobeTickFlag = false;
volatile bool greenOffFlag   = false;

uint8_t currentCount = START_COUNT;
uint8_t flashesRemaining = 0;
bool    ledOnPhase = false;

unsigned long lastBtnChange = 0;
const unsigned long DEBOUNCE_MS = 30;
bool lastBtnState = HIGH;

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void ledOff()   { setRGB(0, 0, 0); }
void ledRed()   { setRGB(RED_BRIGHT, 0, 0); }
void ledGreen() { setRGB(0, GREEN_BRIGHT, 0); }

void IRAM_ATTR onSecondTick() { secondTickFlag = true; }
void IRAM_ATTR onStrobeTick() { strobeTickFlag = true; }
void IRAM_ATTR onGreenOff()   { greenOffFlag   = true; }

void startStrobe(uint8_t flashes) {
  flashesRemaining = flashes;
  ledOnPhase = false;
  strobeTicker.detach();
  strobeTicker.attach(STROBE_PERIOD, onStrobeTick);
  if (Serial) {
    Serial.print("[DEBUG] startStrobe: flashes=");
    Serial.println(flashes);
    Serial.print("[DEBUG] startStrobe: flashesRemaining=");
    Serial.println(flashesRemaining);
  }
}

void stopStrobe() {
  strobeTicker.detach();
  flashesRemaining = 0;
  ledOff();
  if (Serial) {
    Serial.println("[DEBUG] stopStrobe");
  }
}

void startCountdown() {
  if (state != IDLE) return;
  state = COUNTDOWN;
  currentCount = START_COUNT;
  secondTickFlag = false;
  strobeTickFlag = false;
  greenOffFlag   = false;

  secondTicker.detach();
  secondTicker.attach(1.0, onSecondTick);
  startStrobe(1);
  if (Serial) {
    Serial.print("[DEBUG] startCountdown: START_COUNT=");
    Serial.println(START_COUNT);
  }
}

void showGreenThenFinish() {
  state = SHOW_GREEN;
  stopStrobe();
  secondTicker.detach();
  ledGreen();
  greenHoldTicker.detach();
  greenHoldTicker.once(1.0, onGreenOff);
  if (Serial) {
    Serial.println("[DEBUG] showGreenThenFinish: showing green for 1s");
  }
}

void resetToIdle() {
  stopStrobe();
  secondTicker.detach();
  greenHoldTicker.detach();
  ledOff();
  state = IDLE;
  if (Serial) {
    Serial.println("[DEBUG] resetToIdle: back to IDLE");
  }
}

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  strip.begin();
  strip.setBrightness(20);
  ledOff();
  Serial.begin(115200);
  delay(3000);
  Serial.println("[DEBUG] setup: Serial started, pins configured");
}

void loop() {
  bool btn = digitalRead(BTN_PIN);
  unsigned long now = millis();
  if (btn != lastBtnState && (now - lastBtnChange) > DEBOUNCE_MS) {
    lastBtnChange = now;
    lastBtnState = btn;
    if (btn == LOW && state == IDLE) {
      Serial.println("[DEBUG] button pressed (LOW) while IDLE -> startCountdown");
      startCountdown();
    }
  }

  if (strobeTickFlag) {
    strobeTickFlag = false;

    if (state == COUNTDOWN && flashesRemaining > 0) {
      ledOnPhase = !ledOnPhase;
      if (ledOnPhase) {
        ledRed();
        if (Serial) {
          Serial.println("[DEBUG] strobe: LED ON (red)");
        }
      } else {
        ledOff();
        flashesRemaining--;
        if (Serial) {
          Serial.print("[DEBUG] strobe: LED OFF, flashesRemaining=");
          Serial.println(flashesRemaining);
        }
        if (flashesRemaining == 0) {
          stopStrobe();
        }
      }
    }
  }

  if (secondTickFlag) {
    secondTickFlag = false;

    if (state == COUNTDOWN) {
      currentCount--;
      if (Serial) {
        Serial.print("[DEBUG] second tick: decremented currentCount=");
        Serial.println(currentCount);
      }
      if (currentCount == 0) {
        showGreenThenFinish();
      } else {
        startStrobe(1);
      }
    }
  }

  if (greenOffFlag) {
    greenOffFlag = false;
    Serial.println("[DEBUG] greenHold expired -> resetToIdle");
    resetToIdle();
  }
}
void setup() {
    ESP_ERROR_CHECK(esp_timer_init());
    
    // Configure GPIO and NeoPixel
    pinMode(BTN_PIN, INPUT_PULLUP);
    strip.begin();
    strip.setBrightness(20);
    ledOff();
    
    // Start serial and show debug info
    Serial.begin(115200);
    delay(3000);
    Serial.println("[DEBUG] setup: Serial started, pins configured");
}

void loop() {
  // --- Button handling (rising-edge on pullup -> pressed = LOW) ---
  bool btn = digitalRead(BTN_PIN);
  unsigned long now = millis();
  if (btn != lastBtnState && (now - lastBtnChange) > DEBOUNCE_MS) {
    lastBtnChange = now;
    lastBtnState = btn;
    if (btn == LOW && state == IDLE) {
      Serial.println("[DEBUG] button pressed (LOW) while IDLE -> startCountdown");
      startCountdown();
    }
  }

  if (strobeTickFlag) {
    strobeTickFlag = false;

    if (state == COUNTDOWN && flashesRemaining > 0) {
      ledOnPhase = !ledOnPhase;
      if (ledOnPhase) {
        ledRed();
        if (Serial) {
          Serial.println("[DEBUG] strobe: LED ON (red)");
        }
      } else {
        // Turn OFF; completed one flash
        ledOff();
        flashesRemaining--;
        if (Serial) {
          Serial.print("[DEBUG] strobe: LED OFF, flashesRemaining=");
          Serial.println(flashesRemaining);
        }
        if (flashesRemaining == 0) {
          // Done flashing for this second
          stopStrobe();
        }
      }
    }
  }

  if (secondTickFlag) {
    secondTickFlag = false;

    if (state == COUNTDOWN) {
      //  just completed the previous secondMove to next count.
      currentCount--;
      if (Serial) {
        Serial.print("[DEBUG] second tick: decremented currentCount=");
        Serial.println(currentCount);
      }
      if (currentCount == 0) {
        // Finished 3,2,1 → show green
        showGreenThenFinish();
      } else {
        // Start the next second’s flashes
        // single flash per second
        startStrobe(1);
      }
    }
  }

  if (greenOffFlag) {
    greenOffFlag = false;
    Serial.println("[DEBUG] greenHold expired -> resetToIdle");
    resetToIdle();
  }
}
