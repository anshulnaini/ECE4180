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


BLECharacteristic *pCharacteristic;
BLECharacteristic *pMessageCharacteristic; 

#define BUTTON_PIN 10
bool buttonState = HIGH;  

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Monitor initialized!!");


  pinMode(BUTTON_PIN, INPUT_PULLUP);  


  BLEDevice::init("Matt's ESP Controller");          
  BLEServer *pServer = BLEDevice::createServer();  
  BLEService *pService = pServer->createService(SERVICE_UUID);


  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  pCharacteristic->setValue("Button state: RELEASED");


  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.println("BLE Set-up complete (Part 2)");
}

void loop() {
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