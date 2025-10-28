#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <stdio.h>


#define UP_PIN     22
#define DOWN_PIN   20
#define LEFT_PIN   21
#define RIGHT_PIN  19
#define CENTER_PIN 18


uint8_t receiverMAC[] = {0xA0, 0x85, 0xE3, 0xDA, 0xC0, 0xA0};

typedef struct struct_message {
  int8_t dx;
  int8_t dy;
  bool centerPressed;
} struct_message;

struct_message outgoing;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("OnDataSent - MAC: ");
  for (int i = 0; i < 6; i++) {
    if (mac_addr[i] < 0x10) Serial.print("0");
    Serial.print(mac_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.print("  Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n--- Setup start ---");


  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  pinMode(CENTER_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  Serial.println("WiFi set to STA mode");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  Serial.println("ESP-NOW initialized");

  esp_now_register_send_cb(OnDataSent);
  Serial.println("Registered OnDataSent callback");

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_err_t addPeerRes = esp_now_add_peer(&peerInfo);
  if (addPeerRes != ESP_OK) {
    Serial.print("Failed to add peer (err=0x");
    Serial.print(addPeerRes, HEX);
    Serial.println(")");
    return;
  }
  Serial.print("Peer added: ");
  for (int i = 0; i < 6; i++) {
    if (receiverMAC[i] < 0x10) Serial.print("0");
    Serial.print(receiverMAC[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  Serial.println("--- Setup complete ---\n");
}

void readInput(int8_t &dx, int8_t &dy, bool &centerPressed) {
  int up    = (digitalRead(UP_PIN) == LOW);
  int down  = (digitalRead(DOWN_PIN) == LOW);
  int left  = (digitalRead(LEFT_PIN) == LOW);
  int right = (digitalRead(RIGHT_PIN) == LOW);
  centerPressed = (digitalRead(CENTER_PIN) == LOW);
  dx = (right ? 1 : 0) - (left ? 1 : 0);
  dy = (down ? 1 : 0) - (up ? 1 : 0);
}

void loop() {
  int8_t dx = 0, dy = 0;
  bool centerPressed = false;
  readInput(dx, dy, centerPressed);

  outgoing.dx = dx;
  outgoing.dy = dy;
  outgoing.centerPressed = centerPressed;

  if (dx != 0 || dy != 0 || centerPressed) {
    Serial.print("Sending -> dx="); Serial.print(dx);
    Serial.print(" dy="); Serial.print(dy);
    Serial.print(" center="); Serial.print(centerPressed);
    Serial.print("  payloadSize="); Serial.println(sizeof(outgoing));

    esp_err_t res = esp_now_send(receiverMAC, (uint8_t *)&outgoing, sizeof(outgoing));
    if (res == ESP_OK) {
      Serial.println("esp_now_send returned: OK");
    } else {
      Serial.print("esp_now_send failed err=0x"); Serial.println(res, HEX);
    }

    delay(100); 
  }
}
