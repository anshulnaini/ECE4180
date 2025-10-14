#include <Arduino.h>

#include <Adafruit_MCP23X17.h>

#define BUTTON_PIN  1 
#define LED_PIN     0  

#define CS_PIN   10
#define MOSI_PIN 1
#define MISO_PIN 0
#define SCK_PIN  8

Adafruit_MCP23X17 mcp;

// -----------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("MCP23xxx Button Test!");

  if (!mcp.begin_SPI(CS_PIN, SCK_PIN, MISO_PIN, MOSI_PIN, 0)) {
    Serial.println("Error.");
    while (1);
  }


  mcp.pinMode(BUTTON_PIN, INPUT_PULLUP);
  mcp.pinMode(LED_PIN, OUTPUT);
  mcp.digitalWrite(LED_PIN, LOW); 
}

void loop() {

  if (!mcp.digitalRead(BUTTON_PIN)) {
    Serial.println("Button Pressed!");
    mcp.digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("Unpressed!");
    mcp.digitalWrite(LED_PIN, LOW);
  }
  delay(500);
}
