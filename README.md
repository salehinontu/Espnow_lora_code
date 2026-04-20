# ESP-NOW & LoRa Auto-Switch Telemetry System

A dual-protocol telemetry link for ArduPilot drones that automatically switches between **ESP-NOW** (short-range, high-speed) and **LoRa** (long-range, low-power) using a physical toggle switch — giving you the best of both worlds without recompiling.

---

## System Architecture

```
┌─────────────────────────┐          ┌─────────────────────────┐
│      GROUND UNIT        │          │       AIR UNIT          │
│  (ground/ground.ino)    │          │    (air/air.ino)        │
│                         │          │                         │
│  USB ↔ Mission Planner  │◄────────►│  Serial2 ↔ Flight Ctrl  │
│                         │  LoRa /  │                         │
│  Switch → GPIO 25       │ ESP-NOW  │  Switch → GPIO 25       │
│  LED    → GPIO 4        │          │  LED    → GPIO 4        │
└─────────────────────────┘          └─────────────────────────┘
```

| Protocol | Range | Speed | When to use |
|---|---|---|---|
| ESP-NOW | ~200–500 m | ~1 Mbps | Close range, pre-flight, arming |
| LoRa | ~2–15 km | ~50 kbps | Long range, in-flight telemetry |

**Switch open** → LoRa mode (LED ON)  
**Switch closed** (pin to GND) → ESP-NOW mode (LED OFF)

---

## Project Structure

```
esp_now_lora_auto_switch/
├── ground/
│   └── ground.ino       # Ground station — connects to PC/Mission Planner via USB Serial
├── air/
│   └── air.ino          # Air unit — connects to Flight Controller via Serial2
├── mac_finder/
│   └── mac_finder.ino   # Utility: print MAC address to Serial Monitor
└── README.md
```

---

## Hardware Required

- **2× ESP32** development boards (e.g., ESP32 DevKit v1)
- **2× LoRa module** — SX1276/SX1278 (Ra-02 or RFM95W)
- **2× SPDT switch** (or jumper wire)
- **2× LED** + 220Ω resistor
- USB cables, jumper wires

### LoRa Wiring (SPI — same for both units)

| LoRa Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| NSS / CS | GPIO 5 |
| RST | GPIO 14 |
| DIO0 | GPIO 2 |

### Air Unit Extra Wiring (Flight Controller UART)

| ESP32 | Flight Controller |
|---|---|
| GPIO 16 (RX) | FC TX (SERIAL1) |
| GPIO 17 (TX) | FC RX (SERIAL1) |
| GND | GND |

> ArduPilot setting: `SERIAL1_BAUD = 57` (57600 baud)

---

## Software Dependencies

Install via **Arduino Library Manager**:

- `ESP32 Arduino Core` — [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
- `LoRa` by Sandeep Mistry — [sandeepmistry/arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa)
- `esp_now.h` / `esp_wifi.h` — built into ESP32 core

---

## Quick Start Guide

### Step 1 — Find MAC Addresses

Upload `mac_finder/mac_finder.ino` to **each** ESP32 one at a time.  
Open Serial Monitor at **115200 baud** and note down both MACs.

```
================================
  ESP32 MAC ADDRESS FINDER
================================
MY MAC: F0:24:F9:0E:0C:F0
================================
Format for code:
uint8_t mac[] = {0xF0, 0x24, 0xF9, 0x0E, 0x0C, 0xF0};
```

### Step 2 — Set MAC Addresses in Code

**In `ground/ground.ino`** — paste the **AIR unit's MAC**:
```cpp
uint8_t airMac[] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6};  // ← replace with air MAC
```

**In `air/air.ino`** — paste the **GROUND unit's MAC**:
```cpp
uint8_t groundMac[] = {0xF0, 0x24, 0xF9, 0x0E, 0x0C, 0xF0};  // ← replace with ground MAC
```

### Step 3 — Flash

| File | Flash to |
|---|---|
| `ground/ground.ino` | Ground ESP32 (connected to PC) |
| `air/air.ino` | Air ESP32 (connected to Flight Controller) |

### Step 4 — Connect Mission Planner

- COM port: whichever the Ground ESP32 is on
- Baud rate: **57600**

---

## Configuration

Edit these defines to tune RF parameters (must match on both units):

```cpp
#define LORA_FREQ       433E6    // 433 MHz — change to 868E6 or 915E6 as needed
#define LORA_SF         7        // Spreading factor (7–12, higher = longer range, slower)
#define LORA_BW         500E3    // Bandwidth
#define LORA_CR         5        // Coding rate
#define LORA_TX_PW      17       // TX power in dBm (max 20)
```

---

## LED Status

| LED | Mode |
|---|---|
| ON | LoRa active (long-range) |
| OFF | ESP-NOW active (short-range) |

---

## License

MIT License — free to use, modify, and distribute.

---

## Author

**Saleh** — [@salehinontu](https://github.com/salehinontu)
