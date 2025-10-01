#include <Arduino.h>
#include <Goldelox_Serial_4DLib.h>


#define UP_PIN     4
#define DOWN_PIN   5
#define LEFT_PIN   6
#define RIGHT_PIN  7
#define CENTER_PIN 10


#define LCD_RX 21
#define LCD_TX 20

HardwareSerial LCDUart(1);               
Goldelox_Serial_4DLib Display(&LCDUart);     

const uint16_t LCD_WIDTH  = 128;
const uint16_t LCD_HEIGHT = 128;
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_WHITE = 0xFFFF;
const uint16_t COLOR_RED   = 0xF800;
const uint16_t COLOR_BLUE  = 0x001F;
const uint16_t COLOR_GREEN = 0x07E0;

int16_t  ballX = LCD_WIDTH / 2;
int16_t  ballY = LCD_HEIGHT / 2;
uint8_t  ballR = 6;
uint16_t ballColor = COLOR_RED;
uint16_t bgColor   = COLOR_BLACK;
uint8_t  stepSize  = 3;        // pixels per update


void drawBall(uint16_t color) {
  // keep ball fully on-screen
  uint16_t x = constrain(ballX, ballR, LCD_WIDTH  - 1 - ballR);
  uint16_t y = constrain(ballY, ballR, LCD_HEIGHT - 1 - ballR);
  Display.gfx_CircleFilled(x, y, ballR, color);
}

void eraseBall() {
  drawBall(bgColor);
}


void readInput(int8_t &dx, int8_t &dy) {
  int up    = (digitalRead(UP_PIN)    == LOW);
  int down  = (digitalRead(DOWN_PIN)  == LOW);
  int left  = (digitalRead(LEFT_PIN)  == LOW);
  int right = (digitalRead(RIGHT_PIN) == LOW);
  dx = (right ? 1 : 0) - (left ? 1 : 0);
  dy = (down  ? 1 : 0) - (up   ? 1 : 0);
}

void centerPressedAction() {
  ballColor = (ballColor == COLOR_RED) ? COLOR_BLUE : COLOR_RED;
  eraseBall();
  ballX = LCD_WIDTH / 2;
  ballY = LCD_HEIGHT / 2;
  drawBall(ballColor);
}

void setup() {
  // Buttons
  pinMode(UP_PIN,     INPUT_PULLUP);
  pinMode(DOWN_PIN,   INPUT_PULLUP);
  pinMode(LEFT_PIN,   INPUT_PULLUP);
  pinMode(RIGHT_PIN,  INPUT_PULLUP);
  pinMode(CENTER_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  LCDUart.begin(9600, SERIAL_8N1, LCD_RX, LCD_TX);
  delay(3200);              


  // Clear screen and draw initial ball
  Display.gfx_Cls();
  drawBall(ballColor);
}

void loop() {
  static uint32_t lastStep = 0;
  const uint16_t periodMs = 25;    // ~40 Hz

  if (millis() - lastStep >= periodMs) {
    lastStep += periodMs;

    int8_t dx = 0, dy = 0;
    readInput(dx, dy);

    if (digitalRead(CENTER_PIN) == LOW) {
      centerPressedAction();
      delay(180);                  // simple debounce
      return;
    }

    if (dx != 0 || dy != 0) {
      eraseBall();
      ballX += dx * stepSize;
      ballY += dy * stepSize;
      // clamp to keep the ball fully on-screen
      ballX = constrain(ballX, ballR, LCD_WIDTH  - 1 - ballR);
      ballY = constrain(ballY, ballR, LCD_HEIGHT - 1 - ballR);
      drawBall(ballColor);
    }
  }
}
