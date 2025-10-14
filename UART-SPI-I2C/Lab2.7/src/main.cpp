#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"        // Capacitive keypad
#include "ICM_20948.h"              // SparkFun ICM-20948 (IMU)
#include <Goldelox_Serial_4DLib.h>  // 4D Systems uLCD (Goldelox)


#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "ICM_20948.h"
#include <Goldelox_Serial_4DLib.h>

#define BOARD_NUM 95

#define SDA_PIN 21
#define SCL_PIN 22

#define LCD_RX 17
#define LCD_TX 16

#define IMU_ADDR 0x68

enum TiltDir : int8_t { T_IDLE=0, T_UP, T_DOWN, T_LEFT, T_RIGHT };
const TiltDir EXPECTED_TILTS[3] = { T_UP, T_DOWN, T_LEFT };

Adafruit_MPR121 cap = Adafruit_MPR121();
ICM_20948_I2C   imu;

HardwareSerial LCDUart(1);
Goldelox_Serial_4DLib Display(&LCDUart);

bool uLCD_probe(Stream &s, uint16_t cmd, uint32_t timeout_ms = 300) {
  while (s.available()) s.read();
  s.write(uint8_t(cmd >> 8));
  s.write(uint8_t(cmd & 0xFF));
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms) {
    if (s.available()) return s.read() == 0x06;
  }
  return false;
}

void uartHandshakeULCD() {
  LCDUart.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
  delay(1200);

  size_t w = LCDUart.write((uint8_t)0x55);
  LCDUart.flush();
  Serial.printf("[uLCD] Sent autobaud 0x55, bytesWritten=%u\n", (unsigned)w);

  bool ok = uLCD_probe(LCDUart, 0xFFD7, 500);
  Serial.println(ok ? "[uLCD] ACK received" : "[uLCD] No ACK");

  if (!ok) {
    Serial.println("[uLCD] Display did not ACK. Check baud, TX/RX, GND, and that 0x55 was sent soon after power.");
  }
}

const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_RED   = 0xF800;
const uint16_t COLOR_GREEN = 0x07E0;
const uint16_t COLOR_WHITE = 0xFFFF;
const uint16_t COLOR_YELL  = 0xFFE0;

uint16_t lastTouched = 0;
uint16_t currTouched = 0;

int8_t keymap[12] = {0,1,2,3,4,5,6,7,8,9,10,11};

enum Stage { ST_PIN, ST_TILTS, ST_UNLOCKED };
Stage stage = ST_PIN;

String targetPin;
String typedPin = "";

TiltDir tiltBuf[3]      = { T_IDLE, T_IDLE, T_IDLE };
uint8_t tiltCount       = 0;

const float TILT_TH = 0.45f;
bool tiltArmed = true;

String makeTargetPin(uint16_t num) {
  char buf[5];
  snprintf(buf, sizeof(buf), "%04u", (unsigned)num);
  return String(buf);
}

void lcdClear()                { Display.gfx_Cls(); }
void lcdTextColor(uint16_t c)  { Display.txt_FGcolour(c); }
void lcdMoveTo(int x, int y)   { Display.gfx_MoveTo(x, y); }
void lcdPut(const String& s)   { Display.putstr((char*)s.c_str()); }

void drawArrow(int x, int y, TiltDir d, uint16_t color) {
  const int L  = 22;
  const int H  = 8;
  const int TH = 3;

  auto fillHLine = [&](int x1, int x2, int yy) {
    if (x2 < x1) { int t = x1; x1 = x2; x2 = t; }
    Display.gfx_Line(x1, yy, x2, yy, color);
  };

  auto fillVLine = [&](int xx, int y1, int y2) {
    if (y2 < y1) { int t = y1; y1 = y2; y2 = t; }
    Display.gfx_Line(xx, y1, xx, y2, color);
  };

  switch (d) {
    case T_UP: {
      Display.gfx_RectangleFilled(x-TH, y-L, x+TH, y, color);
      for (int i = 0; i <= H; ++i) {
        int half = H - i;
        fillHLine(x - half, x + half, y - L - i);
      }
    } break;

    case T_DOWN: {
      Display.gfx_RectangleFilled(x-TH, y, x+TH, y+L, color);
      for (int i = 0; i <= H; ++i) {
        int half = H - i;
        fillHLine(x - half, x + half, y + L + i);
      }
    } break;

    case T_LEFT: {
      Display.gfx_RectangleFilled(x-L, y-TH, x, y+TH, color);
      for (int i = 0; i <= H; ++i) {
        int half = H - i;
        fillVLine(x - L - i, y - half, y + half);
      }
    } break;

    case T_RIGHT: {
      Display.gfx_RectangleFilled(x, y-TH, x+L, y+TH, color);
      for (int i = 0; i <= H; ++i) {
        int half = H - i;
        fillVLine(x + L + i, y - half, y + half);
      }
    } break;

    default: break;
  }
}

const char* tiltName(TiltDir d) {
  switch (d) {
    case T_UP:    return "UP";
    case T_DOWN:  return "DOWN";
    case T_LEFT:  return "LEFT";
    case T_RIGHT: return "RIGHT";
    default:      return "IDLE";
  }
}

TiltDir readTiltGesture() {
  if (!imu.dataReady()) return T_IDLE;
  imu.getAGMT();

  float ax = imu.accX() / 1000.0f;
  float ay = imu.accY() / 1000.0f;

  TiltDir d = T_IDLE;
  if      (ay >  TILT_TH) {
    d = T_UP;
    Serial.println("[IMU] Detected UP tilt");
  } else if (ay < -TILT_TH) {
    d = T_DOWN;
    Serial.println("[IMU] Detected DOWN tilt");
  } else if (ax >  TILT_TH) {
    d = T_RIGHT;
    Serial.println("[IMU] Detected RIGHT tilt");
  } else if (ax < -TILT_TH) {
    d = T_LEFT;
    Serial.println("[IMU] Detected LEFT tilt");
  }

  if (tiltArmed && d != T_IDLE) { tiltArmed = false; return d; }
  if (!tiltArmed && d == T_IDLE) tiltArmed = true;
  return T_IDLE;
}

void drawHeaderLocked() {
  Serial.println("[UI] Drawing header");
  Display.gfx_RectangleFilled(0, 0, 127, 18, COLOR_BLACK);
  lcdMoveTo(4, 4);
  lcdTextColor(stage == ST_UNLOCKED ? COLOR_GREEN : COLOR_RED);
  lcdPut(stage == ST_UNLOCKED ? "UNLOCKED" : "LOCKED");
}

void drawPinLine() {
  Display.gfx_RectangleFilled(0, 22, 127, 40, COLOR_BLACK);
  lcdMoveTo(4, 26);
  lcdTextColor(COLOR_WHITE);
  lcdPut("PIN: ");
  lcdPut(typedPin);
}

void drawTiltLine() {
  Display.gfx_RectangleFilled(0, 44, 127, 62, COLOR_BLACK);
  lcdMoveTo(4, 48);
  lcdTextColor(COLOR_WHITE);
  lcdPut("TILTS: ");
  for (uint8_t i=0; i<tiltCount; ++i) {
    lcdPut(i? ", " : "");
    lcdPut(tiltName(tiltBuf[i]));
  }

  Display.gfx_RectangleFilled(0, 66, 127, 127, COLOR_BLACK);
  int x = 16, y = 92;
  for (uint8_t i=0; i<tiltCount; ++i) {
    drawArrow(x, y, tiltBuf[i], COLOR_YELL);
    x += 40;
  }
}

void renderAll() {
  drawHeaderLocked();
  drawPinLine();
  drawTiltLine();
}

void resetAll(bool keepPin = false) {
  if (!keepPin) typedPin = "";
  tiltCount = 0;
  for (auto &t : tiltBuf) t = T_IDLE;
  if (stage != ST_UNLOCKED) stage = (typedPin.length() < 4 ? ST_PIN : ST_TILTS);
  renderAll();
}

void handleEnter() {
  Serial.println("[KEYPAD] Enter pressed");
  if (stage == ST_PIN) {
    if (typedPin.length() == 4) {
      stage = ST_TILTS;
    }
  } else if (stage == ST_TILTS) {
    bool pinOk = (typedPin == targetPin);
    bool tiltOk = (tiltCount == 3
                   && tiltBuf[0] == EXPECTED_TILTS[0]
                   && tiltBuf[1] == EXPECTED_TILTS[1]
                   && tiltBuf[2] == EXPECTED_TILTS[2]);
    if (pinOk && tiltOk) {
      stage = ST_UNLOCKED;
    } else {
      stage = ST_PIN;
      typedPin = "";
      tiltCount = 0;
      for (auto &t : tiltBuf) t = T_IDLE;
    }
  }
  renderAll();
}

void handleDelete() {
  if (stage == ST_PIN) {
    if (typedPin.length() > 0) typedPin.remove(typedPin.length()-1);
  } else if (stage == ST_TILTS) {
    if (tiltCount > 0) {
      tiltCount--;
      tiltBuf[tiltCount] = T_IDLE;
    }
  }
  renderAll();
}

void handleDigit(uint8_t d) {
  if (stage == ST_PIN) {
    if (typedPin.length() < 4) {
      typedPin += char('0' + d);
      if (typedPin.length() == 4) {
      }
      renderAll();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  targetPin = makeTargetPin(BOARD_NUM);

  uartHandshakeULCD();

  Display.gfx_Cls();
  Display.txt_BGcolour(0x0000);
  Display.txt_Opacity(0);
  Display.txt_FGcolour(0xFFFF);
  Display.gfx_MoveTo(4, 4);
  Display.putstr("UART OK?");

  Serial.println("[uLCD] Wrote text 'UART OK?' via library. If you see it on the screen, UART is good.");

  delay(3000);
  Display.gfx_Cls();
  Display.txt_FontID(2);
  Display.txt_BGcolour(COLOR_BLACK);
  Display.txt_Opacity(0);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!cap.begin(0x5A)) {
    lcdMoveTo(2, 2); lcdTextColor(COLOR_RED);
    lcdPut("MPR121 not found!");
    while (1) { delay(100); }
  }
  cap.setAutoconfig(true);
  lastTouched = cap.touched();

  if (imu.begin(Wire, IMU_ADDR) != ICM_20948_Stat_Ok) {
    lcdMoveTo(2, 20); lcdTextColor(COLOR_RED);
    lcdPut("ICM-20948 not found");
  }

  renderAll();
}

void loop() {
  currTouched = cap.touched();
  for (uint8_t i = 0; i < 12; i++) {
    bool now = currTouched & (1 << i);
    bool was = lastTouched & (1 << i);
    if (now && !was) {
      int8_t key = keymap[i];
      if (key >= 0 && key <= 9)         handleDigit((uint8_t)key);
      else if (key == 10)               handleEnter();
      else if (key == 11)               handleDelete();
    }
  }
  lastTouched = currTouched;

  if (stage == ST_TILTS && tiltCount < 3) {
    TiltDir g = readTiltGesture();
    if (g != T_IDLE) {
      if (tiltCount == 0 || tiltBuf[tiltCount-1] != g) {
        tiltBuf[tiltCount++] = g;
        renderAll();
      }
    }
  }

  delay(20);
}

