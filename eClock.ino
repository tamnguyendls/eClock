#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TM1637Display.h>
#include <RTClib.h>              // https://github.com/adafruit/RTClib
#include <EEPROM.h>              // Built-in: Store time when NTP syncs
#include "wifi_setup.h"          // WiFi setup web interface HTML/CSS/JS
#include <DHT.h>                 // https://github.com/adafruit/DHT-sensor-library (DHT11/DHT22)

#define ENABLE  1
#define DISABLE 0

#define TEST_DISPLAY          DISABLE
#define LED_DISPLAY           ENABLE
#define WIFI_MANAGER          ENABLE
#define NTP_SERVER            ENABLE    
#define CLIENT_CTRL           DISABLE
#define AUTO_BRIGHTNESS       ENABLE    // Enable automatic brightness adjustment
#define RTC_BACKUP            DISABLE   // RTC backup (DS3231 chip, optional hardware)
#define EEPROM_FALLBACK       ENABLE    // EEPROM fallback (software-only, no hardware needed)
#define TIME_VALIDITY_CHECK   ENABLE    // Display '--:--' until time is valid (REQ_6)
#define DHT_SENSOR            ENABLE    // Enable DHT11/DHT22 humidity & temperature sensor (ERQ_7)

// DHT Sensor Configuration
#define DHT_PIN               D1        // GPIO5 (D1 on NodeMCU) - Data pin for DHT sensor
#define DHT_TYPE              DHT11     // DHT11 or DHT22 - Change based on your sensor
#define DHT_READ_INTERVAL     300000    // Read DHT every 5 minutes (REQ_12: every 5 mins)
#define TEMP_DISPLAY_DURATION 10000     // Display temperature for 10 seconds (user requested 5-10 sec)
#define TEMP_DISPLAY_INTERVAL 60000     // Show temperature every 60 seconds (1 minute)

// Set web server port number to 80
WiFiServer wifi_server(80);
#define WIFI_LED      2 // GPIO2
// Variable to store the HTTP request
String header;

// DHT Sensor Variables
#if (DHT_SENSOR == ENABLE)
DHT dht(DHT_PIN, DHT_TYPE);
float lastHumidity = 0.0;
float lastTemperature = 0.0;
unsigned long lastDHTReadTime = 0;
unsigned long lastTempDisplayTime = 0;
bool showingTemperature = false;
#endif

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
#define NTP_UPDATE_INTERVAL 60000  // Update NTP every 60 seconds (REQ_4)
const long utcOffsetInSeconds = UTC_PLUS_7;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
/* Define NTP Client to get time */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

/* Time tracking variables */
unsigned long lastNTPUpdate = 0;        // Timestamp of last NTP update
bool timeIsValid = false;               // Flag: time has been synced at least once
unsigned long lastRTCSync = 0;          // Timestamp of last RTC sync to NTP

#endif

/* EEPROM Storage for Time Fallback (Software-only, no hardware needed) ================= */
#if (EEPROM_FALLBACK == ENABLE)
#define EEPROM_SIZE 512                 // Total EEPROM size
#define EEPROM_TIME_ADDR 0              // Address for storing epoch time (4 bytes)
#define EEPROM_MILLIS_ADDR 4            // Address for storing millis at save time (4 bytes)

/**
 * SaveTimeToEEPROM: Save current NTP time to EEPROM
 * Stores: Current epoch time + millis() reference
 * Called: After each NTP sync
 */
void SaveTimeToEEPROM()
{
  time_t epochTime = timeClient.getEpochTime();
  unsigned long currentMillis = millis();
  
  // Write epoch time (4 bytes)
  EEPROM.write(EEPROM_TIME_ADDR, (epochTime >> 24) & 0xFF);
  EEPROM.write(EEPROM_TIME_ADDR + 1, (epochTime >> 16) & 0xFF);
  EEPROM.write(EEPROM_TIME_ADDR + 2, (epochTime >> 8) & 0xFF);
  EEPROM.write(EEPROM_TIME_ADDR + 3, epochTime & 0xFF);
  
  // Write millis reference (4 bytes)
  EEPROM.write(EEPROM_MILLIS_ADDR, (currentMillis >> 24) & 0xFF);
  EEPROM.write(EEPROM_MILLIS_ADDR + 1, (currentMillis >> 16) & 0xFF);
  EEPROM.write(EEPROM_MILLIS_ADDR + 2, (currentMillis >> 8) & 0xFF);
  EEPROM.write(EEPROM_MILLIS_ADDR + 3, currentMillis & 0xFF);
  
  EEPROM.commit();  // Write to flash
  Serial.println("[EEPROM] Time saved for WiFi loss fallback");
}

/**
 * GetEEPROMTime: Retrieve saved time and calculate elapsed time
 * Returns: Estimated current time based on saved time + elapsed millis
 */
time_t GetEEPROMTime()
{
  // Read epoch time (4 bytes)
  uint32_t savedEpoch = 0;
  savedEpoch |= ((uint32_t)EEPROM.read(EEPROM_TIME_ADDR)) << 24;
  savedEpoch |= ((uint32_t)EEPROM.read(EEPROM_TIME_ADDR + 1)) << 16;
  savedEpoch |= ((uint32_t)EEPROM.read(EEPROM_TIME_ADDR + 2)) << 8;
  savedEpoch |= ((uint32_t)EEPROM.read(EEPROM_TIME_ADDR + 3));
  
  // Read saved millis reference (4 bytes)
  uint32_t savedMillis = 0;
  savedMillis |= ((uint32_t)EEPROM.read(EEPROM_MILLIS_ADDR)) << 24;
  savedMillis |= ((uint32_t)EEPROM.read(EEPROM_MILLIS_ADDR + 1)) << 16;
  savedMillis |= ((uint32_t)EEPROM.read(EEPROM_MILLIS_ADDR + 2)) << 8;
  savedMillis |= ((uint32_t)EEPROM.read(EEPROM_MILLIS_ADDR + 3));
  
  // Calculate elapsed time since save
  unsigned long elapsed = (millis() - savedMillis) / 1000;  // Convert to seconds
  
  // Return estimated current time
  return savedEpoch + elapsed;
}

#endif

/* RTC Module - DS3231 (I2C: SDA=D4, SCL=D3) ============================================= */
#if (RTC_BACKUP == ENABLE)
RTC_DS3231 rtc;
bool rtcAvailable = false;  // Flag: RTC module found on I2C bus
#endif
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

/* DHT Sensor - Humidity & Temperature (ERQ_7, REQ_12) ================================== */
#if (DHT_SENSOR == ENABLE)
/**
 * InitDHTSensor: Initialize DHT sensor
 * Must be called in setup()
 */
void InitDHTSensor()
{
  dht.begin();
  Serial.println("[DHT] Humidity & Temperature Sensor initialized");
}

/**
 * ReadDHTSensor: Read humidity and temperature from DHT sensor
 * Reads every DHT_READ_INTERVAL (5 minutes per REQ_12)
 * Returns: true if read successful, false if failed
 */
bool ReadDHTSensor()
{
  unsigned long currentTime = millis();
  
  // Only read if interval has passed
  if (currentTime - lastDHTReadTime < DHT_READ_INTERVAL) {
    return true;  // Return cached values
  }
  
  // Read humidity
  float humidity = dht.readHumidity();
  
  // Read temperature as Celsius
  float temperature = dht.readTemperature();
  
  // Check if any reads failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("[DHT] Failed to read from DHT sensor!");
    return false;
  }
  
  // Store values
  lastHumidity = humidity;
  lastTemperature = temperature;
  lastDHTReadTime = currentTime;
  
  // Log readings
  Serial.print("[DHT] Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  return true;
}

/**
 * DisplayTemperature: Show temperature on 7-segment display
 * Format: "TTCC" where TT = temperature in Celsius (2 digits), CC = °C symbol
 * Example: 25°C displays as "25" with degree symbol
 */
void DisplayTemperature()
{
  if (!ReadDHTSensor()) {
    // If sensor read failed, show "-----"
    Serial.println("[DHT] Cannot display temperature - sensor read failed");
    display.setSegments(atStart);
    return;
  }
  
  // Display temperature as 2-digit number (e.g., 25 for 25°C)
  int tempInt = (int)lastTemperature;
  
  // Ensure temperature is within displayable range (0-99)
  if (tempInt < 0) tempInt = 0;
  if (tempInt > 99) tempInt = 99;
  
  // Display temperature: Show as "TT " with colon separator (dot positions as indicator)
  display.showNumberDec(tempInt, false, 2, 0);  // Show in first 2 digits
  
  // Optional: Show degree symbol in 3rd position
  // This is a bit tricky with TM1637 - we can use custom segments
  Serial.print("[DHT] Displaying: ");
  Serial.print(tempInt);
  Serial.println("°C");
}

/**
 * DisplayHumidity: Show humidity on 7-segment display
 * Format: "HH%" where HH = humidity percentage (2 digits)
 * Example: 65% displays as "65"
 */
void DisplayHumidity()
{
  if (!ReadDHTSensor()) {
    // If sensor read failed, show dashes
    Serial.println("[DHT] Cannot display humidity - sensor read failed");
    display.setSegments(atStart);
    return;
  }
  
  // Display humidity as 2-digit number (e.g., 65 for 65%)
  int humidInt = (int)lastHumidity;
  
  // Ensure humidity is within displayable range (0-99)
  if (humidInt < 0) humidInt = 0;
  if (humidInt > 99) humidInt = 99;
  
  // Display humidity: Show as "HH"
  display.showNumberDec(humidInt, false, 2, 0);  // Show in first 2 digits
  
  Serial.print("[DHT] Displaying: ");
  Serial.print(humidInt);
  Serial.println("%");
}

/**
 * HandleTemperatureDisplay: Manage temperature display rotation
 * - Shows time normally
 * - Switches to temperature every 60 seconds (TEMP_DISPLAY_INTERVAL)
 * - Displays temperature for 10 seconds (TEMP_DISPLAY_DURATION)
 * Call this in main loop
 */
void HandleTemperatureDisplay()
{
  unsigned long currentTime = millis();
  
  // Check if it's time to show temperature
  if (!showingTemperature && (currentTime - lastTempDisplayTime >= TEMP_DISPLAY_INTERVAL)) {
    showingTemperature = true;
    lastTempDisplayTime = currentTime;
    Serial.println("[DHT] Switching to temperature display");
    DisplayTemperature();
    return;
  }
  
  // Check if it's time to return to time display
  if (showingTemperature && (currentTime - lastTempDisplayTime >= TEMP_DISPLAY_DURATION)) {
    showingTemperature = false;
    Serial.println("[DHT] Switching back to time display");
    return;
  }
}

/**
 * DisplayDataSequence: Cycle through time, temperature, and humidity displays
 * Sequence: Time (normal) -> Temp (10s) -> Humidity (10s) -> repeat
 * Call this in main loop to manage full rotation
 */
#define DISPLAY_STATE_TIME       0
#define DISPLAY_STATE_TEMP       1
#define DISPLAY_STATE_HUMIDITY   2

uint8_t currentDisplayState = DISPLAY_STATE_TIME;
unsigned long displayStateChangeTime = 0;

void HandleDataDisplaySequence()
{
  unsigned long currentTime = millis();
  
  // After TEMP_DISPLAY_INTERVAL (60s), switch to temperature
  if (currentDisplayState == DISPLAY_STATE_TIME && 
      (currentTime - displayStateChangeTime >= TEMP_DISPLAY_INTERVAL)) {
    currentDisplayState = DISPLAY_STATE_TEMP;
    displayStateChangeTime = currentTime;
    Serial.println("[DHT] State: Showing Temperature");
    DisplayTemperature();
    return;
  }
  
  // After displaying temp for 10s, switch to humidity
  if (currentDisplayState == DISPLAY_STATE_TEMP && 
      (currentTime - displayStateChangeTime >= TEMP_DISPLAY_DURATION)) {
    currentDisplayState = DISPLAY_STATE_HUMIDITY;
    displayStateChangeTime = currentTime;
    Serial.println("[DHT] State: Showing Humidity");
    DisplayHumidity();
    return;
  }
  
  // After displaying humidity for 10s, back to time
  if (currentDisplayState == DISPLAY_STATE_HUMIDITY && 
      (currentTime - displayStateChangeTime >= TEMP_DISPLAY_DURATION)) {
    currentDisplayState = DISPLAY_STATE_TIME;
    displayStateChangeTime = currentTime;
    Serial.println("[DHT] State: Back to Time");
    return;
  }
}

#endif

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

/**
 * UpdateNTPWithInterval: Updates NTP time only if interval has elapsed
 * REQ_4: Updates every 60 seconds instead of every main loop iteration
 * Returns: true if update was performed, false if skipped due to timing
 */
bool UpdateNTPWithInterval()
{
  unsigned long now = millis();
  
  // Only update if NTP_UPDATE_INTERVAL milliseconds have passed
  if (now - lastNTPUpdate >= NTP_UPDATE_INTERVAL) {
    timeClient.update();
    lastNTPUpdate = now;
    timeIsValid = true;  // Mark time as valid once NTP succeeds
    
    Serial.print("[NTP] Time synced at ");
    Serial.println(timeClient.getFormattedTime());
    
    // Save time to EEPROM for WiFi loss fallback
#if (EEPROM_FALLBACK == ENABLE)
    SaveTimeToEEPROM();
#endif
    
    // Sync RTC if available (REQ_11: Every 1st of month)
#if (RTC_BACKUP == ENABLE)
    if (rtcAvailable) {
      SyncRTCFromNTP();
    }
#endif
    
    return true;
  }
  
  return false;
}

#endif

/* RTC Time Functions ======================================================================== */
#if (RTC_BACKUP == ENABLE)

/**
 * InitRTC: Initialize DS3231 RTC module
 * Checks if RTC is available on I2C bus
 * REQ_5: RTC fallback when WiFi lost
 */
void InitRTC()
{
  if (!rtc.begin()) {
    Serial.println("[RTC] DS3231 RTC not found on I2C bus");
    rtcAvailable = false;
  } else {
    Serial.println("[RTC] DS3231 RTC initialized successfully");
    rtcAvailable = true;
    
    // Set RTC time from NTP if RTC time is not set
    if (!rtc.isrunning()) {
      Serial.println("[RTC] RTC not running, setting time from NTP...");
      // Will be synced after NTP updates in first loop
    }
  }
}

/**
 * GetRTCTime: Read current time from DS3231 RTC
 * Returns: DateTime object with RTC time
 */
DateTime GetRTCTime()
{
  if (rtcAvailable) {
    return rtc.now();
  }
  return DateTime(0);  // Return epoch if RTC unavailable
}

/**
 * SyncRTCFromNTP: Synchronize RTC time with NTP time
 * REQ_11: Update RTC time from NTP every 1st day of month
 */
void SyncRTCFromNTP()
{
  time_t ntpTime = timeClient.getEpochTime();
  
  // Only sync on 1st of month or if RTC is not running
  if (rtc.isrunning() == false || timeClient.getDate() == 1) {
    rtc.adjust(DateTime(ntpTime));
    Serial.println("[RTC] Time synchronized from NTP");
    lastRTCSync = millis();
  }
}

/**
 * ApplyTimeSource: Get time from NTP or fallback sources based on WiFi status
 * Returns: true if time is available, false if no valid time source
 * Priority: NTP (if WiFi) > RTC (if available) > EEPROM (if available)
 * REQ_5: RTC fallback when WiFi lost
 * EEPROM: Software-only fallback (no hardware needed)
 */
bool ApplyTimeSource()
{
  if (WiFi.status() == WL_CONNECTED) {
    // WiFi available: use NTP time (preferred)
    return timeIsValid;
  } else {
    // WiFi lost: try hardware and software fallbacks
    
    // First priority: RTC module (most accurate)
#if (RTC_BACKUP == ENABLE)
    if (rtcAvailable) {
      DateTime rtcTime = GetRTCTime();
      if (rtcTime.isValid()) {
        Serial.print("[RTC] Using RTC fallback: ");
        Serial.print(rtcTime.year());
        Serial.print("-");
        Serial.print(rtcTime.month());
        Serial.print("-");
        Serial.print(rtcTime.day());
        Serial.print(" ");
        Serial.print(rtcTime.hour());
        Serial.print(":");
        Serial.print(rtcTime.minute());
        Serial.println();
        return true;
      }
    }
#endif
    
    // Second priority: EEPROM storage (software-only)
#if (EEPROM_FALLBACK == ENABLE)
    time_t eepromTime = GetEEPROMTime();
    if (eepromTime > 0) {
      Serial.print("[EEPROM] Using fallback time (elapsed since sync): ");
      Serial.println(eepromTime);
      return true;
    }
#endif
    
    // No valid time source available
    return false;
  }
}

#endif 

void setup() {
  bool res;

#if (LED_DISPLAY == ENABLE)
  // Display set up
  display.clear();
  delay(100);
  display.setBrightness(7);
  
  // REQ_6: Show "--:--" while waiting for time
#if (TIME_VALIDITY_CHECK == ENABLE)
  display.setSegments((uint8_t[]){0x40, 0x40, 0x40, 0x40});  // Display dashes
  Serial.println("[DISPLAY] Showing '--:--' (waiting for time)");
#else
  display.setSegments(atStart);
#endif

#endif

  Serial.println("Enter setup.\n");
  Serial.begin(115200);
  
  // Initialize EEPROM for time fallback storage
#if (EEPROM_FALLBACK == ENABLE)
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("[EEPROM] Initialized for time fallback storage");
#endif
  
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
      
#if (TIME_VALIDITY_CHECK == ENABLE)
      display.setSegments((uint8_t[]){0x40, 0x40, 0x40, 0x40});  // Still show '--:--' until NTP syncs
#else
      display.setSegments(atStart);
#endif
  }
  
  // if you get here you have connected to the WiFi
  wifi_server.begin();

#if (NTP_SERVER == ENABLE)
  InitTimeClient();
  // Force immediate NTP update on startup
  UpdateNTPWithInterval();
#endif

#if (RTC_BACKUP == ENABLE)
  // Initialize RTC module (I2C: SDA=D4, SCL=D3)
  InitRTC();
#endif

#if (DHT_SENSOR == ENABLE)
  // Initialize DHT sensor (Humidity & Temperature - ERQ_7)
  InitDHTSensor();
#endif

}

bool isClientConnected(WiFiClient client)
{
  return client.connected();
}

/**
 * ServeDataEndpoint: Serve JSON API with current time, temperature, humidity
 * Returns JSON: {"time": "HH:MM:SS", "temperature": 24.5, "humidity": 65.0, "timestamp": 1234567890}
 */
void ServeDataEndpoint(WiFiClient client)
{
  // Send HTTP headers
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: application/json");
  client.println("Connection: close");
  client.println();
  
  // Build JSON response
  String json = "{";
  json += "\"time\":\"";
  
  // Format time HH:MM:SS
  if (timeClient.getHours() < 10) json += "0";
  json += timeClient.getHours();
  json += ":";
  if (timeClient.getMinutes() < 10) json += "0";
  json += timeClient.getMinutes();
  json += ":";
  if (timeClient.getSeconds() < 10) json += "0";
  json += timeClient.getSeconds();
  json += "\",";
  
#if (DHT_SENSOR == ENABLE)
  json += "\"temperature\":" + String(lastTemperature, 1) + ",";
  json += "\"humidity\":" + String(lastHumidity, 1) + ",";
#else
  json += "\"temperature\":null,";
  json += "\"humidity\":null,";
#endif
  
  json += "\"timestamp\":" + String(timeClient.getEpochTime());
  json += "}";
  
  client.print(json);
  
  Serial.println("[API] Served data endpoint");
}

/**
 * ServeDashboardPage: Serve HTML dashboard displaying live time, temperature, humidity
 */
void ServeDashboardPage(WiFiClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: text/html");
  client.println("Connection: close");
  client.println();
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>eClock Dashboard</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 15px;
      box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
      padding: 40px;
      max-width: 500px;
      width: 100%;
    }
    h1 {
      color: #333;
      text-align: center;
      margin-bottom: 10px;
      font-size: 32px;
    }
    .subtitle {
      color: #999;
      text-align: center;
      margin-bottom: 40px;
      font-size: 14px;
    }
    .data-box {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border-radius: 10px;
      padding: 20px;
      margin-bottom: 20px;
      text-align: center;
    }
    .data-label {
      font-size: 12px;
      opacity: 0.9;
      margin-bottom: 8px;
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    .data-value {
      font-size: 36px;
      font-weight: bold;
      font-family: 'Courier New', monospace;
    }
    .data-unit {
      font-size: 14px;
      opacity: 0.8;
      margin-left: 8px;
    }
    .info-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
      margin-bottom: 20px;
    }
    .info-item {
      background: #f5f7ff;
      border-left: 4px solid #667eea;
      padding: 15px;
      border-radius: 5px;
    }
    .info-label {
      font-size: 12px;
      color: #666;
      margin-bottom: 5px;
      text-transform: uppercase;
    }
    .info-value {
      font-size: 24px;
      color: #333;
      font-weight: bold;
    }
    .refresh-status {
      text-align: center;
      font-size: 12px;
      color: #999;
      margin-top: 20px;
    }
    .status-online {
      display: inline-block;
      width: 8px;
      height: 8px;
      background: #4caf50;
      border-radius: 50%;
      margin-right: 5px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>eClock</h1>
    <p class="subtitle">Live Data Dashboard</p>
    
    <div class="data-box">
      <div class="data-label">⏰ Current Time</div>
      <div class="data-value" id="timeDisplay">--:--:--</div>
    </div>
    
    <div class="info-grid">
      <div class="info-item">
        <div class="info-label">🌡️ Temperature</div>
        <div class="info-value" id="tempDisplay">--</div>
        <span style="font-size: 12px;">°C</span>
      </div>
      <div class="info-item">
        <div class="info-label">💧 Humidity</div>
        <div class="info-value" id="humidDisplay">--</div>
        <span style="font-size: 12px;">%</span>
      </div>
    </div>
    
    <div class="refresh-status">
      <span class="status-online"></span>
      <span id="statusText">Connecting...</span>
    </div>
  </div>
  
  <script>
    const API_ENDPOINT = '/api/data';
    const REFRESH_INTERVAL = 2000; // Update every 2 seconds
    
    function fetchData() {
      fetch(API_ENDPOINT)
        .then(response => response.json())
        .then(data => {
          document.getElementById('timeDisplay').textContent = data.time;
          
          if (data.temperature !== null) {
            document.getElementById('tempDisplay').textContent = data.temperature.toFixed(1);
          } else {
            document.getElementById('tempDisplay').textContent = '--';
          }
          
          if (data.humidity !== null) {
            document.getElementById('humidDisplay').textContent = data.humidity.toFixed(1);
          } else {
            document.getElementById('humidDisplay').textContent = '--';
          }
          
          document.getElementById('statusText').textContent = 'Connected • Last update: ' + new Date().toLocaleTimeString();
        })
        .catch(error => {
          console.error('Error fetching data:', error);
          document.getElementById('statusText').textContent = 'Connection error';
        });
    }
    
    // Initial fetch and set up auto-refresh
    fetchData();
    setInterval(fetchData, REFRESH_INTERVAL);
  </script>
</body>
</html>
)rawliteral";
  
  client.print(html);
  
  Serial.println("[WEB] Served dashboard page");
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
        // Check for different routes and serve appropriate content
        
        // Route 1: API endpoint /api/data (JSON with time, temp, humidity)
        if (header.indexOf("GET /api/data") >= 0) {
          ServeDataEndpoint(client);
          client.println();
          return;
        }
        
        // Route 2: Dashboard /dashboard (HTML with live data display)
        if (header.indexOf("GET /dashboard") >= 0) {
          ServeDashboardPage(client);
          client.println();
          return;
        }
        
        // Route 3: GPIO control (legacy)
        if (header.indexOf("GET /2/on") >= 0) {
          Serial.println("GPIO 2 on");
          output2State = "on";
          digitalWrite(output2, HIGH);
        } else if (header.indexOf("GET /2/off") >= 0) {
          Serial.println("GPIO 2 off");
          output2State = "off";
          digitalWrite(output2, LOW);
        }
        
        // Route 4: Default page - WiFi Setup or Dashboard
        // If root path, show dashboard if DHT available, else WiFi setup
#if (DHT_SENSOR == ENABLE)
        if (header.indexOf("GET /") >= 0) {
          ServeDashboardPage(client);
          client.println();
          return;
        }
#endif
        
        // Default: Display the HTML web page from wifi_setup.h header file
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();
        
        // Get HTML template from PROGMEM (stored in wifi_setup.h)
        String htmlPage = FPSTR(WIFI_SETUP_HTML);
        
        // Build network list as JSON for dynamic injection
        String networksJson = "[";
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i) {
          if (i > 0) networksJson += ",";
          networksJson += "{\"ssid\":\"" + WiFi.SSID(i) + "\"";
          networksJson += ",\"rssi\":" + String(WiFi.RSSI(i));
          networksJson += ",\"channel\":" + String(WiFi.channel(i));
          networksJson += ",\"secure\":" + (WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false");
          networksJson += "}";
        }
        networksJson += "]";
        
        // Replace placeholder with actual network data
        htmlPage.replace("%NETWORKS_JSON%", networksJson);
        
        // Send complete HTML page
        client.print(htmlPage);
        
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
      
      // REQ_4: Update NTP only every 60 seconds
#if (NTP_SERVER == ENABLE)
      UpdateNTPWithInterval();
      
      // REQ_6: Check if time is valid before displaying
#if (TIME_VALIDITY_CHECK == ENABLE)
      if (!timeIsValid) {
        // Display "--:--" until time is synced
        display.setSegments((uint8_t[]){0x40, 0x40, 0x40, 0x40});
        delay(1000);
        display.setSegments((uint8_t[]){0x40, 0x40, 0x40, 0x40});
        delay(1000);
        return;  // Skip display update until time is valid
      }
#endif
      
      // Get time from appropriate source (NTP or RTC fallback)
#if (RTC_BACKUP == ENABLE)
      if (!ApplyTimeSource()) {
        // No valid time source
        Serial.println("[TIME] No valid time source available");
        display.setSegments((uint8_t[]){0x40, 0x40, 0x40, 0x40});
        delay(1000);
        return;
      }
#endif
      
      DisplayTimeClient();
#endif
      
#if (AUTO_BRIGHTNESS == ENABLE)
      // REQ_14: Adjust display brightness based on environment (< 5 seconds response)
      AdaptDisplayBrightness();
#endif

#if (DHT_SENSOR == ENABLE)
      // ERQ_7, REQ_12: Handle data display rotation (Time -> Temp -> Humidity)
      // Reads temperature/humidity every 5 minutes, cycles displays every 60 seconds
      HandleDataDisplaySequence();
      
      // If not showing temperature or humidity, display time normally
      if (currentDisplayState == DISPLAY_STATE_TIME) {
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

#if (DHT_SENSOR == ENABLE)
      }  // End of currentDisplayState == DISPLAY_STATE_TIME check
#endif
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


