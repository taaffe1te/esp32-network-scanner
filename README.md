# ESP32 Network Monitor

A standalone Wi-Fi network security monitor built on ESP32 with a 1.14" ST7789 TFT display. Detects unauthorized devices on a local network using raw ICMP and ARP — no cloud, no dependencies, fully embedded.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue?style=flat)
![Language](https://img.shields.io/badge/Language-C++-00599C?style=flat&logo=cplusplus&logoColor=white)
![Display](https://img.shields.io/badge/Display-ST7789%201.14%22-orange?style=flat)
![Category](https://img.shields.io/badge/Category-IoT%20Security-red?style=flat)

---

## What it does

- Scans the local Wi-Fi subnet every 60 seconds using raw ICMP Echo (ping) over lwIP raw sockets
- Resolves MAC addresses directly from the ARP table (no external ARP library)
- Maintains a persistent database of authorized devices in ESP32 NVS (non-volatile storage)
- Detects and alerts on any new/unknown device appearing in the network
- Displays real-time status, device list, threat details, and scan statistics on a 240×135 TFT screen

## Security model

On first boot, all discovered devices are automatically authorized and saved to NVS flash. On every subsequent scan, any device not in the authorized list triggers a **THREAT DETECTED** alert on the display. The operator can authorize new devices manually via a button press.

This approach mirrors the whitelist model used in industrial network monitoring (ICS/OT security).

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32 (tested on TTGO T-Display) |
| Display | ST7789 1.14" 240×135 TFT via SPI |
| Interface | 2 buttons (next screen / prev screen / long press to scan) |
| Storage | ESP32 NVS (Preferences) for device database |

## Architecture

```
main.ino
├── config.h          — Wi-Fi credentials, scan intervals, limits
├── scanner.h         — Subnet scanner: ICMP ping loop + MAC resolution
├── icmp_ping.h       — Raw ICMP Echo over lwIP sockets + ARP table lookup
├── device_db.h       — Authorized device database (NVS-backed whitelist)
├── display.h         — UI: 4-screen display manager (status/devices/threats/stats)
└── st7789.h          — Custom ST7789 SPI driver (no Adafruit dependency)
```

## Screens

| Screen | Content |
|--------|---------|
| Network Status | Wi-Fi connection, IP, RSSI signal bars, threat banner |
| Devices | Total / authorized / unknown device count + IP list |
| Threats | IP and MAC address of each unknown device |
| Statistics | Time since last scan, uptime, total scans performed |

## Key implementation details

**Raw ICMP ping** — implemented from scratch using `lwip/sockets.h`. Sends ICMP Echo Request (type 8), waits for Echo Reply (type 0) with configurable timeout. No `ping` library used.

**ARP MAC resolution** — after a successful ping, MAC address is retrieved by iterating `etharp_get_entry()` directly from the lwIP ARP table. Works without sending additional ARP requests.

**NVS whitelist** — device authorization state persists across reboots using `Preferences` (ESP32 NVS). MAC address hex string used as key.

**Custom ST7789 driver** — display driven via raw SPI commands without Adafruit GFX or TFT_eSPI. Includes 5×7 bitmap font stored in PROGMEM.

## Build & Flash

1. Open in Arduino IDE 2.x
2. Install board: **ESP32 by Espressif** via Board Manager
3. Set `config.h` credentials:
```cpp
#define WIFI_SSID     "your_network"
#define WIFI_PASSWORD "your_password"
```
4. Select board: **TTGO T1** (or ESP32 Dev Module)
5. Flash at 921600 baud

## Possible extensions

- [ ] Web interface for remote device authorization
- [ ] MQTT alert publishing to a broker
- [ ] Passive ARP sniffing (no ping — fully passive detection)
- [ ] OUI vendor lookup from MAC prefix
- [ ] Export scan log over serial / SD card

## Author

Makar — IoT & Embedded Security, MSTU Bauman 2025

---

*Built as a graduation thesis project in embedded systems security.*
