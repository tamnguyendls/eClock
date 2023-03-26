#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TM1637Display.h>

#define ENABLE  1
#define DISABLE 0

#define TEST_DISPLAY          DISABLE
#define LED_DISPLAY           ENABLE
#define WIFI_MANAGER          ENABLE
#define NTP_SERVER            ENABLE    
#define CLIENT_CTRL           DISABLE

// Set web server port number to 80
WiFiServer wifi_server(80);
#define WIFI_LED      2 // GPIO2
// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output2State = "off";

// Assign output variables to GPIO pins
const int output2 = 2;

String currentLine = "";                // make a String to hold incoming data from the client

/* NTP Server Time ============================================================================ */
/*
  You need to set offsettime for me it is 19800
  Because my timezone is utc+5:30 so
  UTC +5:30=5.5*60*60=19800
  UTC+1=1*60*60=3600

  UTC+7 = 7*60*60=25200
  CALCULATE your timezone and edit it and then upload the code.
*/
#if (NTP_SERVER == ENABLE)
#define UTC_PLUS_7  25200
const long utcOffsetInSeconds = UTC_PLUS_7;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
/* Define NTP Client to get time */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#endif
/* 7-SEG LED 4-DIGITS - TM1637 ================================================================ */
// Define the connections pins
const int CLK = D6; //Set the CLK pin connection to the display
const int DIO = D5; //Set the DIO pin connection to the display

// Create a display object of type TM1637Display
TM1637Display display(CLK, DIO);

// Create an array that turns all segments ON
const uint8_t allON[] = {0xff, 0xff, 0xff, 0xff};

// Create an array that turns all segments OFF
const uint8_t allOFF[] = {0x00, 0x00, 0x00, 0x00};

// Create an array that sets individual segments per digit to display the word "dOnE"
const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

// Create degree celsius symbol
const uint8_t celsius[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Degree symbol
  SEG_A | SEG_D | SEG_E | SEG_F   // C
};

#define BRNS_VVHIGH 1000 // 1023 (10-bit ADC)
#define BRNS_VHIGH  854
#define BRNS_HIGH    708
#define BRNS_NRML    562
#define BRNS_LOW     416
#define BRNS_VLOW 270
#define BRNS_VVLOW 124
#define BRNS_VV_HIGH 1000

unsigned int ReadEnvBrightness()
{
  // return ADC value
  return 0;
}

void AdaptDisplayBrightness()
{
  // env_brightness = ReadEnvBrightness();
  /*
  if (env_brightness >= BRNS_VVHIGH) {
    // 1
  } else if (env_brightness < BRNS_VVHIGH) && (env_brightness >= BRNS_VHIGH) {
    // 2
  } else if (env_brightness < BRNS_VHIGH) && (env_brightness >= BRNS_HIGH) {
    // 3
  } else if (env_brightness < BRNS_HIGH) && (env_brightness >= BRNS_NRML) {
    // 4
  } else if (env_brightness < BRNS_NRML) && (env_brightness >= BRNS_LOW) {
    // 5
  } else if (env_brightness < BRNS_LOW) && (env_brightness >= BRNS_VVLOW) {
    // 6
  } else {
    // 7
  }
  */
}

#if (NTP_SERVER == ENABLE)
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

void UpdateNTPTime()
{
  timeClient.update();  
}

void DisplayTimeClient()
{
  // Serial.println("DisplayTimeClient");
  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  //Serial.println(timeClient.getFormattedTime());

  // delay(1000);
}
#endif 

void setup() {
  bool res;

  Serial.println("Enter setup.\n");
  Serial.begin(115200);
  
  // Initialize the output variables as outputs
  pinMode(output2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output2, LOW);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wm.resetSettings();

  res = wm.autoConnect("eClock_AP");
  if(!res) {
      Serial.println("Connect failure");
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("Connect successfully");
  }
  
  // if you get here you have connected to the WiFi
  wifi_server.begin();

#if (NTP_SERVER == ENABLE)
  InitTimeClient();
#endif

#if (LED_DISPLAY == ENABLE)
  // Display set up
  display.clear();
  delay(100);
  display.setBrightness(7);
#endif
}

bool isClientConnected(WiFiClient client)
{
  return client.connected();
}

#if (CLIENT_CTRL == ENABLE)
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
        // Break out of this func
        return;
      } else { // if you got a newline, then clear currentLine
        currentLine = "";
      }
    } else if (c != '\r') {  // if you got anything else but a carriage return character,
      currentLine += c;      // add it to the end of the currentLine
    }
  }
}
#endif

void loop(){

  // WiFiClient client = wifi_server.available();   // Listen for incoming clients

  // if (client) { 
    // Serial.println("\nNew Client.");          // print a message out in the serial port

    // if (isClientConnected(client)) {            // loop while the client's connected
//     digitalWrite(output2, HIGH);
//     Serial.println("Connected.");
// #if (CLIENT_CTRL == ENABLE)
//       checkForControlFromClient(client);
// #endif
//       UpdateNTPTime();
//       DisplayTimeClient();
//     } else {
//       digitalWrite(output2, LOW);
//     }
//     // Clear the header variable
//     header = "";
//     // Close the connection
//     client.stop();
//     Serial.println("Client disconnected.");
//     Serial.println("");
//   }
      uint hrs, time;
      uint dot_on = 0b01000000;
      uint dot_off = 0b00000000;
      UpdateNTPTime();
      DisplayTimeClient();
      // display.showNumberDec(timeClient.getHours(),false,2,0);
      // display.showNumberDec(timeClient.getMinutes(),false,2,2);
      hrs = timeClient.getHours() * 100;
      time = hrs + timeClient.getMinutes();
      display.showNumberDecEx(time, dot_on, true, 4, 0);
      delay(1000);
      display.showNumberDecEx(time, dot_off, true, 4, 0);
      delay(1000);
}

#if (TEST_DISPLAY == ENABLE)
void test_display() {
  int numCounter = 0;
  for(numCounter = 0; numCounter < 40; numCounter++) //Iterate numCounter
  {
    if ((numCounter >= 0)&& (numCounter < 5)){
      display.setBrightness(0);
    } else if ((numCounter >= 5)&& (numCounter < 10)) {
      display.setBrightness(1);
    } else if ((numCounter >= 10)&& (numCounter < 15)) {
      display.setBrightness(2);
    } else if ((numCounter >= 15)&& (numCounter < 20)) {
      display.setBrightness(3);
    } else if ((numCounter >= 20)&& (numCounter < 25)) {
      display.setBrightness(4);
    } else if ((numCounter >= 25)&& (numCounter < 30)) {
      display.setBrightness(5);
    } else if ((numCounter >= 30)&& (numCounter < 35)) {
      display.setBrightness(6);
    } else {
      display.setBrightness(7);
    }
    display.showNumberDec(numCounter); //Display the numCounter value;
    delay(1000);
  }
}
#endif

/* Ref link */
// [1] https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
// [2] https://www.instructables.com/Getting-Time-From-Internet-Using-ESP8266-NTP-Clock/
// [3] https://tttapa.github.io/ESP8266/Chap15%20-%20NTP.html
// [4] https://www.instructables.com/How-to-Use-the-TM1637-Digit-Display-With-Arduino/
// [5] https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
// [6] https://github.com/himikat123/Clock
// [7] http://melissamerritt.epizy.com/internet-clock/internet_clock.html?i=1


