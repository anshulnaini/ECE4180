//ESP32 as a station 
//Adapted by T Goo for ECE 4180

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h> // need to download Arduino_JSON lib

/*******************************
TODO: Setup and connect to Wifi hotspot. HIGHLY recommend you change from default name
************************************/
//for iPhone ssid: settings -> general -> About to change ssid of hotspot
//for Android: https://www.apple.com/
const char* ssid = "Anshul";
const char* password = "password"; 

// World Time API URL
// **************************
// Do not change these three lines! The esp32 will attempt to request data to this link (API key-less) every 5 seconds. You should expect a successful response every one out of five requests.
const char* serverUrl = "http://worldtimeapi.org/api/timezone/America/New_York";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;  // every 5 seconds

void setup() {
  Serial.begin(115200);
  delay(3000); //delay to allow serial monitor to open
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  Serial.println(ssid);
  Serial.println(password);

  /*******************************
  TODO: Edit the while loop condition to check if you have connected to the Wifi hotspot. EXIT the loop once you have connected. 
  Hint: Check the Wifi.status() and reference the enums!
  ************************************/
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected! localIP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);

      /*************************
      TODO: change the header to your ESP32 number
      http.addHeader("ESP32-1", "ESP32-WorldTimeLab");
      *************************/
      http.addHeader("ESP32-4180", "ESP32-WorldTimeLab");

      /*************************
      TODO: Generate a GET request with the http object. Check http object lib.
      What does the GET request return? 
      https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp
      *************************/
      int httpCode = http.GET();  // returns HTTP status code (e.g., 200 for OK)

      /*************************
      TODO: What is a "successful" http code? 
      Change the if condition to check for a succesful code.
      *************************/
      if (httpCode == HTTP_CODE_OK) {  // 200 OK
        String payload = http.getString(); //get the payload of the http object
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        Serial.print("Payload: ");
        Serial.println(payload);

        /***********************************
        TODO: Parse through payload via JSON obj. Use/download Aduino_JSON lib to parse through the payload. Extract the date time field. Print out the date and time on seperate lines. 
        **************************************/
        JSONVar obj = JSON.parse(payload);
        if (JSON.typeof(obj) != "undefined") {
          String datetime = (const char*)obj["datetime"]; // e.g. "2025-10-16T12:34:56.789012-04:00"

          int tIndex = datetime.indexOf('T');
          String dateStr = (tIndex > 0) ? datetime.substring(0, tIndex) : "";
          String timeStr = (tIndex > 0) ? datetime.substring(tIndex + 1) : "";

          // Trim sub-second part and timezone offset for clean time display
          int cut = timeStr.indexOf('.');
          if (cut == -1) cut = timeStr.indexOf('+');
          if (cut == -1) cut = timeStr.indexOf('-');
          if (cut != -1) timeStr = timeStr.substring(0, cut);

          Serial.println(dateStr); // Date on its own line
          Serial.println(timeStr); // Time on its own line
        } else {
          Serial.println("Failed to parse JSON");
        }
      } else {
        Serial.print("Failed, http repsonse error code: ");
        Serial.println(httpCode);
        String payload = http.getString();
        Serial.println(httpCode);
      }
      http.end();
    } else {
      Serial.println("WiFi disconnected");
    }
    lastTime = millis();
  }
}
