# eClock — ESP32-C3 + WS2812 8x32 Matrix Variant

Branch: `ESP32-C3-WS2812-Clock`

Same features and configuration as the ESP8266/TM1637 clock (NTP + WiFiManager `eClock_AP`, UTC+7, EEPROM fallback, DHT11, LDR auto-brightness, Time → Temp → Humidity rotation). Only the MCU and the display change.

## Hardware

| Part | Notes |
|------|-------|
| ESP32-C3 board | ESP32-C3 SuperMini / DevKitM-1 / similar |
| WS2812B 8x32 RGB matrix | 256 LEDs, 5V |
| 5V power supply | ≥ 2A recommended (firmware caps brightness at 64/255) |
| DHT11 / DHT22 | Optional (`DHT_SENSOR`) |
| LDR + 10kΩ resistor | Optional (`AUTO_BRIGHTNESS`) |
| 330Ω resistor, 1000µF capacitor | Recommended for the matrix data line / power input |

## Pins

| Function | ESP32-C3 GPIO | Define |
|----------|---------------|--------|
| Matrix DIN | GPIO7 | `MATRIX_PIN` |
| DHT data | GPIO10 | `DHT_PIN` |
| LDR (ADC) | GPIO3 | `LDR_PIN` |
| RTC SDA / SCL | GPIO5 / GPIO6 | `RTC_SDA_PIN` / `RTC_SCL_PIN` |
| Status LED | GPIO8 | `WIFI_LED` |

Avoid GPIO2/8/9 (strapping pins) for external parts, and GPIO18/19 (USB).

Wiring:
- Power the matrix directly from the 5V supply, **not** through the board's 5V pin. Connect the supply GND to the ESP32-C3 GND.
- DIN ← GPIO7 through a 330Ω resistor. The ESP32-C3 drives 3.3V logic; most WS2812B panels accept it. If the display flickers, add a 74AHCT125 level shifter.
- LDR divider: 3.3V — LDR — GPIO3 — 10kΩ — GND (same arrangement as A0 on the NodeMCU).

## Arduino IDE setup

1. Board package: **esp32 by Espressif** (Boards Manager).
2. Board: **ESP32C3 Dev Module**. Set **USB CDC On Boot: Enabled** to see Serial output over native USB (115200 baud).
3. Libraries: WiFiManager, NTPClient, DHT sensor library, Adafruit Unified Sensor, RTClib (zips in `Lib/` or Library Manager), plus **Adafruit NeoPixel** (Library Manager). TM1637 is no longer needed.

## Display

- Built-in 5x7 font (no Adafruit_GFX needed), text centered on the 32-column panel.
- Time: `HH:MM` in orange with a blinking colon. Temperature: `25°C` in red-orange. Humidity: `65%` in blue. Waiting for time: `--:--` in green. WiFi failure: `88:88` in red.
- Colors: `COLOR_*` defines. Brightness: the 0–7 levels from `GetBrightnessLevel()` map to `MATRIX_LEVELS[]`.
- `analogReadResolution(10)` keeps the ADC at 0–1023, so the `BRNS_*` thresholds stay the same as on the ESP8266.

### If text appears mirrored or scrambled

Panels differ in how the LEDs are chained. Adjust:

| Symptom | Fix |
|---------|-----|
| Text mirrored left-right | `MATRIX_FLIP_X ENABLE` |
| Text upside down | `MATRIX_FLIP_Y ENABLE` |
| Every other column is flipped | Toggle `MATRIX_SERPENTINE` |
| Text is garbled / diagonal | Toggle `MATRIX_COLUMN_MAJOR` |
