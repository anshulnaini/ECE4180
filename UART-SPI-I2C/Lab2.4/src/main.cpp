#include <Arduino.h>

#define UP_PIN  4
#define DOWN_PIN  5
#define LEFT_PIN  6
#define RIGHT_PIN  7
#define CENTER_PIN  10




void setColor();

void setup() {

  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  pinMode(CENTER_PIN, INPUT_PULLUP);


  Serial.begin(115200);
}

void loop() {
  int upButtonValue = digitalRead(UP_PIN);
  int downButtonValue = digitalRead(DOWN_PIN);
  int leftButtonValue = digitalRead(LEFT_PIN);
  int rightButtonValue = digitalRead(RIGHT_PIN);
  int centerButtonValue = digitalRead(CENTER_PIN);


  if (( upButtonValue == LOW && leftButtonValue == LOW)) {
    Serial.println("up - left");
    delay(200);
  } else if (( upButtonValue == LOW && rightButtonValue == LOW)) {
    Serial.println("up - right");
    delay(200);
  } else if ((downButtonValue == LOW && leftButtonValue == LOW)) {
    Serial.println("down - left");
    delay(200);
  } else if ((downButtonValue == LOW && rightButtonValue == LOW)) {
    Serial.println("down - right");
    delay(200);
  } else if (upButtonValue == LOW) {
    Serial.println("up");
    delay(200);
  } else if (downButtonValue == LOW) {
    Serial.println("down");
    delay(200);
  } else if (leftButtonValue == LOW) {
    Serial.println("left");
    delay(200);
  } else if (rightButtonValue == LOW) {
    Serial.println("right");
    delay(200);
  } else if (centerButtonValue == LOW) {
    Serial.println("center");
    delay(200);
  }


}