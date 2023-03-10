#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

#include "NTPClient.h"
#include "WiFiUdp.h"

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output2State = "off";

// Assign output variables to GPIO pins
const int output2 = 2;

String currentLine = "";                // make a String to hold incoming data from the client

/* NTP Server Time */
/*
  You need to set offsettime for me it is 19800
  Because my timezone is utc+5:30 so
  UTC +5:30=5.5*60*60=19800
  UTC+1=1*60*60=3600
  CALCULATE your timezone and edit it and then upload the code.
*/
const long utcOffsetInSeconds = 19800;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
/* Define NTP Client to get time */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void InitTimeClient()
{
  timeClient.begin();

  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  // timeClient.setTimeOffset(0);
}

void UpdateTimeClient()
{
  timeClient.update();  
}

void DisplayTimeClient()
{
  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  //Serial.println(timeClient.getFormattedTime());

  delay(1000);
}
void setup() {
  Serial.println("Enter setup.\n");
  Serial.begin(115200);
  
  // Initialize the output variables as outputs
  pinMode(output2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output2, LOW);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  server.begin();

  InitTimeClient();
}

bool isClientConnected(WiFiClient client)
{
  return client.connected();
}


void checkForControlFromClient(WiFiClient client) // if there's bytes to read from the client,
{
  if (client.available()) 
  {             
    char c = client.read();             // read a byte, then
    Serial.write(c);                    // print it out the serial monitor
    header += c;
    if (c == '\n') {                    // if the byte is a newline character
      // if the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if (currentLine.length() == 0) {
        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
        // and a content-type so the client knows what's coming, then a blank line:
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();
        
        // turns the GPIOs on and off
        if (header.indexOf("GET /2/on") >= 0) {
          Serial.println("GPIO 2 on");
          output2State = "on";
          digitalWrite(output2, HIGH);
        } else if (header.indexOf("GET /2/off") >= 0) {
          Serial.println("GPIO 2 off");
          output2State = "off";
          digitalWrite(output2, LOW);
        }
        
        // Display the HTML web page
        client.println("<!DOCTYPE html><html>");
        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
        client.println("<link rel=\"icon\" href=\"data:,\">");
        // CSS to style the on/off buttons 
        // Feel free to change the background-color and font-size attributes to fit your preferences
        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
        client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
        client.println(".button2 {background-color: #77878A;}</style></head>");
        
        // Web Page Heading
        client.println("<body><h1>ESP8266 Web Server</h1>");
        
        // Display current state, and ON/OFF buttons for GPIO 5  
        client.println("<p>GPIO 2 - State " + output2State + "</p>");
        // If the output5State is off, it displays the ON button       
        if (output2State=="off") {
          client.println("<p><a href=\"/2/on\"><button class=\"button\">ON</button></a></p>");
        } else {
          client.println("<p><a href=\"/2/off\"><button class=\"button button2\">OFF</button></a></p>");
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

void loop(){

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) { 
    Serial.println("\nNew Client.");          // print a message out in the serial port

    while (isClientConnected(client)) {            // loop while the client's connected
      checkForControlFromClient(client);
      UpdateTimeClient();
      DisplayTimeClient();
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}


/* Ref link */
// [1] https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
// [2] https://www.instructables.com/Getting-Time-From-Internet-Using-ESP8266-NTP-Clock/
// [3] https://tttapa.github.io/ESP8266/Chap15%20-%20NTP.html

