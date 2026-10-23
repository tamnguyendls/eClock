# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

eClock is an ESP8266 (NodeMCU) Arduino firmware: an internet clock on a TM1637 4-digit 7-segment display. It gets time via NTP over WiFi (WiFiManager captive portal, SSID `eClock_AP`), with optional DS3231 RTC / EEPROM fallback, a DHT11/22 temp/humidity sensor, and LDR-based auto-brightness.

## Build / run

There is no build system, test suite, or linter. Everything is built with the Arduino IDE (or arduino-cli) targeting an ESP8266 board (NodeMCU 1.0). Serial monitor is 115200 baud.

- Required libraries are vendored as zips in `Lib/` (Adafruit_BusIO, Adafruit_Sensor, DHT-sensor-library, RTClib, TM1637, WiFiManager). Install via Arduino IDE → Sketch → Include Library → Add .ZIP Library. NTPClient is also needed (not in `Lib/`).
- The whole sketch is `eClock.ino` (single file, ~1200 lines) plus `wifi_setup.h`. The folder name must stay `eClock` to match the `.ino`.
- `wifi_setup_test.html` is a standalone copy of the setup UI for previewing in a browser.
- Verification is on hardware only; the user flashes the board. You can't run the firmware here.

## Architecture

**Feature-flag driven.** `eClock.ino` opens with `#define X ENABLE|DISABLE` flags (`LED_DISPLAY`, `WIFI_MANAGER`, `NTP_SERVER`, `AUTO_BRIGHTNESS`, `RTC_BACKUP`, `EEPROM_FALLBACK`, `TIME_VALIDITY_CHECK`, `DHT_SENSOR`, `CLIENT_CTRL`, `TEST_DISPLAY`). All feature code is wrapped in `#if (FLAG == ENABLE)`, including call sites in `setup()`/`loop()`. When adding a feature, add a flag and guard both its definitions and its calls. `RTC_BACKUP` and `CLIENT_CTRL` are currently DISABLED.

**Logging.** Use `LOG_ERROR/WARN/INFO/DEBUG` (and `...F` printf variants) — compiled in/out by `DEBUG_LEVEL` (0–3). `SERIAL_PRINTLN` is a legacy alias for `LOG_INFO`. Wrap string literals in `F()`. See `doc/DEBUG_LEVELS.md`.

**Control flow.** `setup()`: blank display → show `--:--` → `WiFiManager::autoConnect("eClock_AP")` (blocking) → `InitTimeClient()` + forced `UpdateNTPWithInterval()` → optional RTC/DHT init. `loop()` is a single blocking pass: `HandleStartupBlinking()` → `UpdateNTPWithInterval()` (rate-limited by `NTP_UPDATE_INTERVAL`, 60 s) → `timeIsValid` gate (shows `--:--` and returns) → optional `ApplyTimeSource()` → `AdaptDisplayBrightness()` → `HandleDataDisplaySequence()` → colon-blink render using two `delay(1000)` calls. Because of those delays, the loop runs at ~2 s cadence; anything time-sensitive must account for that.

**Time source priority (REQ_5):** NTP → RTC (if `RTC_BACKUP`) → EEPROM-saved epoch + `millis()` delta (`EEPROM_TIME_ADDR`/`EEPROM_MILLIS_ADDR`). Timezone is compile-time via `UTC_PLUS_7` / `utcOffsetInSeconds` (REQ_17 would move it to flash).

**Display state machine.** `HandleDataDisplaySequence()` rotates `DISPLAY_STATE_TIME → TEMP → HUMIDITY` (`TEMP_DISPLAY_INTERVAL`/`TEMP_DISPLAY_DURATION`); DHT is read every `DHT_READ_INTERVAL` (5 min) with cached values.

**Brightness.** `ReadEnvBrightness()` smooths the LDR on `A0` (`BRIGHTNESS_SMOOTHING` samples), `GetBrightnessLevel()` maps ADC thresholds (`BRNS_*`) to TM1637 levels 0–7 with hysteresis.

**Web UI (`wifi_setup.h`).** Contains `WIFI_SETUP_HTML` as a PROGMEM raw string literal (HTML/CSS/JS in one blob); `eClock.ino` reads it with `FPSTR()` and injects a scanned-network JSON.

**Gotcha — web server code is dead by default.** `ServeDataEndpoint` (`/api/data`) and `ServeDashboardPage` (`/dashboard`) are always compiled, but the routing to them and the setup-page handler live in `checkForControlFromClient`, which is only compiled under `CLIENT_CTRL` (DISABLED), and `loop()` never calls `wifi_server.available()`. `wifi_server.begin()` runs in `setup()`, but nothing serves requests unless that wiring is added. Docs/Requirement.txt describe the dashboard as working, so verify before relying on them.

**Pins:** TM1637 CLK=D6, DIO=D5; DHT=D1; RTC I2C SDA=D4/SCL=D3; LDR=A0; status LED=GPIO2 (note `output2` is also GPIO2).

## Requirements & docs

`Requirement.txt` is the source of truth for REQ_*/ERQ_* IDs and their status; code comments reference these IDs (e.g. `// REQ_6`), so keep them when editing. Not implemented: ERQ_8 (radar), ERQ_9 (LDS), REQ_16 (factory-reset button), REQ_17 (timezone in setup page). `doc/` holds per-feature guides; `README.md` is the end-user first-time setup guide.

`.github/copilot-instructions.md` is outdated (stale line numbers; lists REQ_5/6/13/14 as unimplemented though they're done) — trust the code and `Requirement.txt` over it.
