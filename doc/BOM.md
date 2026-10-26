# eClock — Bill of Materials (BOM)

Parts needed to build one eClock. Prices are not listed because they vary by region and supplier.

## Core parts (required)

| # | Part | Qty | Spec / notes | Used for |
|---|------|-----|--------------|----------|
| 1 | ESP8266 NodeMCU dev board | 1 | NodeMCU V3 (ESP-12E/F, CP2102 or CH340 USB-UART). Use a board with the on-board A0 voltage divider (standard NodeMCU) so A0 accepts 0–3.3 V | MCU, WiFi, NTP |
| 2 | TM1637 4-digit 7-segment display module | 1 | 0.56" **clock type with the colon (`:`) segment**. Plain 4-digit modules without a colon connection cannot blink the colon | Time / temp / humidity display |
| 3 | LDR (photoresistor) | 1 | GL5516 or similar (about 5 kΩ–200 kΩ) | Auto-brightness (REQ_13/14) |
| 4 | Resistor 10 kΩ, 1/4 W, 5 % | 1 | Fixed leg of the LDR voltage divider | Auto-brightness |
| 5 | DHT11 sensor, 3-pin module | 1 | Matches `DHT_TYPE DHT11` in firmware. A DHT22 also works; change `DHT_TYPE` to `DHT22`. The 3-pin module has the pull-up on board | Temp / humidity (ERQ_7, REQ_12) |
| 6 | Micro-USB cable (data-capable) | 1 | For flashing and power | Power, programming |
| 7 | USB 5 V power adapter | 1 | 5 V, at least 1 A | Power |
| 8 | Breadboard (830 point) or perfboard | 1 | Prototype first, solder later | Assembly |
| 9 | Jumper wires, dupont | 1 set | Male-male for breadboard; female-female or female-male for modules with header pins | Wiring |

## Optional parts

| # | Part | Qty | Spec / notes | Used for |
|---|------|-----|--------------|----------|
| 10 | DS3231 RTC module (I2C) | 1 | ZS-042 or similar. Needs `RTC_BACKUP` set to `ENABLE` and a firmware change (see [WIRING_GUIDE.md](WIRING_GUIDE.md#optional-ds3231-rtc)) | Time fallback when WiFi is lost (REQ_5, ERQ_10, REQ_11) |
| 11 | CR2032 coin cell | 1 | Keeps RTC time through power loss. On ZS-042 modules, see the charging warning in the wiring guide | RTC backup power |
| 12 | Capacitor 0.1 µF (100 nF) ceramic | 1 | Across the LDR to GND to reduce brightness flicker (suggested in `AUTO_BRIGHTNESS_GUIDE.md`) | Noise filtering |
| 13 | Resistor 4.7 kΩ–10 kΩ | 1 | Only if you use a **bare 4-pin DHT** instead of the 3-pin module: pull-up from DATA to 3.3 V | DHT data line |
| 14 | Enclosure / diffuser | 1 | Cut a window for the display and a small hole for the LDR. Keep the DHT out of heat from the board, otherwise it reads high | Housing |
| 15 | Push button | 1 | Not used by the current firmware. Reserved for REQ_16 (factory reset), which is not implemented | Future |

## Not included

- **Radar (RCWL-0516, ERQ_8)** and **LDS sensor (ERQ_9)** are not implemented in firmware, so they are not in this BOM.

## Software / libraries

Vendored as zips in `Lib/`: Adafruit_BusIO, Adafruit_Sensor, DHT-sensor-library, RTClib, TM1637, WiFiManager. NTPClient must be installed separately. Board support: ESP8266 core for Arduino IDE.
