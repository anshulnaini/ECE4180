#include <Arduino.h>
#include <Wire.h>
#include "ICM_20948.h"               
#include <Goldelox_Serial_4DLib.h>    


#define IMU_ADDR 0x68

  



// (2) One shared I2C bus for BOTH keypad and IMU
//    >>> Pick pins that are FREE on your ESP32 and DO NOT overlap with the uLCD UART pins. <<<
#define SDA_PIN 21
#define SCL_PIN 22

// (3) uLCD UART pins (must NOT reuse SDA/SCL)
#define LCD_RX 17   // ESP32 RX  from uLCD TX
#define LCD_TX 16   // ESP32 TX  to   uLCD RX

const uint16_t LCD_WIDTH   = 128;
const uint16_t LCD_HEIGHT  = 128;
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_RED   = 0xF800;
const uint16_t COLOR_BLUE  = 0x001F;
const uint16_t COLOR_GREEN = 0x07E0;
const uint16_t COLOR_YELLOW= 0xFFE0;
const uint16_t COLORS[]    = { COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW };

int16_t  spriteX = LCD_WIDTH / 2;
int16_t  spriteY = LCD_HEIGHT / 2;
uint8_t  margin  = 14;            
uint16_t spriteColor = COLOR_RED;
uint16_t bgColor      = COLOR_BLACK;
uint8_t  stepSize     = 3;

const float TILT_TH = 0.45f;      
const int BOX_W = 100;
const int BOX_H = 18;

int16_t prevX = -1, prevY = -1;

HardwareSerial LCDUart(1);
Goldelox_Serial_4DLib Display(&LCDUart);
ICM_20948_I2C imu;

bool uLCD_probe(Stream &s, uint16_t cmd, uint32_t timeout_ms = 300) {
  while (s.available()) s.read();
  s.write(uint8_t(cmd >> 8));  s.write(uint8_t(cmd & 0xFF));
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms) if (s.available()) return s.read() == 0x06;
  return false;
}

const char* directionLabel(int8_t dx, int8_t dy) {
  if (dx == 0 && dy == 0) return "IDLE";
  if (dx == 0 && dy < 0)  return "UP";
  if (dx == 0 && dy > 0)  return "DOWN";
  if (dx < 0  && dy == 0) return "LEFT";
  if (dx > 0  && dy == 0) return "RIGHT";
  if (dx < 0  && dy < 0)  return "UP-LEFT";
  if (dx > 0  && dy < 0)  return "UP-RIGHT";
  if (dx < 0  && dy > 0)  return "DOWN-LEFT";
  return "DOWN-RIGHT";            
}

void clearBoxAt(int16_t x, int16_t y) {
  x = constrain(x, 0, (int16_t)LCD_WIDTH  - 1);
  y = constrain(y, 0, (int16_t)LCD_HEIGHT - 1);
  int16_t x2 = min<int16_t>(LCD_WIDTH  - 1, x + BOX_W);
  int16_t y2 = min<int16_t>(LCD_HEIGHT - 1, y + BOX_H);
  Display.gfx_RectangleFilled(x, y, x2, y2, COLOR_BLACK);
}

void drawLabelAt(int16_t x, int16_t y, const char* text, uint16_t color) {
  x = constrain(x, margin, (int16_t)LCD_WIDTH  - 1 - margin);
  y = constrain(y, margin, (int16_t)LCD_HEIGHT - 1 - margin);
  Display.txt_FGcolour(color);
  Display.gfx_MoveTo(x, y);
  Display.putstr((char*)text);   
}

void changeSpriteColor() {
  static uint8_t idx = 0;
  idx = (idx + 1) % (sizeof(COLORS) / sizeof(COLORS[0]));
  spriteColor = COLORS[idx];
}

void setup() {
  Serial.begin(115200);
  delay(100);

  LCDUart.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
  delay(1200);
  if (uLCD_probe(LCDUart, 0xFFD7)) Serial.println("uLCD OK");
  else                              Serial.println("uLCD no ACK");
  Display.gfx_Cls();

  Display.txt_FontID(2);
  Display.txt_FGcolour(spriteColor);
  Display.txt_BGcolour(COLOR_BLACK);
  Display.txt_Opacity(0);          // transparent

  drawLabelAt(spriteX, spriteY, "IDLE", spriteColor);
  prevX = spriteX; prevY = spriteY;

  // IMU
  Wire.begin(SDA_PIN, SCL_PIN);
  if (imu.begin(Wire, IMU_ADDR) != ICM_20948_Stat_Ok) Serial.println("ICM-20948 not found");
  else                                                Serial.println("ICM-20948 ready");
}

void loop() {
  if (!imu.dataReady()) { delay(20); return; }
  imu.getAGMT();

  float ax = imu.accX() / 1000.0f;
  float ay = imu.accY() / 1000.0f;

  int8_t dx = 0, dy = 0;
  if (ax >  TILT_TH) dx =  1;
  if (ax < -TILT_TH) dx = -1;
  if (ay >  TILT_TH) dy = -1;     // forward = up
  if (ay < -TILT_TH) dy =  1;     // back = down

  const char* newLabel = directionLabel(dx, dy);

  spriteX += dx * stepSize;
  spriteY += dy * stepSize;
  int16_t clampedX = constrain(spriteX, margin, (int16_t)LCD_WIDTH  - 1 - margin);
  int16_t clampedY = constrain(spriteY, margin, (int16_t)LCD_HEIGHT - 1 - margin);

  if (clampedX != spriteX || clampedY != spriteY) changeSpriteColor();
  spriteX = clampedX; spriteY = clampedY;

  clearBoxAt(prevX, prevY);
  drawLabelAt(spriteX, spriteY, newLabel, spriteColor);
  prevX = spriteX; prevY = spriteY;

  delay(40);
}
