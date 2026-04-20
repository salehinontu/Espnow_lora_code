# ESP-NOW & LoRa Auto-Switch Communication System

A wireless communication system for ESP32 that automatically switches between **ESP-NOW** (short-range, high-speed) and **LoRa** (long-range, low-power) based on signal quality and distance — giving the best of both protocols seamlessly.

---

## Overview

| Feature | ESP-NOW | LoRa |
|---|---|---|
| Range | ~200–500 m | ~2–15 km |
| Speed | Up to 1 Mbps | 0.3–50 kbps |
| Power | Low | Very Low |
| Best Use | Close-range, real-time | Long-range, low-data |

The auto-switch logic monitors RSSI and link quality. When the ESP-NOW signal degrades below a threshold, the system seamlessly falls back to LoRa — and switches back when the ESP-NOW link recovers.

---

## Hardware Required

- **2x ESP32** development boards (e.g., ESP32 DevKit v1)
- **2x LoRa module** — SX1276/SX1278 (e.g., Ra-02, RFM95W)
- Jumper wires
- USB cables for programming

### LoRa Wiring (SPI — ESP32)

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

---

## Software Dependencies

Install via **Arduino Library Manager** or **PlatformIO**:

- [`ESP32 Arduino Core`](https://github.com/espressif/arduino-esp32) — for ESP-NOW support
- [`LoRa`](https://github.com/sandeepmistry/arduino-LoRa) by Sandeep Mistry
- `esp_now.h` — built into ESP32 Arduino Core

---

## Project Structure

```
esp_now_lora_auto_switch/
├── sender/
│   └── sender.ino          # Transmitter node
├── receiver/
│   └── receiver.ino        # Receiver node
├── lib/
│   └── auto_switch.h       # Switch logic (RSSI threshold)
└── README.md
```

---

## How It Works

```
┌─────────────────────────────────────────────┐
│              Sender Node (ESP32)            │
│                                             │
│  1. Try ESP-NOW → check ACK & RSSI          │
│  2. If RSSI < threshold → switch to LoRa    │
│  3. Keep checking → switch back if improved │
└─────────────────────────────────────────────┘
                     │
          [ESP-NOW / LoRa packet]
                     │
┌─────────────────────────────────────────────┐
│             Receiver Node (ESP32)           │
│                                             │
│  1. Listen on both ESP-NOW & LoRa           │
│  2. Accept whichever arrives first          │
│  3. Respond with ACK                        │
└─────────────────────────────────────────────┘
```

### Auto-Switch Logic

```cpp
if (espnow_rssi > RSSI_THRESHOLD) {
    use_espnow();       // Fast, short-range
} else {
    use_lora();         // Reliable, long-range
}
```

---

## Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/salehinontu/Espnow_lora_code.git
cd Espnow_lora_code
```

### 2. Flash the Sender

Open `sender/sender.ino` in Arduino IDE, select your ESP32 board and COM port, then upload.

### 3. Flash the Receiver

Open `receiver/receiver.ino`, select your ESP32 board, and upload.

### 4. Monitor

Open Serial Monitor at **115200 baud** to see which protocol is active and the incoming data.

---

## Configuration

Edit these defines in the code to tune behavior:

```cpp
#define RSSI_THRESHOLD     -70    // dBm — below this, switch to LoRa
#define LORA_FREQUENCY     433E6  // 433 MHz (change to 868E6 or 915E6 as needed)
#define ESPNOW_CHANNEL     1      // WiFi channel for ESP-NOW
#define SWITCH_HYSTERESIS  5      // dBm hysteresis to avoid flapping
```

---

## Serial Output Example

```
[ESP-NOW] RSSI: -55 dBm → Using ESP-NOW
Data received: "Hello from sender" | Protocol: ESP-NOW

[ESP-NOW] RSSI: -80 dBm → Switching to LoRa
Data received: "Hello from sender" | Protocol: LoRa

[ESP-NOW] RSSI: -60 dBm → Switching back to ESP-NOW
```

---

## License

MIT License — free to use, modify, and distribute.

---

## Author

**Saleh** — [@salehinontu](https://github.com/salehinontu)
