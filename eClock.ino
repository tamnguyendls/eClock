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
#define AUTO_BRIGHTNESS       ENABLE    // Enable automatic brightness adjustment

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
const uint8_t atStart[] = {0x40, 0x40, 0x40, 0x40};
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

/* Brightness Adaptation Thresholds (LDR ADC values: 0-1023) */
#define BRNS_VVHIGH 950   // Very very bright (< 50 ADC)
#define BRNS_VHIGH  800   // Very bright (50-200 ADC)
#define BRNS_HIGH   650   // Bright (200-350 ADC)
#define BRNS_NRML   500   // Normal (350-500 ADC)
#define BRNS_LOW    350   // Low light (500-650 ADC)
#define BRNS_VLOW   200   // Very low light (650-800 ADC)
#define BRNS_VVLOW  50    // Very very low light (800-973 ADC)
/* ADC readings are inverted: Higher ADC = Darker environment */

#define BRIGHTNESS_SMOOTHING 5  // Number of readings to average (reduces flickering)

unsigned int lastBrightness = 0;
unsigned int brightnessReadings[BRIGHTNESS_SMOOTHING] = {0};
unsigned int readingIndex = 0;

/**
 * ReadEnvBrightness: Reads analog light sensor (LDR) value from A0
 * Returns: ADC value (0-1023) with smoothing applied
 * Note: Higher ADC value = Darker environment
 */
unsigned int ReadEnvBrightness()
{
  // Read analog value from LDR (connected to A0)
  unsigned int adcValue = analogRead(A0);
  
  // Store reading in circular buffer for smoothing
  brightnessReadings[readingIndex] = adcValue;
  readingIndex = (readingIndex + 1) % BRIGHTNESS_SMOOTHING;
  
  // Calculate average of last BRIGHTNESS_SMOOTHING readings
  unsigned long sum = 0;
  for (int i = 0; i < BRIGHTNESS_SMOOTHING; i++) {
    sum += brightnessReadings[i];
  }
  
  return sum / BRIGHTNESS_SMOOTHING;
}

/**
 * GetBrightnessLevel: Maps ADC value to display brightness level (0-7)
 * Returns: Brightness level 0 (dimmest) to 7 (brightest)
 */
uint8_t GetBrightnessLevel(unsigned int adcValue)
{
  // ADC thresholds map to brightness levels
  // Higher ADC = Darker = Need Higher Brightness
  if (adcValue >= BRNS_VVHIGH) {
    return 7;  // Very dark environment - maximum brightness
  } else if (adcValue >= BRNS_VHIGH) {
    return 6;  // Very low light
  } else if (adcValue >= BRNS_HIGH) {
    return 5;  // Low light
  } else if (adcValue >= BRNS_NRML) {
    return 4;  // Normal lighting
  } else if (adcValue >= BRNS_LOW) {
    return 3;  // Moderate light
  } else if (adcValue >= BRNS_VLOW) {
    return 2;  // Bright light
  } else if (adcValue >= BRNS_VVLOW) {
    return 1;  // Very bright light
  } else {
    return 0;  // Extremely bright - minimum brightness
  }
}

/**
 * AdaptDisplayBrightness: Adjusts TM1637 display brightness based on environment
 * Automatically updates display brightness to match ambient light conditions
 * Includes hysteresis to prevent flickering (only updates if change > threshold)
 */
void AdaptDisplayBrightness()
{
  unsigned int currentADC = ReadEnvBrightness();
  uint8_t currentBrightness = GetBrightnessLevel(currentADC);
  
  // Only update display if brightness level changed
  // This prevents constant updates and flickering
  if (currentBrightness != lastBrightness) {
    display.setBrightness(currentBrightness, true);
    lastBrightness = currentBrightness;
    
    // Optional: Log brightness changes to serial
    Serial.print("Brightness adjusted: ADC=");
    Serial.print(currentADC);
    Serial.print(" Level=");
    Serial.println(currentBrightness);
  }
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

#if (LED_DISPLAY == ENABLE)
  // Display set up
  display.clear();
  delay(100);
  display.setBrightness(7);
  display.setSegments(atStart);
#endif

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
      display.setSegments(allON);
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("Connect successfully");
      display.setSegments(atStart);
  }
  
  // if you get here you have connected to the WiFi
  wifi_server.begin();

#if (NTP_SERVER == ENABLE)
  InitTimeClient();
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
        client.println("<!DOCTYPE html>");
        client.println("<html lang=\"en\">");
        client.println("<head>");
        client.println("<meta charset=\"UTF-8\">");
        client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
        client.println("<title>eClock WiFi Setup</title>");
        client.println("<link rel=\"icon\" href=\"data:,\">");
        client.println("<style>");
        client.println("* { margin: 0; padding: 0; box-sizing: border-box; }");
        client.println("body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; display: flex; justify-content: center; align-items: center; padding: 20px; }");
        client.println(".container { background: white; border-radius: 10px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); padding: 40px; max-width: 400px; width: 100%; }");
        client.println("h1 { color: #333; margin-bottom: 10px; font-size: 28px; text-align: center; }");
        client.println(".subtitle { color: #666; text-align: center; margin-bottom: 30px; font-size: 14px; }");
        client.println(".network-list { margin-bottom: 25px; }");
        client.println(".network-item { padding: 12px; margin-bottom: 8px; border: 2px solid #e0e0e0; border-radius: 8px; cursor: pointer; transition: all 0.3s ease; display: flex; justify-content: space-between; align-items: center; }");
        client.println(".network-item:hover { border-color: #667eea; background-color: #f5f7ff; }");
        client.println(".network-item.selected { background-color: #667eea; color: white; border-color: #667eea; }");
        client.println(".network-name { flex: 1; font-weight: 500; }");
        client.println(".network-signal { font-size: 12px; opacity: 0.7; }");
        client.println(".network-item.selected .network-signal { opacity: 1; }");
        client.println(".form-group { margin-bottom: 20px; }");
        client.println("label { display: block; margin-bottom: 8px; color: #333; font-weight: 500; font-size: 14px; }");
        client.println("input[type='text'], input[type='password'] { width: 100%; padding: 12px; border: 2px solid #e0e0e0; border-radius: 8px; font-size: 14px; transition: border-color 0.3s ease; }");
        client.println("input[type='text']:focus, input[type='password']:focus { outline: none; border-color: #667eea; }");
        client.println(".button { width: 100%; padding: 12px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; transition: transform 0.2s ease, box-shadow 0.2s ease; }");
        client.println(".button:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4); }");
        client.println(".button:disabled { opacity: 0.6; cursor: not-allowed; transform: none; }");
        client.println(".info-box { background-color: #f0f4ff; border-left: 4px solid #667eea; padding: 12px; margin-bottom: 20px; border-radius: 4px; font-size: 13px; color: #333; }");
        client.println(".spinner { display: none; text-align: center; margin: 20px 0; }");
        client.println(".spinner.show { display: block; }");
        client.println(".spinner span { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: #667eea; margin: 0 4px; animation: pulse 1.4s infinite; }");
        client.println(".spinner span:nth-child(2) { animation-delay: 0.2s; }");
        client.println(".spinner span:nth-child(3) { animation-delay: 0.4s; }");
        client.println("@keyframes pulse { 0%, 100% { opacity: 0.3; } 50% { opacity: 1; } }");
        client.println(".error { color: #d32f2f; font-size: 12px; margin-top: 5px; display: none; }");
        client.println(".success { color: #388e3c; font-size: 14px; text-align: center; margin-bottom: 20px; display: none; }");
        client.println("</style>");
        client.println("</head>");
        client.println("<body>");
        client.println("<div class=\"container\">");
        client.println("<h1>eClock</h1>");
        client.println("<p class=\"subtitle\">WiFi Network Setup</p>");
        
        client.println("<div class=\"info-box\">📡 Available Networks</div>");
        
        client.println("<form id=\"wifiForm\" onsubmit=\"handleSubmit(event)\">");
        client.println("<div class=\"network-list\" id=\"networkList\">");
        client.println("<div class=\"spinner show\" id=\"spinner\"><span></span><span></span><span></span></div>");
        client.println("</div>");
        
        client.println("<div class=\"form-group\">");
        client.println("<label for=\"selectedNetwork\">Selected Network:</label>");
        client.println("<input type=\"text\" id=\"selectedNetwork\" name=\"ssid\" readonly placeholder=\"Select a network above...\" style=\"background-color: #f5f5f5; cursor: not-allowed;\">");
        client.println("</div>");
        
        client.println("<div class=\"form-group\">");
        client.println("<label for=\"password\">Password:</label>");
        client.println("<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter WiFi password\" autocomplete=\"off\">");
        client.println("<div class=\"error\" id=\"passwordError\">Password is required</div>");
        client.println("</div>");
        
        client.println("<button type=\"submit\" class=\"button\" id=\"connectBtn\" disabled>Connect to Network</button>");
        client.println("<div class=\"success\" id=\"successMsg\">✓ Connected successfully!</div>");
        client.println("<div class=\"error\" id=\"generalError\"></div>");
        
        client.println("</form>");
        
        client.println("</div>");
        
        client.println("<script>");
        client.println("const networks = ");
        
        // Scan WiFi networks and output as JSON
        int n = WiFi.scanNetworks();
        client.print("[");
        for (int i = 0; i < n; ++i) {
          if (i > 0) client.print(",");
          client.print("{\"ssid\":\"");
          client.print(WiFi.SSID(i));
          client.print("\",\"rssi\":");
          client.print(WiFi.RSSI(i));
          client.print(",\"channel\":");
          client.print(WiFi.channel(i));
          client.print(",\"secure\":");
          client.print(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false");
          client.print("}");
        }
        client.println("];");
        
        client.println("function getSignalStrength(rssi) {");
        client.println("  if (rssi >= -50) return '📶 Excellent';");
        client.println("  else if (rssi >= -60) return '📶 Good';");
        client.println("  else if (rssi >= -70) return '📳 Fair';");
        client.println("  else return '📴 Weak';");
        client.println("}");
        
        client.println("function displayNetworks() {");
        client.println("  const list = document.getElementById('networkList');");
        client.println("  const spinner = document.getElementById('spinner');");
        client.println("  spinner.classList.remove('show');");
        client.println("  ");
        client.println("  if (networks.length === 0) {");
        client.println("    list.innerHTML = '<div style=\"color: #999; padding: 20px; text-align: center;\">No networks found. Please scan again.</div>';");
        client.println("    return;");
        client.println("  }");
        client.println("  ");
        client.println("  list.innerHTML = networks.map(net => `");
        client.println("    <div class=\"network-item\" onclick=\"selectNetwork('${net.ssid}', this)\">");
        client.println("      <span class=\"network-name\">${net.ssid}</span>");
        client.println("      <span class=\"network-signal\">${getSignalStrength(net.rssi)} ${net.secure ? '🔒' : '🔓'}</span>");
        client.println("    </div>");
        client.println("  `).join('');");
        client.println("}");
        
        client.println("function selectNetwork(ssid, element) {");
        client.println("  document.querySelectorAll('.network-item').forEach(el => el.classList.remove('selected'));");
        client.println("  element.classList.add('selected');");
        client.println("  document.getElementById('selectedNetwork').value = ssid;");
        client.println("  document.getElementById('connectBtn').disabled = false;");
        client.println("  document.getElementById('password').focus();");
        client.println("}");
        
        client.println("function handleSubmit(event) {");
        client.println("  event.preventDefault();");
        client.println("  const ssid = document.getElementById('selectedNetwork').value;");
        client.println("  const password = document.getElementById('password').value;");
        client.println("  ");
        client.println("  if (!ssid) {");
        client.println("    document.getElementById('generalError').textContent = 'Please select a network';");
        client.println("    document.getElementById('generalError').style.display = 'block';");
        client.println("    return;");
        client.println("  }");
        client.println("  ");
        client.println("  if (!password) {");
        client.println("    document.getElementById('passwordError').style.display = 'block';");
        client.println("    return;");
        client.println("  }");
        client.println("  ");
        client.println("  document.getElementById('connectBtn').disabled = true;");
        client.println("  document.getElementById('connectBtn').textContent = 'Connecting...';");
        client.println("  ");
        client.println("  fetch('/connect', {");
        client.println("    method: 'POST',");
        client.println("    headers: {'Content-Type': 'application/x-www-form-urlencoded'},");
        client.println("    body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`");
        client.println("  })");
        client.println("  .then(response => response.text())");
        client.println("  .then(data => {");
        client.println("    document.getElementById('wifiForm').style.display = 'none';");
        client.println("    document.getElementById('successMsg').style.display = 'block';");
        client.println("    setTimeout(() => {");
        client.println("      document.location.href = '/';");
        client.println("    }, 3000);");
        client.println("  })");
        client.println("  .catch(error => {");
        client.println("    document.getElementById('connectBtn').disabled = false;");
        client.println("    document.getElementById('connectBtn').textContent = 'Connect to Network';");
        client.println("    document.getElementById('generalError').textContent = 'Connection failed. Please try again.';");
        client.println("    document.getElementById('generalError').style.display = 'block';");
        client.println("  });");
        client.println("}");
        
        client.println("document.getElementById('password').addEventListener('input', function() {");
        client.println("  document.getElementById('passwordError').style.display = 'none';");
        client.println("  document.getElementById('generalError').style.display = 'none';");
        client.println("});");
        
        client.println("displayNetworks();");
        client.println("</script>");
        
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
      uint dot_on = 0b01000000;   // Colon ON
      uint dot_off = 0b00000000;  // Colon OFF
      
      UpdateNTPTime();
      DisplayTimeClient();
      
#if (AUTO_BRIGHTNESS == ENABLE)
      // Adjust display brightness based on environment
      AdaptDisplayBrightness();
#endif
      
      // Display time with colon blinking
      hrs = timeClient.getHours() * 100;
      time = hrs + timeClient.getMinutes();
      
      // Show time with colon ON for 1 second
      display.showNumberDecEx(time, dot_on, true, 4, 0);
      delay(1000);
      
      // Show time with colon OFF for 1 second
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


