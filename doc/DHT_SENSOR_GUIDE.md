# DHT Sensor Implementation Guide (ERQ_7, REQ_12)

## Overview
eClock now supports DHT11 or DHT22 humidity and temperature sensors. The device reads sensor data every 5 minutes and displays temperature on the LED for 10 seconds every 60 seconds.

## Hardware Requirements

### Components
- **DHT11 or DHT22** temperature/humidity sensor module
- **10kΩ resistor** (optional pull-up for data line)
- **Jumper wires** for connections

### Wiring Diagram
```
DHT Sensor Pin Layout (3-pin module):
Pin 1 (VCC)   → ESP8266 3.3V
Pin 2 (DATA)  → D1 (GPIO5) with optional 10kΩ pull-up to 3.3V
Pin 3 (GND)   → ESP8266 GND
```

### NodeMCU Pin Reference
| Pin | GPIO | Function |
|-----|------|----------|
| D1  | GPIO5 | DHT DATA (configurable) |
| 3.3V | - | Power |
| GND | - | Ground |

## Software Configuration

### 1. Installation
Add the DHT library in Arduino IDE:
- **Sketch** → **Include Library** → **Manage Libraries**
- Search for: `"Adafruit DHT sensor library"`
- Click **Install**

### 2. Enable Feature
In `eClock.ino`, line ~24:
```cpp
#define DHT_SENSOR            ENABLE    // Set to ENABLE to activate
#define DHT_PIN               D1        // GPIO5 (NodeMCU D1)
#define DHT_TYPE              DHT11     // DHT11 or DHT22
```

### 3. Adjust Timing (Optional)
```cpp
#define DHT_READ_INTERVAL     300000    // 5 minutes (300000 ms)
#define TEMP_DISPLAY_DURATION 10000     // 10 seconds (can change to 5000-15000)
#define TEMP_DISPLAY_INTERVAL 60000     // 60 seconds = 1 minute
```

## Features

### Display Rotation Behavior
```
Timeline:
Second 0:   [HH:MM] Time display (2 seconds)
Second 2:   [HH:MM] Time display (2 seconds)
...
Second 58:  [HH:MM] Time display
Second 60:  [TT] Temperature display (10 seconds)
            - Shows current temperature (e.g., "25" for 25°C)
Second 70:  [HH:MM] Back to time display
            Loop repeats
```

### Display Example
- **Time**: `12:34` (with blinking colon)
- **Temperature**: `25` (means 25°C with optional degree symbol)

### Serial Monitor Output
```
[DHT] Humidity & Temperature Sensor initialized
[DHT] Temperature: 24.50°C, Humidity: 65.00%
[DHT] Displaying: 24°C
[DHT] Switching to temperature display
[DHT] Switching back to time display
```

## API Reference

### Function: `InitDHTSensor()`
```cpp
void InitDHTSensor();
```
- **Purpose**: Initialize DHT sensor
- **Call**: In `setup()` (automatically done if `DHT_SENSOR == ENABLE`)
- **Returns**: void

### Function: `ReadDHTSensor()`
```cpp
bool ReadDHTSensor();
```
- **Purpose**: Read humidity and temperature from sensor
- **Call**: Automatically called by `HandleTemperatureDisplay()`
- **Frequency**: Every 5 minutes (DHT_READ_INTERVAL)
- **Returns**: `true` if successful, `false` if read fails

### Function: `DisplayTemperature()`
```cpp
void DisplayTemperature();
```
- **Purpose**: Show temperature on 7-segment display
- **Format**: 2-digit temperature (0-99°C)
- **Example**: 25°C displays as `[25]`

### Function: `HandleTemperatureDisplay()`
```cpp
void HandleTemperatureDisplay();
```
- **Purpose**: Manage temperature display rotation
- **Call**: Every loop iteration (in `loop()`)
- **Behavior**: 
  - Switches to temperature every 60 seconds
  - Displays for 10 seconds then back to time
  - Reads sensor every 5 minutes

## Troubleshooting

### Problem: "DHT library not found"
**Solution**: 
1. In Arduino IDE, go to **Sketch** → **Include Library** → **Manage Libraries**
2. Search for `"Adafruit DHT sensor library"`
3. Click **Install** button
4. Restart Arduino IDE

### Problem: Sensor reads fail
**Error**: `[DHT] Failed to read from DHT sensor!`

**Causes**:
- Loose wiring (check all connections)
- DHT data pin not set correctly (verify `#define DHT_PIN D1`)
- Sensor powered off or damaged
- Missing pull-up resistor on data line

**Solutions**:
1. Verify wiring with multimeter
2. Add 10kΩ pull-up resistor between DATA and 3.3V
3. Test sensor with standalone DHT example sketch
4. Replace sensor if damaged

### Problem: Temperature value unrealistic
**Examples**: -1°C, 100°C, or NaN

**Causes**:
- Bad sensor read (transient)
- Sensor in direct sunlight or near heat source
- Sensor cable too long (>1 meter)

**Solutions**:
1. Wait for next read cycle (5 minutes)
2. Move sensor to room temperature
3. Keep sensor cable short (<30cm)
4. Check sensor calibration on datasheet

## Comparison: DHT11 vs DHT22

| Feature | DHT11 | DHT22 |
|---------|-------|-------|
| Temperature Range | 0-50°C | -40 to 80°C |
| Humidity Range | 20-80% | 0-100% |
| Accuracy Temp | ±2°C | ±0.5°C |
| Accuracy Humidity | ±5% | ±2% |
| Cost | ~$1 | ~$5 |
| Response Time | ~1s | ~2s |

**Recommendation**: DHT11 for basic indoor use, DHT22 for wider range/accuracy

## Advanced Configuration

### Change Display Duration
```cpp
// Show temperature for 5 seconds instead of 10
#define TEMP_DISPLAY_DURATION 5000
```

### Change Display Frequency
```cpp
// Show temperature every 30 seconds instead of 60
#define TEMP_DISPLAY_INTERVAL 30000
```

### Change Read Frequency
```cpp
// Read sensor every 10 minutes instead of 5
#define DHT_READ_INTERVAL 600000  // 600000ms = 10 minutes
```

### Use DHT22 Instead
```cpp
#define DHT_TYPE              DHT22     // Change from DHT11 to DHT22
```

## Requirements Status

| Requirement | Status | Notes |
|------------|--------|-------|
| ERQ_7: Humidity sensor | ✅ IMPLEMENTED | DHT11/DHT22 support |
| REQ_12: Update every 5 mins | ✅ IMPLEMENTED | Reads every 300 seconds |
| Temperature display | ✅ IMPLEMENTED | Shows for 10 seconds every 60 seconds |
| Humidity caching | ✅ IMPLEMENTED | Cached in memory |

## Related Features
- [Auto Brightness](AUTO_BRIGHTNESS_GUIDE.md) - Can be adjusted based on temperature
- [Time Validity Check](REQUIREMENTS_IMPLEMENTATION.md) - Ensures valid time before display
- [RTC Backup](RTC_SETUP_GUIDE.md) - Can sync with temperature sensor data

## References
- [Adafruit DHT Library](https://github.com/adafruit/DHT-sensor-library)
- [DHT Sensor Datasheet](https://www.adafruit.com/product/386)
- [Random Nerd Tutorials DHT Guide](https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/)

## Support
For issues:
1. Check serial monitor output: `115200` baud
2. Verify wiring against hardware section
3. Test with DHT example from library
4. Check if sensor is powered and not damaged
