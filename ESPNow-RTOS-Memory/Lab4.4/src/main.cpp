#include <Arduino.h>
#include <Goldelox_Serial_4DLib.h>
#include <Preferences.h> 

// Pin definitions
#define LCD_RX 21
#define LCD_TX 20

// LCD parameters
const uint16_t LCD_WIDTH  = 128;
const uint16_t LCD_HEIGHT = 128;
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_RED   = 0xF800;

// Ball parameters
int16_t  ballX;
int16_t  ballY;
int8_t   velX;
int8_t   velY;
uint8_t  ballR = 6;
uint16_t ballColor = COLOR_RED;
uint16_t bgColor   = COLOR_BLACK;


const uint16_t periodMs = 25;  

HardwareSerial LCDUart(1);
Goldelox_Serial_4DLib Display(&LCDUart);
Preferences prefs;


void drawBall(uint16_t color);
void eraseBall();
void saveState();
void loadState();

void drawBall(uint16_t color) {
  uint16_t x = constrain(ballX, ballR, LCD_WIDTH - 1 - ballR);
  uint16_t y = constrain(ballY, ballR, LCD_HEIGHT - 1 - ballR);
  Display.gfx_CircleFilled(x, y, ballR, color);
}

void eraseBall() {
  drawBall(bgColor);
}

void saveState() {
  prefs.begin("ballState", false);
  prefs.putInt("x", ballX);
  prefs.putInt("y", ballY);
  prefs.putInt("vx", velX);
  prefs.putInt("vy", velY);
  prefs.end();
}

void loadState() {
  prefs.begin("ballState", true);
  ballX = prefs.getInt("x", LCD_WIDTH / 2);
  ballY = prefs.getInt("y", LCD_HEIGHT / 2);
  velX  = prefs.getInt("vx", 3);
  velY  = prefs.getInt("vy", 2);
  prefs.end();
}

void setup() {
  Serial.begin(115200);
  LCDUart.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
  delay(3200);

  loadState();

  Display.gfx_Cls();
  drawBall(ballColor);

  Serial.println("Bouncing ball with NVS state restore started.");
}

void loop() {
  static uint32_t lastStep = 0;

  if (millis() - lastStep >= periodMs) {
    lastStep += periodMs;

    eraseBall();

    // Update position
    ballX += velX;
    ballY += velY;

    // Check wall collisions
    if (ballX - ballR <= 0 || ballX + ballR >= LCD_WIDTH - 1) {
      velX = -velX;
      ballX = constrain(ballX, ballR, LCD_WIDTH - 1 - ballR);
    }
    if (ballY - ballR <= 0 || ballY + ballR >= LCD_HEIGHT - 1) {
      velY = -velY;
      ballY = constrain(ballY, ballR, LCD_HEIGHT - 1 - ballR);
    }

    drawBall(ballColor);
    saveState(); 
  }
}
