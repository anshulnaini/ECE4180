/**
 * Part 2, 2.5, 3.5 of Bluetooth portion of ECE4180 Lab 3
 * Code for BLE Server to broadcast messages, in this case
 * state encoding of status of buttons pressed
 *
 * Based on default example provided under BLE->Server
 * author Jason Hsiao
 * 
 * original author unknown
 * updated by chegewara
 */

// Useful Bluetooth packages
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// IMPORTANT, PLEASE REPLACE THESE UIUD'S BEFORE STARTING THE LAB, THESE MUST BE UNIQUE FOR A REASON
// Part 2 TODO #1:
// Set UIUDs for your service and any characteristics you might need using online UIUD generator: https://www.uuidgenerator.net/
#define SERVICE_UUID           "e7a6e6d1-9a23-4f7c-9fdd-2c3b61be70b1"
#define CHARACTERISTIC_UUID    "8d9d5b0e-0a84-4a62-9d9a-8b9f9e9d5f01"

// These are pointers to your BLE characteristic objects
// BLE Characteristics for LED and for messaging
BLECharacteristic *pCharacteristic;
BLECharacteristic *pMessageCharacteristic;

// Part 2 TODO #2:
// Define any constants or variables you might need for operating a pushbutton
#define BUTTON_PIN  0              // Button to GND; uses internal pull-up
bool buttonState = HIGH;           // current stable state (HIGH = not pressed)
bool lastReadState = HIGH;         // last raw read for debounce
unsigned long lastDebounceMs = 0;
const unsigned long debounceDelayMs = 100; // debounce window

// Part 2.5 TODO #1:
// Define any constants or variables you might need for operating an LED
#define LED_PIN 2                  // On-board LED on many ESP32 devkits

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Monitor initialized!!");

  // Part 2 TODO #2 (continued)
  // Set up your pushbutton here
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  buttonState   = digitalRead(BUTTON_PIN);
  lastReadState = buttonState;

  // Part 2.5 TODO #1 (continued)
  // Set up your LED here
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Part 2 TODO #3
  // Setting up your Bluetooth Device
  BLEDevice::init("Jason's ESP Controller");  // Replace with name of your Bluetooth device, just make this something easily identifiable so it can be found via Bluetooth scanner
  BLEServer *pServer = BLEDevice::createServer();                      // Using the BLEDevice object that you just initiated, create a Server object
  BLEService *pService = pServer->createService(SERVICE_UUID);         // Using the Server object that you just created, create a Service with your specific UIUD that you defined in TODO #1

  // Following this trend, use the Service object you just created to create a characteristic (which we defined earlier)
  // Hint: Part 1 requires you to demonstrate that you must be able to READ your characteristic from a Bluetooth scanner
  pCharacteristic = pService->createCharacteristic(    
    CHARACTERISTIC_UUID, // How do we identify the characteristic?
    // BLECharacteristic::PROPERTY1 | BLECharacteristic::PROPERTY2 | BLECharacteristic::PROPERTY3 // Here we pass in the properties we wish the characteristic to have  
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );

  // Part 2.5 TODO #2: Coming back to this characteristic definition, what additional properties do you need to enable to allow the Client to WRITE back to this server
  // Hint: Furthermore, you will need to find a way to tell other devices that your characteristic has been updated (essentially pinging updates)
  // Note: These properties are all in a register together, so you can easily bitwise OR additional properties
  // (We keep WRITE for the *message* characteristic below; this state characteristic is READ + NOTIFY.)
  pCharacteristic->setValue((uint8_t)(buttonState == LOW ? 1 : 0));

  // Part 3.5 TODO #1 
  // Create a Second Characteristic using the same service to handle messaging functionality
  pMessageCharacteristic = pService->createCharacteristic(
    "a3b25c72-6f6d-4b02-9557-921f8b3e4a11",                // replace with your own unique UUID if desired
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pMessageCharacteristic->setValue("Ready");

  // We can now start our service :)
  pService->start();

  // This part is where our Bluetooth device starts advertising its presence to surrounding receivers
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // determines the minimum gap between communications
  // pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE Set-up complete");
}

void loop() {
  // Once set-up is complete, put the rest of your code here

  // --- Debounce the pushbutton ---
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastReadState) {
    lastDebounceMs = millis();
    lastReadState = reading;
  }

  if ((millis() - lastDebounceMs) > debounceDelayMs) {
    if (reading != buttonState) {
      buttonState = reading;
      bool pressed = (buttonState == LOW); // active-low

      // LED feedback
      digitalWrite(LED_PIN, pressed ? HIGH : LOW);

      // Update primary characteristic (0/1) and notify
      uint8_t stateVal = pressed ? 1 : 0;
      pCharacteristic->setValue(&stateVal, 1);
      pCharacteristic->notify();

      // Update message characteristic and notify
      if (pressed) {
        pMessageCharacteristic->setValue("Button pressed!");
        pMessageCharacteristic->notify();
        Serial.println("Button pressed!");
      } else {
        pMessageCharacteristic->setValue("Button released!");
        pMessageCharacteristic->notify();
        Serial.println("Button released!");
      }
    }
  }

  delay(50); // Small delay to reduce CPU usage
}
