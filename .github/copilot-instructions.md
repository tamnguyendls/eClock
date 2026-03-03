# eClock AI Coding Agent Guide

## Project Overview
eClock is an ESP8266-based Internet Clock that displays time on a 4-digit 7-segment LED display (TM1637). The device fetches time from an NTP server via WiFi and displays HH:MM format with dynamic brightness adjustment based on environment light.

## Architecture & Key Components

### Hardware Setup
- **MCU**: ESP8266/NodeMCU
- **Display**: TM1637Display (4-digit 7-segment LED)
  - CLK pin: D6
  - DIO pin: D5
  - Brightness levels: 0-7 (set via `display.setBrightness()`)
- **WiFi LED**: GPIO 2
- **Light Sensor (LDR)**: For adaptive brightness (feature in requirements, not yet implemented)

### Software Architecture
- **WiFi Management**: WiFiManager library handles dynamic WiFi configuration via captive portal ("eClock_AP" SSID)
- **Time Synchronization**: NTPClient fetches from "pool.ntp.org" 
- **Timezone Handling**: UTC offset in seconds (currently UTC+7 = 25200)
- **Display Rendering**: Updates HH:MM with colon blink (on/off every 1000ms)

## Critical Development Patterns

### Configuration via Preprocessor Directives
The code uses feature flags to enable/disable functionality (top of file):
```cpp
#define TEST_DISPLAY          DISABLE     // Test all brightness levels
#define LED_DISPLAY           ENABLE      // 7-segment display
#define WIFI_MANAGER          ENABLE      // WiFi setup portal
#define NTP_SERVER            ENABLE      // NTP time sync
#define CLIENT_CTRL           DISABLE     // Web server control (disabled, contains HTTP server code)
```
**Pattern**: Wrap feature code in `#if (FEATURE == ENABLE)` blocks. This allows safe inclusion of incomplete/experimental features.

### Time Display Logic
Located in the main loop (around lines 326-335):
```cpp
hrs = timeClient.getHours() * 100;
time = hrs + timeClient.getMinutes();
display.showNumberDecEx(time, dot_on, true, 4, 0);  // Colon ON
delay(1000);
display.showNumberDecEx(time, dot_off, true, 4, 0); // Colon OFF
```
The colon blink is a simple 1-second toggle. Updates happen in the main loop.

### NTP Time Update Workflow
1. `InitTimeClient()` - Starts NTP client
2. `UpdateNTPTime()` - Calls `timeClient.update()` to sync
3. `DisplayTimeClient()` - Prints to serial (for debugging)

## Project Requirements & Current Status

**Implemented**:
- REQ_1: Dynamic WiFi setup via WiFiManager ✓
- REQ_2: NTP server time fetch ✓
- REQ_3: HH:MM display on TM1637 ✓
- REQ_4: Time updates in main loop (currently 1-second intervals, not 60s)

**Not Yet Implemented** (flagged in Requirement.txt):
- REQ_5: RTC fallback on WiFi loss
- REQ_6: "--:--" display when no time
- ERQ_7-10: Additional sensors (humidity, temperature, radar, LDS)
- REQ_13-14: Adaptive brightness based on LDR readings
- REQ_15: Web server for WiFi config (web server code exists but disabled)

## Important Code Sections

| Feature | Lines | Status |
|---------|-------|--------|
| Display initialization | 192-196 | Active |
| WiFi setup | 213-230 | Active |
| NTP initialization | 232 | Active |
| Main loop | 313-339 | Active |
| Web server/client control | 276-312 | Disabled (CLIENT_CTRL) |
| Brightness adaptation stub | 92-114 | Empty implementation |

## Common Tasks & Approaches

**To add a new sensor**:
1. Include library at top
2. Create `#define SENSOR_NAME DISABLE` flag
3. Wrap initialization in `#if (SENSOR_NAME == ENABLE)`
4. Add update logic in main loop (within the same `#if` block)
5. Reference example: sensor code structure mirrors NTP pattern (Init → Update → Display)

**To modify timezone**:
- Change `#define UTC_PLUS_7 25200` to your offset in seconds
- Formula: UTC_offset_hours × 3600

**To adjust display timing**:
- The 1-second delay is in the main loop (lines 333-334)
- Colon blink toggle happens every `delay(1000)` call

**To enable features**:
- Set `#define FEATURE_NAME ENABLE` at the top
- Recompile and upload to ESP8266

## Build & Upload
Uses Arduino IDE with ESP8266 board support. Dependencies in code:
- ESP8266WiFi.h (built-in)
- WiFiManager (library, handles AP setup)
- NTPClient (library)
- TM1637Display (library)
- WiFiUdp.h (built-in)
- DNSServer.h, ESP8266WebServer.h (built-in, for WiFiManager)

Serial monitor: 115200 baud for debugging output.

## Known Quirks & Gotchas
- **Global variable pollution**: `header`, `currentLine`, `output2State` are globals—any new features sharing client communication may conflict
- **Blocking delays**: 1-second delays in main loop; WiFi updates/NTP sync block execution
- **WiFiManager AP name hardcoded**: "eClock_AP" string at line 220
- **Time format**: 24-hour only; no AM/PM support in display
