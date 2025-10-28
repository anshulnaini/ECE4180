//ESP32 as a Soft AP
//Adapted by T Goo for ECE 4180

#include <WiFi.h>

/*********************************
1.TODO: Generate an ssid (ESP32-<your ESP number>)
and password
**********************************/
const char* ssid     = "ESP32-4180";
const char* password = "ece4180pass";

//set web server port number to 80
WiFiServer server(80);

//var to store the HTTP request data 
String header;

// output state to store status of HTML and GPIO 
String output7State = "off";


void setup() {
  Serial.begin(115200);
  delay(3000);

  /*****************************
  1.5 TODO: setup GPIO 7 as an output
  ****************************/
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  Serial.println("\nSetting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  /**********************************

  2. TODO: Print out the IP address of the ESP32.Read this documentation to get the IP address.

  ESP32 acts as a Wifi AP that constantly broadcasts an html. Stations that connect to the server and visit the correlating IP address can get access to this HTML.

  Note that ESP32 is acting as a SoftAP (Software Access Point). 

  https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html
  **************************************/
  Serial.print("AP IP address: ");  
  
  IPAddress IP = WiFi.softAPIP();
  while (!IP) {
    Serial.println("Populate and print out IPAddress");
    delay(5000);
    IP = WiFi.softAPIP();
  }
  Serial.println(IP);

  server.begin(); //Starts server
}


void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) { // If a new client connects,
    Serial.println("New Client.");   
    String currentLine = "";  // make a String to hold incoming data from client
    while (client.connected()) {   // loop while the client's connected
      if (client.available()) {    // if there's bytes to read from client
        char c = client.read();    // read a byte, then
        header += c;              //append the byte to header var
        if (c == '\n') {          // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers ALWAYS start with a response code (e.g. HTTP/1.1 200 OK) and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            /***********************************
            3. TODO: PRINT OUT THE header car (given at top).
            3.5 TODO: PARSE through the header data and update the output7state, and update the GPIO7 output.
            *************************************/
            Serial.println("----- HTTP REQUEST HEADER START -----");
            Serial.print(header);
            Serial.println("----- HTTP REQUEST HEADER END   -----");

            if (header.indexOf("GET /7/on") >= 0) {
              output7State = "on";
              digitalWrite(7, HIGH);
            } else if (header.indexOf("GET /7/off") >= 0) {
              output7State = "off";
              digitalWrite(7, LOW);
            }

            //(THIS IS THE HTML BEING BROADCASTED TO STATION)
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS for on/off buttons 
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            /***********************************
            4. TODO: change "Default Header" to the name of the ssid
            *************************************/
            client.print("<body><h1>");
            client.print(ssid); 
            client.println("</h1>");        

            // Display current state, and ON/OFF buttons for GPIO 7  
            client.println("<p>GPIO 7 - State " + output7State + "</p>");
            // If the output7State is off, display the ON button       
            if (output7State=="off") {
              client.println("<p><a href=\"/7/on\"><button class=\"button\">ON</button></a></p>");
            } else { //output7state=on display the OFF button
              client.println("<p><a href=\"/7/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";

    // Close connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
