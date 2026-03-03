# Debug Level Quick Reference

## Change Debug Level (Line 26 in eClock.ino)

```cpp
#define DEBUG_LEVEL 2  // Change this to: 0, 1, 2, or 3
```

## Debug Levels at a Glance

| Level | Name | Shows | Use Case |
|-------|------|-------|----------|
| **0** | PRODUCTION | ❌ ERROR only | Final hardware deployment |
| **1** | WARNING | ⚠️ WARN + ERROR | Testing with minimal logs |
| **2** | INFO | ℹ️ INFO + WARN + ERROR | **Development (default)** |
| **3** | DEBUG | 🐛 DEBUG + all above | Troubleshooting issues |

## Logging Macros

```cpp
LOG_ERROR(msg)           // Only shown at levels 0+
LOG_WARN(msg)            // Only shown at levels 1+
LOG_INFO(msg)            // Only shown at levels 2+ (DEFAULT SHOWN)
LOG_DEBUG(msg)           // Only shown at level 3

LOG_ERRORF(fmt, args)    // Formatted: LOG_ERRORF("[ADC] %u", value);
LOG_WARNF(fmt, args)     // Formatted: LOG_WARNF("[TEMP] %.1f°C", temp);
LOG_INFOF(fmt, args)     // Formatted: LOG_INFOF("[NTP] Synced: %s", time_str);
LOG_DEBUGF(fmt, args)    // Formatted: LOG_DEBUGF("[STATE] %d", state);
```

## Example Usage

```cpp
// Simple message (uses current LOG level)
LOG_INFO("WiFi connected");              // ℹ️ Shows at levels 2+
LOG_WARN("Connection timeout");          // ⚠️ Shows at levels 1+
LOG_ERROR("DHT sensor failed");           // ❌ Shows at levels 0+
LOG_DEBUG("Temperature value calculated"); // 🐛 Shows at level 3 only

// Formatted messages with data
LOG_INFOF("[DHT] Temp: %.1f°C, Humid: %.1f%%", 24.5, 65.0);
LOG_DEBUGF("[BRIGHTNESS] ADC=%u, Level=%u", 512, 4);
LOG_WARNF("[RTC] Sync failed, error code: %d", errno);
LOG_ERRORF("[TIMEOUT] Waited %lu ms", elapsed_ms);
```

## Module Tags (in messages)

```
[DISPLAY]   - 7-segment LED display
[WiFi]      - WiFi connection & status
[NTP]       - NTP time synchronization
[RTC]       - Real-time clock module
[EEPROM]    - Flash memory storage
[DHT]       - Humidity & temperature sensor
[BRIGHTNESS] - Light sensor adaptive brightness
[API]       - Web server API endpoints
[WEB]       - Web server pages
[STARTUP]   - Startup initialization
[TIME]      - Time source management
```

## Serial Monitor Output Examples

### Level 0 (PRODUCTION - Minimal)
```
[ERROR] DHT sensor failed
```

### Level 1 (WARNING)
```
[WARN] Connection timeout
[ERROR] DHT sensor failed
```

### Level 2 (INFO - Recommended)
```
[INFO] WiFi connected
[INFO] NTP Time synced
[INFO] DHT sensor initialized
[WARN] Connection timeout
[ERROR] DHT sensor failed
```

### Level 3 (DEBUG - Verbose)
```
[DEBUG] Checking brightness sensor
[DEBUG] State: Showing Temperature
[INFO] WiFi connected
[DEBUG] Blinking display: ON
[INFO] NTP Time synced at 14:35:42
[INFO] DHT sensor initialized
[DEBUG] Temperature: 24.5°C
[WARN] Connection timeout
[ERROR] DHT sensor failed
```

## Best Practices

✅ **DO:**
- Use `LOG_INFOF()` for formatted output: `LOG_INFOF("[DHT] %.1f°C", temp);`
- Set `DEBUG_LEVEL 0` for production deployment
- Set `DEBUG_LEVEL 2` during development
- Use module tags: `LOG_INFO("[WiFi] Connected");`
- Use appropriate severity levels

❌ **DON'T:**
- Use `Serial.println()` directly (use LOG_* macros instead)
- Mix debug levels for the same operation
- Use debug-only macros in critical sections
- Leave DEBUG_LEVEL at 3 in production (performance impact)

## Quick Settings

```cpp
// For Development
#define DEBUG_LEVEL 2  // ✅ Recommended for normal coding

// For Testing
#define DEBUG_LEVEL 1  // Shows warnings/errors only

// For Production
#define DEBUG_LEVEL 0  // Minimal overhead, errors only

// For Troubleshooting
#define DEBUG_LEVEL 3  // Maximum verbosity
```

## Performance Impact

- **Level 0**: No overhead from logging
- **Level 1**: Minimal overhead (~5-10 Serial calls/minute)
- **Level 2**: Moderate overhead (~20-50 Serial calls/minute) ← Recommended for dev
- **Level 3**: High overhead (~50-100 Serial calls/minute) ← Troubleshooting only

---
**Tip:** Set to level 2 for development, level 0 for production deployment.
