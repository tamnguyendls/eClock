# eClock — Wiring Guide

All modules run from the NodeMCU **3V3** rail. Wire with the board **unplugged** from USB.

## Pin map (matches `eClock.ino`)

| Module | Module pin | NodeMCU pin | GPIO | Defined in firmware |
|--------|-----------|-------------|------|---------------------|
| TM1637 display | VCC | 3V3 | — | — |
| | GND | GND | — | — |
| | CLK | **D6** | GPIO12 | `const int CLK = D6` |
| | DIO | **D5** | GPIO14 | `const int DIO = D5` |
| DHT11 / DHT22 | VCC (+) | 3V3 | — | — |
| | DATA (S/OUT) | **D1** | GPIO5 | `#define DHT_PIN D1` |
| | GND (−) | GND | — | — |
| LDR divider | middle point | **A0** | ADC0 | `analogRead(A0)` |
| Status LED | on-board | — | GPIO2 | `WIFI_LED 2` (active LOW, no wiring needed) |

## Step by step

### 1. TM1637 display

```
 TM1637 module             NodeMCU
 ┌─────────┐
 │ CLK ────┼──────────────► D6
 │ DIO ────┼──────────────► D5
 │ VCC ────┼──────────────► 3V3
 │ GND ────┼──────────────► GND
 └─────────┘
```

- Do not swap CLK and DIO. A swap shows a blank display.
- The display can also run from **VIN (5 V)** for a brighter output; the 3.3 V logic levels from the ESP8266 normally still work. Use 3V3 if unsure.

### 2. DHT11 / DHT22

```
 DHT module (3-pin)        NodeMCU
 ┌─────────┐
 │ DATA ───┼──────────────► D1
 │ VCC  ───┼──────────────► 3V3
 │ GND  ───┼──────────────► GND
 └─────────┘
```

- Check the silkscreen: pin order differs between module makers (`S + −`, `+ OUT −`, or `DATA VCC GND`).
- Bare 4-pin DHT: pin 1 VCC, pin 2 DATA, pin 3 not connected, pin 4 GND. Add a 4.7–10 kΩ resistor between DATA and 3V3.
- Run the DHT from **3V3** so its data line never exceeds the ESP8266 limit.

### 3. LDR (auto-brightness)

The firmware treats **a higher ADC value as darker** (`adc ≥ 950` → brightness 7). That requires the LDR on the **ground side** of the divider:

```
 3V3 ────┬──────────────────
         │
       10 kΩ
         │
         ├────────────────── A0
         │
        LDR (GL5516)        optional: 0.1 µF cap from A0 to GND
         │
 GND ────┴──────────────────
```

| Light | LDR resistance | Voltage at A0 | ADC | Display brightness |
|-------|---------------|---------------|-----|--------------------|
| Dark | high | high | high (≥ 950) | 7 (max) |
| Bright | low | low | low (< 50) | 0 (min) |

> **Note:** `doc/AUTO_BRIGHTNESS_GUIDE.md` draws the LDR on the 3.3 V side with the 10 kΩ resistor to GND. That arrangement reverses the behavior (display dims in the dark). Use the wiring above, or swap the two parts' positions if you prefer the guide's layout and then invert the logic in firmware.

- The NodeMCU's on-board divider scales A0 to a 0–3.3 V input. Do not feed more than 3.3 V into A0. On a bare ESP-12 module without that divider, the limit is 1 V.
- Place the LDR where it sees room light but not the display's own glow, or the brightness will oscillate.

### Optional: DS3231 RTC

RTC support is disabled by default (`RTC_BACKUP DISABLE`), and the existing wiring documentation does not work with the current firmware:

- `RTC_SETUP_GUIDE.md` says SDA=D4, SCL=D3, and labels D4 as GPIO4. On NodeMCU, **D4 is GPIO2** and **D3 is GPIO0**. These are boot-strap pins. GPIO2 is also the on-board LED, and `setup()` drives it as an output.
- The code never calls `Wire.begin()`, so the I2C bus uses the ESP8266 defaults: SDA=D2 (GPIO4), SCL=D1 (GPIO5). D1 is the DHT data pin.

Recommended wiring, which needs one firmware change:

```
 DS3231 module             NodeMCU
 ┌─────────┐
 │ SDA ────┼──────────────► D2  (GPIO4)
 │ SCL ────┼──────────────► D7  (GPIO13)
 │ VCC ────┼──────────────► 3V3
 │ GND ────┼──────────────► GND
 └─────────┘
```

Firmware change (not yet made): call `Wire.begin(D2, D7);` before `rtc.begin()` in `InitRTC()`, then set `RTC_BACKUP` to `ENABLE`. D2 and D7 are free in the current pin map and not boot-strap pins.

- Power the module from **3V3**, not 5 V.
- ZS-042 modules have a charging circuit meant for a rechargeable LIR2032. With a non-rechargeable **CR2032**, remove the charging diode or resistor on the module, or use an LIR2032.

## Complete pin summary

| NodeMCU | GPIO | Connected to |
|---------|------|--------------|
| D1 | 5 | DHT DATA |
| D2 | 4 | DS3231 SDA (optional, needs firmware change) |
| D5 | 14 | TM1637 DIO |
| D6 | 12 | TM1637 CLK |
| D7 | 13 | DS3231 SCL (optional, needs firmware change) |
| A0 | ADC0 | LDR divider |
| 3V3 | — | TM1637 VCC, DHT VCC, LDR divider top, DS3231 VCC |
| GND | — | All module grounds, LDR divider bottom |

Avoid D3 (GPIO0), D4 (GPIO2) and D8 (GPIO15): they set the boot mode, and a wrong level at power-up stops the board from starting or flashing.

## Checks after wiring

1. Plug in USB. The display shows blinking `--:--` while it waits for WiFi (see `README.md` for first-time setup).
2. Open the serial monitor at 115200 baud and look for `[INFO]` lines from NTP, DHT and brightness.
3. Cover the LDR. The display should get brighter. If it dims instead, the divider is wired the other way round.
4. A DHT read error in the log usually means a swapped pin or no pull-up on a bare sensor.
