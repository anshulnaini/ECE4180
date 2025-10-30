#include <Arduino.h>
#include <Wire.h>
#include <Goldelox_Serial_4DLib.h>
#include "ICM_20948.h"


//  Pin map 
#define PIN_POT   0
#define PIN_AIN1  23
#define PIN_AIN2  22
#define PIN_PWMA  18

#define LCD_RX_PIN      8          // uLCD RX
#define LCD_TX_PIN      1          // uLCD TX
#define I2C_SDA_PIN     21         // IMU SDA
#define I2C_SCL_PIN     20         // IMU SCL
#define IMU_ADDR        0x68

//  LCD / Display constants 
static const uint16_t COLOR_BLACK = 0x0000;
static const uint16_t COLOR_RED   = 0xF800;

//  FreeRTOS primitives 
SemaphoreHandle_t lcdMutex;
SemaphoreHandle_t speedMutex;
EventGroupHandle_t egSystem;

#define CRASH_BIT   (1 << 0)

//  Shared state 
volatile uint8_t g_targetSpeedPct = 0;   // 0..100
volatile bool    g_crashed = false;

//  LCD objects 
HardwareSerial LCDUart(1);
Goldelox_Serial_4DLib Display(&LCDUart);

//  IMU object 
ICM_20948_I2C imu;

//  PWM (LEDC) settings 
static const int PWM_CHANNEL = 0;
static const int PWM_FREQ_HZ = 20000;  
static const int PWM_RES_BITS = 8;     // duty 0..255

// Display helpers (mutex-protected)
void ulcd_safe_begin() {
  xSemaphoreTake(lcdMutex, portMAX_DELAY);
  Display.gfx_Cls();                 // clear
  xSemaphoreGive(lcdMutex);
}

void ulcd_safe_showSpeed(uint8_t pct) {
  xSemaphoreTake(lcdMutex, portMAX_DELAY);

  Display.gfx_Cls();

  Display.txt_MoveCursor(0, 0);
  Display.putstr((char*)"Vehicle Speed:");
  Display.txt_MoveCursor(1, 0);
  char buf[32];
  snprintf(buf, sizeof(buf), "%u%%", pct);
  Display.putstr(buf);
  xSemaphoreGive(lcdMutex);
}

void ulcd_safe_showCrash() {
  xSemaphoreTake(lcdMutex, portMAX_DELAY);
  Display.gfx_Cls();
  Display.txt_MoveCursor(0, 0);
  Display.putstr((char*)"*** CRASH ***");
  Display.txt_MoveCursor(1, 0);
  Display.putstr((char*)"MOTOR STOPPED");
  xSemaphoreGive(lcdMutex);
}

// Motor control helpers
void motor_stop_immediate() {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  analogWrite(PIN_PWMA, 0);
}

void motor_set_speed_pct(uint8_t pct) {
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, LOW);
  int duty = map(pct, 0, 100, 0, 255);
  analogWrite(PIN_PWMA, duty);
}


// Task: Read potentiometer
void PotTask(void* pv) {
  // Simple IIR low-pass and 1% deadband
  const float alpha = 0.15f;
  float filt = 0.0f;
  uint8_t lastPct = 255; // force first update

  for (;;) {
    int raw = analogRead(PIN_POT);       // 0..4095
    float pct = (raw / 4095.0f) * 100.0f;
    filt = alpha * pct + (1.0f - alpha) * filt;

    uint8_t sp = (uint8_t)roundf(constrain(filt, 0.0f, 100.0f));

    // Deadband
    if (lastPct == 255 || abs((int)sp - (int)lastPct) >= 1) {
      xSemaphoreTake(speedMutex, portMAX_DELAY);
      g_targetSpeedPct = sp;
      xSemaphoreGive(speedMutex);
      lastPct = sp;
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // ~50 Hz
  }
}

// Task: Motor control
void MotorTask(void* pv) {
  for (;;) {
    // If crash: stop immediately and hold
    EventBits_t bits = xEventGroupGetBits(egSystem);
    if (bits & CRASH_BIT) {
      motor_stop_immediate();
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    //  the latest shared target speed
    xSemaphoreTake(speedMutex, portMAX_DELAY);
    uint8_t sp = g_targetSpeedPct;
    xSemaphoreGive(speedMutex);

    static uint8_t last_sp = 255;  
    if (last_sp != sp) {
        Serial.print("[MotorTask] New speed request: ");
        Serial.println(sp);
        last_sp = sp;
    }

    motor_set_speed_pct(sp);
    vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz update
  }
}

//  Task: Display (uLCD) 
void DisplayTask(void* pv) {
  uint8_t lastShown = 255;
  bool lastCrash = false;

  for (;;) {
    // Check crash state
    EventBits_t bits = xEventGroupGetBits(egSystem);
    bool crashed = (bits & CRASH_BIT);

    if (crashed && !lastCrash) {
      ulcd_safe_showCrash();
      lastCrash = true;
    }

    if (!crashed) {
      // Show speed at a modest rate
      xSemaphoreTake(speedMutex, portMAX_DELAY);
      uint8_t sp = g_targetSpeedPct;
      xSemaphoreGive(speedMutex);

      if (lastShown == 255 || abs((int)sp - (int)lastShown) >= 1) {
        ulcd_safe_showSpeed(sp);
        lastShown = sp;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(200)); // ~5 Hz UI refresh
  }
}

// Task: IMU / Crash detection 
void IMUTask(void* pv) {

  const float G_TRIGGER_ABS   = 1.4f;  // absolute violent shake
  const float G_TRIGGER_DELTA = .8f;  // relative spike vs baseline
  const int   CONSEC_REQ      = 2;     // hysteresis: # of consecutive samples

  float aBaseline = 1.0f;              // start near 1g
  const float k    = 0.98f;            // baseline IIR factor (slow drift)
  int tripCount    = 0;

  for (;;) {
    // If already crashed, just idle a bit to avoid I2C spam
    EventBits_t bits = xEventGroupGetBits(egSystem);
    if (bits & CRASH_BIT) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    if (!imu.dataReady()) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    imu.getAGMT();

    float ax = imu.accX() / 1000.0f;
    float ay = imu.accY() / 1000.0f;
    float az = imu.accZ() / 1000.0f;

    // Magnitude
    float amag = sqrtf(ax*ax + ay*ay + az*az);

    aBaseline = k * aBaseline + (1.0f - k) * amag;
    aBaseline = constrain(aBaseline, 0.8f, 1.2f);

    bool violent = (amag >= G_TRIGGER_ABS) || ((amag - aBaseline) >= G_TRIGGER_DELTA);
    if (violent) {
      tripCount++;
    } else {
      tripCount = max(0, tripCount - 1);
    }

    if (tripCount >= CONSEC_REQ) {
      // Signal crash
      xEventGroupSetBits(egSystem, CRASH_BIT);
      // Immediate motor stop for safety
      motor_stop_immediate();
      // Update LCD immediately
      ulcd_safe_showCrash();
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // ~100 Hz sample loop
  }
}

void setup() {
  // Serial
  Serial.begin(115200);

  // Direction pins
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);

  // Basic PWM setup on PWMA
  pinMode(PIN_PWMA, OUTPUT);
  analogWrite(PIN_PWMA, 0);  // just initialize to 0 duty


  // ADC
  analogReadResolution(12); // 0..4095

  // uLCD UART
  LCDUart.begin(9600, SERIAL_8N1, LCD_RX_PIN, LCD_TX_PIN);
  delay(3200); 
  // Init FreeRTOS sync
  lcdMutex   = xSemaphoreCreateMutex();
  speedMutex = xSemaphoreCreateMutex();
  egSystem   = xEventGroupCreate();

  // LCD start screen
  ulcd_safe_begin();
  ulcd_safe_showSpeed(0);

  // IMU
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  imu.begin(Wire, IMU_ADDR);
  if (imu.status != ICM_20948_Stat_Ok) {
    Serial.println("ICM-20948 not found");
  } else {
    Serial.println("ICM-20948 ready");
  }

  // Create tasks
  xTaskCreatePinnedToCore(PotTask,    "PotTask",    4096, NULL, 2,  NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(MotorTask,  "MotorTask",  4096, NULL, 3,  NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(DisplayTask,"DisplayTask",4096, NULL, 1,  NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(IMUTask,    "IMUTask",    6144, NULL, 4,  NULL, ARDUINO_RUNNING_CORE);

  Serial.println("FreeRTOS Crash Detection system started.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
