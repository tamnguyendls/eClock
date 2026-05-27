# eClock Debug Level Configuration

## Overview
The eClock codebase now includes a **Debug Level System** that controls the verbosity of `Serial.println()` and `Serial.print()` messages globally. This allows you to easily switch between **Debug** mode (detailed logging) and **Production** mode (minimal logging) without commenting out individual log statements.

## Debug Levels

### Level 0: PRODUCTION (Minimal Logging)
```cpp
#define DEBUG_LEVEL 0
```
- **Only** error messages are printed (LOG_ERROR)
- Best for final hardware deployment
- Minimal serial output and EEPROM writes
- Fastest performance

### Level 1: WARNING (Errors + Warnings)
```cpp
#define DEBUG_LEVEL 1
```
- **ERROR** messages (LOG_ERROR)
- **WARNING** messages (LOG_WARN)
- Useful for detecting issues in production
- Slightly more serial output

### Level 2: INFO (Default - Recommended for Development)
```cpp
#define DEBUG_LEVEL 2
```
- **ERROR** messages (LOG_ERROR)
- **WARNING** messages (LOG_WARN)
- **INFO** messages (LOG_INFO)
- General operational information
- **Recommended for normal development**
- Good balance between detail and performance

### Level 3: DEBUG (All Messages - Maximum Verbosity)
```cpp
#define DEBUG_LEVEL 3
```
- **ERROR** messages (LOG_ERROR)
- **WARNING** messages (LOG_WARN)
- **INFO** messages (LOG_INFO)
- **DEBUG** messages (LOG_DEBUG)
- Detailed debug information
- Best for troubleshooting complex issues
- Maximum serial output and slowest performance

## Configuration

Edit the top of `eClock.ino` (line ~20):

```cpp
/* ========================================================================================
   DEBUG LEVEL CONFIGURATION (Switch between DEBUG and PRODUCTION)
   ========================================================================================
   DEBUG_LEVEL: Controls verbosity of Serial.println() messages
   Options:
     0 = PRODUCTION (minimal logging - only errors)
     1 = WARNING    (errors + warnings)
     2 = INFO       (errors + warnings + info messages - RECOMMENDED FOR DEVELOPMENT)
     3 = DEBUG      (all messages including detailed debug info)
   ======================================================================================== */
#define DEBUG_LEVEL 2  // <-- Change this number (0, 1, 2, or 3)
```

## Logging Macros

### Available Logging Functions

| Macro | Level | Usage | Example |
|-------|-------|-------|---------|
| `LOG_ERROR(msg)` | 0+ | Critical errors | `LOG_ERROR("[DHT] Sensor failed");` |
| `LOG_WARN(msg)` | 1+ | Warnings | `LOG_WARN("[WiFi] Connection timeout");` |
| `LOG_INFO(msg)` | 2+ | Informational | `LOG_INFO("[NTP] Time synced");` |
| `LOG_DEBUG(msg)` | 3+ | Debug details | `LOG_DEBUG("[DHT] Showing Temperature");` |
| `LOG_ERRORF(fmt, ...)` | 0+ | Formatted errors | `LOG_ERRORF("[ERROR] Code: %d", errno);` |
| `LOG_WARNF(fmt, ...)` | 1+ | Formatted warnings | `LOG_WARNF("[ADC] Value: %u", adc_val);` |
| `LOG_INFOF(fmt, ...)` | 2+ | Formatted info | `LOG_INFOF("[NTP] Time: %s", time_str);` |
| `LOG_DEBUGF(fmt, ...)` | 3+ | Formatted debug | `LOG_DEBUGF("[TEMP] %.1f°C", temp_val);` |

### Formatting Examples

```cpp
// Simple string
LOG_INFO("WiFi connected");

// With formatted data
LOG_INFOF("[DHT] Temperature: %.1f°C, Humidity: %.1f%%", 24.5, 65.0);

// With integers
LOG_DEBUGF("[BRIGHTNESS] ADC=%u, Level=%u", 512, 4);

// With multiple types
LOG_WARNF("[RTC] Time %04u-%02u-%02u %02u:%02u", year, month, day, hour, minute);
```

## Serial Output Examples

### DEBUG_LEVEL 0 (PRODUCTION)
```
[ESP8266 booting...]
[Connected to WiFi]
[Time valid]
[Ready]
```
*Only shows critical information*

### DEBUG_LEVEL 1 (WARNING)
```
[WARN] [WiFi] Connection timeout
[ERROR] [DHT] Failed to read from DHT sensor!
```
*Shows warnings and errors only*

### DEBUG_LEVEL 2 (INFO - Default)
```
Enter setup.
[INFO] [DISPLAY] Showing '--:--' (waiting for time)
[INFO] [EEPROM] Initialized for time fallback storage
[INFO] [WiFi] Connect successfully
[INFO] [RTC] DS3231 RTC initialized successfully
[INFO] [DHT] Humidity & Temperature Sensor initialized
[INFO] [NTP] Time synced at 14:35:42
[INFO] [DHT] Temperature: 24.5°C, Humidity: 65.0%
[INFO] [EEPROM] Time saved for WiFi loss fallback
```
*Shows normal operational information*

### DEBUG_LEVEL 3 (DEBUG)
```
Enter setup.
[INFO] [DISPLAY] Showing '--:--' (waiting for time)
[INFO] [EEPROM] Initialized for time fallback storage
[DEBUG] [EEPROM] Elapsed time since save: 3600 seconds
[INFO] [WiFi] Connect successfully
[INFO] [RTC] DS3231 RTC initialized successfully
[DEBUG] [STARTUP] Blinking display: ON (dashes visible)
[DEBUG] [STARTUP] Blinking display: OFF (blank)
[INFO] [DHT] Humidity & Temperature Sensor initialized
[DEBUG] [DHT] State: Showing Temperature
[INFO] [DHT] Displaying: 25°C
[DEBUG] [DHT] State: Showing Humidity
[DEBUG] [API] Served data endpoint
[DEBUG] Brightness adjusted: ADC=512 Level=4
[INFO] [NTP] Time synced at 14:35:42
[INFO] [DHT] Temperature: 24.5°C, Humidity: 65.0%
[INFO] [EEPROM] Time saved for WiFi loss fallback
```
*Shows all operational and debug information*

## Performance Considerations

| Level | Serial Calls | Overhead | Recommendation |
|-------|--------------|----------|-----------------|
| 0 (PRODUCTION) | ~5-10 per cycle | Minimal | Final deployment |
| 1 (WARNING) | ~10-20 per cycle | Low | Testing with logging |
| 2 (INFO) | ~20-50 per cycle | Moderate | **Development** |
| 3 (DEBUG) | ~50-100 per cycle | High | Troubleshooting |

## Migration from Serial.println()

All `Serial.println()` calls in the codebase have been replaced with appropriate `LOG_*` macros:

- **Critical errors** → `LOG_ERROR()`
- **Warning conditions** → `LOG_WARN()`
- **Normal operations** → `LOG_INFO()` (default level 2)
- **Debug details** → `LOG_DEBUG()` (level 3 only)

### Legacy Support
If you need to use old-style `Serial.println()`:
```cpp
SERIAL_PRINTLN(msg)  // Maps to LOG_INFO() automatically
SERIAL_PRINT(msg)    // Only prints if DEBUG_LEVEL >= 2
```

## Recommended Settings

### For Development
```cpp
#define DEBUG_LEVEL 2  // INFO + all debug info needed for development
```

### For Testing
```cpp
#define DEBUG_LEVEL 1  // Only warnings and errors to keep output clean
```

### For Production
```cpp
#define DEBUG_LEVEL 0  // Only errors to minimize overhead
```

### For Troubleshooting
```cpp
#define DEBUG_LEVEL 3  // All information including debug traces
```

## Serial Monitor Setup

Set your serial monitor baud rate to **115200** (standard for ESP8266):

```
Tool → Serial Monitor → Baud Rate: 115200
```

All messages are prefixed with their log level:
- `[ERROR]` - Critical problems
- `[WARN]` - Warning conditions
- `[INFO]` - Informational messages
- `[DEBUG]` - Debug details

## Module-Specific Logging

Messages are tagged by module for easy filtering:
- `[DISPLAY]` - 7-segment display
- `[WiFi]` - WiFi connection
- `[NTP]` - Network time protocol
- `[RTC]` - Real-time clock
- `[EEPROM]` - Flash memory fallback
- `[DHT]` - Temperature/humidity sensor
- `[BRIGHTNESS]` - Light sensor adaptive brightness
- `[API]` - Web server API endpoints
- `[WEB]` - Web server pages
- `[STARTUP]` - Startup/initialization
- `[TIME]` - Time source management

## Performance Testing

To measure performance impact:

1. **DEBUG_LEVEL 0**: Baseline performance
2. **DEBUG_LEVEL 2**: Compare cycle times and memory usage
3. **DEBUG_LEVEL 3**: Identify slow operations

Serial operations are relatively slow on ESP8266. Setting `DEBUG_LEVEL 0` in production eliminates all logging overhead.

---

**Last Updated:** 2026-03-03  
**Version:** 1.0
