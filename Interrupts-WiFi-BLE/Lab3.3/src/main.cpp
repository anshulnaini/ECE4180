/**
 * Part 3, 2.5, 3.5 of Bluetooth portion of ECE4180 Lab 3
 * BLE Client: subscribe to server char and toggle LED on '1'/'0'
 * plus chat messaging via Serial Monitor
 */

#define BUTTON 15
#define LED 23
#define LED2 22
#include "BLEDevice.h"

// --- UUIDs must match your server ---
static BLEUUID serviceUUID("C83086A7-AE34-49E0-8BFD-B399B8A97843");
static BLEUUID charUUID   ("6b055572-1602-4d75-a4d8-e40c836bb901"); // state / LED control
static BLEUUID messageCharUUID("a3b25c72-6f6d-4b02-9557-921f8b3e4a11"); // chat characteristic

// Connection control
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

bool lastButton = HIGH;

// BLE objects
static BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;         // state
static BLERemoteCharacteristic *pMessageRemoteCharacteristic = nullptr;  // message
static BLEAdvertisedDevice *myDevice = nullptr;

// --- Notification callback ---
static void notifyCallback(BLERemoteCharacteristic *ch, uint8_t *pData, size_t length, bool isNotify) {
  if (!length) return;

  // Handle LED state updates ('1'/'0')
  if (ch->getUUID().equals(charUUID)) {
    char v = (char)pData[0];
    if (v == '1') {
      digitalWrite(LED, HIGH);
      Serial.println("LED ON (server)");
    } else if (v == '0') {
      digitalWrite(LED, LOW);
      Serial.println("LED OFF (server)");
    }
  }

  // Handle chat messages
  else if (ch->getUUID().equals(messageCharUUID)) {
    String msg((char*)pData, length);
    Serial.print("[Server -> Client] ");
    Serial.println(msg);
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}
  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("Disconnected from server");
    if (doScan) BLEDevice::getScan()->start(5, false);
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println("Client successfully created!");
  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  pClient->setMTU(517);

  // Get service
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service.");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Get state characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find state characteristic.");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our state characteristic");
  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  // --- Get message characteristic (for chat) ---
  pMessageRemoteCharacteristic = pRemoteService->getCharacteristic(messageCharUUID);
  if (pMessageRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find message characteristic.");
  } else {
    Serial.println(" - Found our message characteristic");
    if (pMessageRemoteCharacteristic->canNotify())
      pMessageRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  connected = true;
  return true;
}

// --- Scanner callback ---
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
  pinMode(LED2, OUTPUT);
  digitalWrite(LED, LOW);
  digitalWrite(LED2, LOW);

  // Start scanning
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
      Serial.println("Connected to BLE Server!");
    } else {
      Serial.println("Failed to connect to server.");
    }
    doConnect = false;
  }

  if (connected) {
    // --- Part 2.5: Button writes '1'/'0' to server LED ---
    bool reading = digitalRead(BUTTON);
    if (reading != lastButton) {
      lastButton = reading;
      uint8_t cmd = (reading == LOW) ? '1' : '0';
      if (pRemoteCharacteristic && pRemoteCharacteristic->canWrite()) {
        pRemoteCharacteristic->writeValue(&cmd, 1, false);
        Serial.print("Wrote to server LED: ");
        Serial.println((char)cmd);
      }
    }

    // --- Part 3.5: Serial chat system ---
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line.length() && pMessageRemoteCharacteristic && pMessageRemoteCharacteristic->canWrite()) {
        pMessageRemoteCharacteristic->writeValue(line.c_str());
        Serial.print("[Client -> Server] ");
        Serial.println(line);
      }
    }
  }

  delay(50);
}
