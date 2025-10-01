#include <Wire.h>
#include "Adafruit_MPR121.h"
#include <HardwareSerial.h>

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif


Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t lasttouched = 0;
uint16_t currtouched = 0;


int8_t keymap[12] = {0,1,2,3,4,5,6,7,8,9,-3,-3}; 


String keypadBuf = "";

// UART to Diego
HardwareSerial ToDiego(1);
const long DIEGO_BAUD = 9600;
const int MY_UART_RX = 16;
const int MY_UART_TX = 17; //  Diego RX 18


void sendNumberToDiego(uint16_t num) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%u\n", (unsigned)num);
  ToDiego.print(buf); 
  Serial.print("[TX→Diego] "); Serial.print(buf);
}

void handleKey(int8_t key) {
  if (key >= 0 && key <= 9) {
    if (keypadBuf.length() < 4) {
      keypadBuf += char('0' + key);
      Serial.print("Keypad: "); Serial.println(keypadBuf);
    } else {
      uint16_t val = (uint16_t)keypadBuf.toInt();
      sendNumberToDiego(val);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  delay(200);

  Serial.println("\n=== Keypad → Diego UART sender ===");

  // UART to Diego
  // begin(baud, config, rxPin, txPin)
  ToDiego.begin(DIEGO_BAUD, SERIAL_8N1, MY_UART_RX, MY_UART_TX);
  Serial.println("UART ready (9600 -> Diego). Connect TX17 -> Diego RX18 and common GND.");


  Wire.setPins(21, 22); 
  Serial.println("Initializing MPR121...");
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring!");
    while (1) { delay(100); }
  }
  cap.setAutoconfig(true);
  Serial.println("MPR121 found. Use electrodes 0-9 for digits, 10='*' clear, 11='#' send.");

  lasttouched = cap.touched();
}

void loop() {
  currtouched = cap.touched();


  for (uint8_t i = 0; i < 12; i++) {
    bool now = currtouched & _BV(i);
    bool was = lasttouched & _BV(i);
    if (now && !was) {
      int8_t mapped = keymap[i];
      if (mapped >= -2) handleKey(mapped);
    }
  }
  lasttouched = currtouched;




  delay(30); 
}
