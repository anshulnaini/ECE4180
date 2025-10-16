/**
 * Part 3, 2.5, 3.5 of Bluetooth portion of ECE4180 Lab 3
 * BLE Client: subscribe to server char and toggle LED on '1'/'0'
 */

#define BUTTON 15
#define LED 23
#define LED2 22
#include "BLEDevice.h"

// Match your server UUIDs exactly (case-insensitive is fine)
static BLEUUID serviceUUID("C83086A7-AE34-49E0-8BFD-B399B8A97843");
static BLEUUID charUUID   ("6b055572-1602-4d75-a4d8-e40c836bb901");

// Connection control
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

// BLE objects
static BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;
static BLEAdvertisedDevice *myDevice = nullptr;

// --- Notification callback: drives LED from '1'/'0' ---
static void notifyCallback(BLERemoteCharacteristic *ch, uint8_t *pData, size_t length, bool isNotify) {
  Serial.print("Notify from ");
  Serial.print(ch->getUUID().toString().c_str());
  Serial.print(" len=");
  Serial.println(length);

  if (!length) return;
  char v = (char)pData[0];
  if (v == '1') {
    digitalWrite(LED, HIGH);
    Serial.println("LED ON");
  } else if (v == '0') {
    digitalWrite(LED, LOW);
    Serial.println("LED OFF");
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}
  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect");
    if (doScan) BLEDevice::getScan()->start(5, false); // optional: resume scanning
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println("Client successfully created!");
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect + configure MTU
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  pClient->setMTU(517);

  // Get service
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Get characteristic (the ONLY one we use)
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find characteristic: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Optional: read current value
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("Initial value: ");
    Serial.println(value.c_str());
  }

  // Subscribe to notifications
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println(" - Subscribed to notifications");
  } else {
    Serial.println(" - WARNING: characteristic cannot notify");
  }

  connected = true;
  return true;
}

// --- Scanner callback: finds our service and triggers connect ---
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("ESP32 Button Client");

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(LED2, OUTPUT);         // not used; keep if your lab template expects it
  digitalWrite(LED, LOW);
  digitalWrite(LED2, LOW);

  // Scanner
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server.");
    }
    doConnect = false;
  }

  if (connected) {
    // No polling needed; notifications drive the LED.
    // (If you really want polling, you could readValue() here.)
  }

  delay(50);
}
