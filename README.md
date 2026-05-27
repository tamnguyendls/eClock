# eClock — First Time Setup Guide

## What you need
- ESP8266 NodeMCU board (with eClock firmware already uploaded)
- A phone or laptop with WiFi
- Your home WiFi network name (SSID) and password

---

## Step 1 — Power on the board

Connect the board to USB power. The LED display will show blinking `--:--`, meaning it is waiting for WiFi.

---

## Step 2 — Connect to eClock hotspot

On your phone or laptop, open WiFi settings and connect to:

```
Network name: eClock_AP
Password:     (none)
```

> The board broadcasts this open hotspot automatically on first boot, or whenever it cannot find a saved WiFi network.

---

## Step 3 — Open the setup page

Open a browser and go to:

```
http://192.168.4.1
```

You will see the **eClock WiFi Setup** page.

---

## Step 4 — Enter your WiFi credentials

1. The page will show a list of nearby WiFi networks
2. Tap your home network name
3. Enter your WiFi password
4. Press **Connect**

---

## Step 5 — Wait for connection

The board will restart and try to connect to your WiFi.

| Display | Meaning |
|---------|---------|
| Blinking `--:--` | Connecting to WiFi / waiting for time sync |
| `HH:MM` with blinking colon | Connected and showing time |
| All segments ON `8888` | WiFi connection failed |

> First time sync may take up to 30 seconds depending on your network.

---

## Next time you power on

The board remembers your WiFi credentials and timezone. It will connect automatically without needing to repeat the setup steps.

---

## Factory Reset (Hardware Button)

> **Planned feature — not yet implemented**

A dedicated hardware reset button will be added to the board. Pressing and holding it will perform a full factory reset:

- Clears saved WiFi credentials
- Clears saved timezone setting
- Restarts board and re-enters AP mode (`eClock_AP`)

After factory reset, repeat from **Step 2** to reconfigure.

---

## Timezone

Timezone is configured during the WiFi setup process (Step 4) and saved to flash memory.

> **Planned feature — not yet implemented**

The WiFi setup page will include a **timezone selector** after the password field. The selected timezone offset will be saved to flash and applied on every boot — no need to re-enter unless factory reset is performed.

**Common offsets for reference:**

| Timezone | Offset (seconds) |
|----------|-----------------|
| UTC+7 (Vietnam, Thailand) | 25200 |
| UTC+8 (Singapore, China) | 28800 |
| UTC+9 (Japan, Korea) | 32400 |
| UTC+0 (UK) | 0 |
| UTC-5 (US Eastern) | -18000 |

Until this feature is implemented, timezone is set at compile time in `eClock.ino`:

```cpp
#define UTC_PLUS_7  25200   // change this value to your offset
```
