#include <Arduino.h>

// Pin setup (common anode: LOW = ON, HIGH = OFF)
const int RED_PIN   = 18;
const int GREEN_PIN = 19;
const int BLUE_PIN  = 20;

const int BTN_FWD   = 0;
const int BTN_BACK  = 1;

// Color state machine
enum Color { RED, GREEN, BLUE, YELLOW };
volatile Color currentColor = RED;

// Optional flags just for printing in loop (not required for function)
volatile bool forwardEvent  = false;
volatile bool backwardEvent = false;

// Debounce timing
volatile unsigned long lastForwardTime = 0;
volatile unsigned long lastBackwardTime = 0;
const unsigned long debounceDelay = 150; // ms

// --- LED color (put in IRAM so it's safe to call from ISR on ESP32) ---
void IRAM_ATTR setColor(Color c) {
  // All OFF first (HIGH = off for common anode)
  digitalWrite(RED_PIN,   HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN,  HIGH);

  switch (c) {
    case RED:    digitalWrite(RED_PIN,   LOW); break;
    case GREEN:  digitalWrite(GREEN_PIN, LOW); break;
    case BLUE:   digitalWrite(BLUE_PIN,  LOW); break;
    case YELLOW: digitalWrite(RED_PIN,   LOW);
                 digitalWrite(GREEN_PIN, LOW); break; // R+G
  }
}

// --- ISRs: update state AND LED inside the interrupt ---
void IRAM_ATTR handleForward() {
  unsigned long now = millis();
  if (now - lastForwardTime > debounceDelay) {
    // R → G → B → Y → R
    currentColor = static_cast<Color>((currentColor + 1) % 4);
    setColor(currentColor);       // change LED immediately
    forwardEvent = true;          // for optional prints in loop
    lastForwardTime = now;
  }
}

void IRAM_ATTR handleBackward() {
  unsigned long now = millis();
  if (now - lastBackwardTime > debounceDelay) {
    // R ← Y ← B ← G ← R  (−1 mod 4 == +3 mod 4)
    currentColor = static_cast<Color>((currentColor + 3) % 4);
    setColor(currentColor);       // change LED immediately
    backwardEvent = true;         // for optional prints in loop
    lastBackwardTime = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting up...");

  // LED pins
  pinMode(RED_PIN,   OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN,  OUTPUT);

  // Buttons (to GND)
  pinMode(BTN_FWD,  INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  // Start with RED
  setColor(currentColor);

  // Interrupts on falling edge (active-low buttons)
  attachInterrupt(digitalPinToInterrupt(BTN_FWD),  handleForward, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_BACK), handleBackward, FALLING);
}

void loop() {
  // Optional: print events without affecting timing of the ISR-driven LED change
  if (forwardEvent) {
    noInterrupts(); forwardEvent = false; interrupts();
    Serial.println("Forward pressed: " + String(currentColor));
  }
  if (backwardEvent) {
    noInterrupts(); backwardEvent = false; interrupts();
    Serial.println("Backward pressed: " + String(currentColor));
  }

  // Periodic state print (optional)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 250) {
    lastPrint = millis();
    Serial.println("Current Color State: " + String(currentColor));
  }
}
