/**
 * Part 2 of Bluetooth portion of ECE4180 Lab 3
 * BLE Server broadcasts a custom message when a pushbutton is pressed
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


#define SERVICE_UUID        "c83086a7-ae34-49e0-8bfd-b399b8a97843"
#define CHARACTERISTIC_UUID "6b055572-1602-4d75-a4d8-e40c836bb901"
#define MESSAGE_CHARACTERISTIC_UUID "a3b25c72-6f6d-4b02-9557-921f8b3e4a11"


BLECharacteristic *pCharacteristic;
BLECharacteristic *pMessageCharacteristic; 


#define BUTTON_PIN 10
#define SERVER_LED_PIN 15 
bool buttonState = HIGH;  

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Monitor initialized!!");


  pinMode(BUTTON_PIN, INPUT_PULLUP);  
  pinMode(SERVER_LED_PIN, OUTPUT);
  digitalWrite(SERVER_LED_PIN, LOW);


  BLEDevice::init("Matt's ESP Controller");          
  BLEServer *pServer = BLEDevice::createServer();  
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // --- message characteristic (READ | WRITE | NOTIFY) ---
pMessageCharacteristic = pService->createCharacteristic(
  MESSAGE_CHARACTERISTIC_UUID,
  BLECharacteristic::PROPERTY_READ |
  BLECharacteristic::PROPERTY_WRITE |
  BLECharacteristic::PROPERTY_NOTIFY
);
pMessageCharacteristic->setValue("Server ready");

  // Print anything the client writes to this message characteristic
  class MsgCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *c) override {
      String s = c->getValue();            // Arduino String (avoids std::string issue)
      if (s.length()) {
        Serial.print("[Client -> Server] ");
        Serial.println(s);
        // Echo it back to all subscribers so both serial monitors see it
        c->setValue(s.c_str());
        c->notify();
      }
    }
  };
  pMessageCharacteristic->setCallbacks(new MsgCallbacks());


  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setValue("Button state: RELEASED");

  class CharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *c) override {
      String v = c->getValue();          // <-- returns Arduino String
      if (v.length() > 0) {
        char b = v.charAt(0);            // first byte written
        if (b == '1') {
          digitalWrite(SERVER_LED_PIN, HIGH);
          Serial.println("Server LED -> ON (client write)");
        } else if (b == '0') {
          digitalWrite(SERVER_LED_PIN, LOW);
          Serial.println("Server LED -> OFF (client write)");
        }
      }
    }
  };
  pCharacteristic->setCallbacks(new CharCallbacks());


  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.println("BLE Set-up complete (Part 2)");
}

void loop() {

  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length()) {
      pMessageCharacteristic->setValue(line.c_str());
      pMessageCharacteristic->notify();
      Serial.print("[Server -> Client] ");
      Serial.println(line);
    }
  }
  int current = digitalRead(BUTTON_PIN);
  if (current != buttonState) {
    buttonState = current;

    if (buttonState == LOW) {
      uint8_t value = '1'; 
      pCharacteristic->setValue(&value, 1);
      pCharacteristic->notify();  
      Serial.println("Button pressed -> sent 1");
    } else {
      uint8_t value = '0';  
      pCharacteristic->setValue(&value, 1);
      pCharacteristic->notify();
      Serial.println("Button released -> sent 0");
    }
  }
  delay(10);
}