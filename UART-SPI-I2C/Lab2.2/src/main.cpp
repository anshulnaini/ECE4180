#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "ICM_20948.h"

#define LED_PIN 8
#define NUM_LEDS 1
#define SDA_PIN 6
#define SCL_PIN 7
#define IMU_ADDR 0x68 

Adafruit_NeoPixel pixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
ICM_20948_I2C imu;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  pixel.begin();
  pixel.setBrightness(20);
  pixel.show();

  imu.begin(Wire, IMU_ADDR);
  if (imu.status != ICM_20948_Stat_Ok) Serial.println("ICM-20948 not found");
}

void loop() {
  if (!imu.dataReady()) { delay(50); return; }
  imu.getAGMT();


  float x = imu.accX() / 1000.0f;
  float y = imu.accY() / 1000.0f;
  float z = imu.accZ() / 1000.0f;

  const float TILT_TH = 0.45f;   // g
  const float Z_UP    = 1.50f;   // g
  const float Z_DOWN  = 0.50f;   // g

  Serial.print("x: "); Serial.print(x); Serial.print(" g, ");
  Serial.print("y: "); Serial.print(y); Serial.print(" g, ");
  Serial.print("z: "); Serial.print(z); Serial.println(" g");


  if      (z > Z_UP)                 pixel.setPixelColor(0, pixel.Color(255, 255, 0));   // up: yellow
  else if (z < Z_DOWN)               pixel.setPixelColor(0, pixel.Color(0, 255, 255));   // down: cyan
  else if (x >  TILT_TH)             pixel.setPixelColor(0, pixel.Color(255, 0, 0));     // right: red
  else if (x < -TILT_TH)             pixel.setPixelColor(0, pixel.Color(0, 255, 0));     // left:  green
  else if (y >  TILT_TH)             pixel.setPixelColor(0, pixel.Color(0, 0, 255));     // fwd:   blue
  else if (y < -TILT_TH)             pixel.setPixelColor(0, pixel.Color(255, 0, 255));   // back:  magenta
  else                               pixel.setPixelColor(0, pixel.Color(0, 0, 0));       // idle

  pixel.show();
  delay(500);
}
