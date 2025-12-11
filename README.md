# LoRa TDMA Master-Slave Network with WiFi Upload

A time-division multiple access (TDMA) LoRa network implementation with automatic frame synchronization and WiFi-based data upload. The master node broadcasts frame synchronization messages, and slave nodes transmit in staggered time slots to avoid collisions.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [How It Works](#how-it-works)
- [Project Structure](#project-structure)
- [Usage](#usage)
- [Network Protocol](#network-protocol)
- [LoRa Parameters](#lora-parameters)
- [Power Management](#power-management)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## 🎯 Overview

This project implements a TDMA-based LoRa network where:
- **Master Node (ID 1)**: Broadcasts frame synchronization messages every 10 seconds
- **Slave Nodes (ID 2+)**: Transmit in staggered time slots after receiving frame sync
- **TDMA Scheduling**: Each slave transmits at a specific offset to prevent collisions
- **Data Logging**: Master logs all received data to SPIFFS as NDJSON
- **WiFi Upload**: Master periodically uploads logged data to a webhook URL via HTTP POST

## ✨ Features

- **TDMA Architecture**: Time-division multiple access prevents packet collisions
- **Frame Synchronization**: Master broadcasts "FRAME|counter" every 10 seconds
- **Staggered Transmission**: Slaves transmit 1 second apart (Node 2 at +200ms, Node 3 at +1200ms, etc.)
- **Deep Sleep**: Slaves sleep for 10 seconds after transmission for power efficiency
- **Data Logging**: Master logs all received packets to SPIFFS as newline-delimited JSON
- **WiFi Upload**: Automatic HTTP POST upload to webhook URL every 60 seconds
- **Persistent Counters**: Slave counters survive deep sleep using RTC memory
- **Collision Avoidance**: TDMA scheduling ensures no two nodes transmit simultaneously

## 🔧 Hardware Requirements

- **Heltec ESP32 LoRa V3** (or compatible ESP32 with LoRa module)
- **WiFi Access**: Master node requires WiFi connectivity for data upload
- **Power Supply**: Battery or USB power (for long-term deployment, use battery)

### Pin Connections

- **LoRa Module**: Integrated on Heltec board
  - DIO1 Pin: Used for RX interrupt callback
- **OLED Display**: Integrated (used for status display)
- **WiFi**: Built-in ESP32 WiFi (master node only)

## 💻 Software Requirements

- **Arduino IDE** (1.8.x or 2.x) or **PlatformIO**
- **ESP32 Board Support** for Arduino IDE
- **Required Libraries**:
  - `heltec_unofficial.h` - Heltec ESP32 LoRa board support
  - `WiFi.h` - ESP32 WiFi library (built-in)
  - `HTTPClient.h` - ESP32 HTTP client library (built-in)
  - `SPIFFS.h` - ESP32 SPIFFS file system (built-in)
  - `esp_sleep.h` - ESP32 deep sleep (built-in)
  - RadioLib (included with heltec_unofficial)

## 📦 Installation

### 1. Install Arduino IDE and ESP32 Support

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to **File → Preferences**
   - Add this URL to **Additional Board Manager URLs**:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools → Board → Boards Manager**
   - Search for "ESP32" and install "esp32 by Espressif Systems"

### 2. Install Required Libraries

Install the following libraries via Arduino Library Manager (**Sketch → Include Library → Manage Libraries**):

- **heltec_unofficial** (search for Heltec ESP32 or install from GitHub)

### 3. Clone/Download This Repository

```bash
git clone https://github.com/YOUR_USERNAME/LoRa-RxTx.git
cd LoRa-RxTx
```

### 4. Open the Project

- **For Master Node**: Open `master/master.ino` in Arduino IDE
- **For Slave Nodes**: Open `slave/slave.ino` in Arduino IDE
- Select your board: **Tools → Board → ESP32 Arduino → Heltec ESP32 LoRa V3**
- Select the correct port: **Tools → Port**

## ⚙️ Configuration

### Master Node Configuration (`master/master.ino`)

#### WiFi Settings
```cpp
const char* WIFI_SSID     = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";
```

#### Server Configuration
```cpp
const char* SERVER_URL = "https://your-webhook-url.com/endpoint";
const char* JSON_PATH  = "/data.json";
```

#### LoRa Parameters
```cpp
#define FREQUENCY        916.3        // MHz (adjust for your region)
#define BANDWIDTH        250.0        // kHz (wider bandwidth for faster transmission)
#define SPREADING_FACTOR 9            // 6-12 (lower = faster, less range)
#define TXPOWER          14           // dBm
#define FRAME_PERIOD_MS  10000        // Frame broadcast interval (10 seconds)
```

#### Upload Settings
```cpp
const unsigned long UPLOAD_INTERVAL_MS = 60000;   // Upload every 60 seconds
```

### Slave Node Configuration (`slave/slave.ino`)

#### Node ID
```cpp
#define NODE_ID 3  // Set to 2, 3, 4, etc. for each slave
```

#### LoRa Parameters (must match master)
```cpp
#define FREQUENCY        916.3        // Must match master
#define BANDWIDTH        250.0        // Must match master
#define SPREADING_FACTOR 9            // Must match master
#define TXPOWER          14           // Must match master
```

#### Timing Parameters
```cpp
#define FRAME_PERIOD_MS     10000    // Must match master
#define BASE_OFFSET_MS      200      // Delay after FRAME before first slave
#define PER_NODE_DELAY_MS   1000     // 1 second between each slave
#define SLEEP_SECONDS       10       // Deep sleep duration (nominally 1 frame)
```

## 🔄 How It Works

### Master Node Operation

1. **Initialization**:
   - Initializes SPIFFS file system
   - Connects to WiFi
   - Initializes LoRa radio
   - Starts listening for slave transmissions

2. **Frame Broadcasting**:
   - Every 10 seconds, broadcasts `"FRAME|" + frameCounter`
   - Frame counter increments with each broadcast
   - Example: `"FRAME|42"`

3. **Receiving Slave Data**:
   - Listens for slave transmissions using interrupt-based RX
   - Receives packets in format: `"NODE_ID + 4-digit counter"`
   - Example: `"20005"` = Node 2, Counter 5

4. **Data Logging**:
   - Logs each received packet to SPIFFS as NDJSON (newline-delimited JSON)
   - Format: `{"node":2,"counter":5,"raw":"20005","frame":42}`
   - One JSON object per line

5. **Data Upload**:
   - Every 60 seconds, reads all logged data from SPIFFS
   - Converts NDJSON to JSON array format
   - POSTs to configured webhook URL via HTTP
   - Deletes local file on successful upload
   - Retries on failure (keeps file for next attempt)

### Slave Node Operation

1. **Initial Boot**:
   - Initializes LoRa radio
   - Waits for first "FRAME|" message from master
   - Records frame start time and enters synced state

2. **Frame Synchronization**:
   - Listens for "FRAME|" messages continuously
   - Updates frame start time on each FRAME reception
   - Maintains synchronization with master

3. **Staggered Transmission**:
   - Calculates transmission time based on node ID:
     - Node 2: `frameStart + 200ms + 0 * 1000ms`
     - Node 3: `frameStart + 200ms + 1 * 1000ms`
     - Node 4: `frameStart + 200ms + 2 * 1000ms`
   - Transmits within 200ms window to tolerate jitter
   - Prevents multiple transmissions in same frame

4. **Packet Format**:
   - Builds message: `NODE_ID (1 digit) + Counter (4 digits)`
   - Example: Node 3, Counter 12 → `"30012"`
   - Counter persists across deep sleep using RTC memory

5. **Deep Sleep**:
   - After transmission, enters deep sleep for 10 seconds
   - Wakes up before next frame to resynchronize
   - Counter value preserved across sleep cycles

### TDMA Timing Diagram

```
Time (seconds)
0    1    2    3    4    5    6    7    8    9    10
|----|----|----|----|----|----|----|----|----|----|
M: FRAME|1
    |
    +--200ms--> S2: 20005
    |
    +--1200ms--> S3: 30012
    |
    +--2200ms--> S4: 40023
    |
    (slaves sleep until next frame)
    
M: FRAME|2
    |
    +--200ms--> S2: 20006
    ...
```

## 📁 Project Structure

```
LoRa-RxTx/
├── master/
│   └── master.ino         # Master node code (with WiFi upload)
├── slave/
│   └── slave.ino          # Slave node code (TDMA transmission)
├── LoRa_rx_tx/
│   └── LoRa_rx_tx.ino     # Legacy code (may be deprecated)
├── .gitignore             # Git ignore file
└── README.md              # This file
```

## 🚀 Usage

### Setting Up Master Node

1. Open `master/master.ino` in Arduino IDE
2. Configure WiFi credentials:
   ```cpp
   const char* WIFI_SSID     = "YourWiFiSSID";
   const char* WIFI_PASSWORD = "YourWiFiPassword";
   ```
3. Configure webhook URL:
   ```cpp
   const char* SERVER_URL = "https://your-webhook-url.com/endpoint";
   ```
4. Upload to Node ID 1 board
5. Monitor Serial output (115200 baud) for status

### Setting Up Slave Nodes

1. Open `slave/slave.ino` in Arduino IDE
2. Set `NODE_ID` for each slave (2, 3, 4, etc.)
3. Ensure LoRa parameters match master exactly
4. Upload to respective boards
5. Monitor Serial output (115200 baud) for synchronization status

### Network Operation

1. Power on master node first (it will start broadcasting FRAME messages)
2. Power on slave nodes (they will sync to first FRAME received)
3. Slaves will transmit in their assigned time slots
4. Master logs all received data and uploads to webhook every 60 seconds

## 📡 Network Protocol

### Frame Synchronization

- **Master Packet**: `"FRAME|" + frameCounter`
- **Frequency**: Every 10 seconds
- **Purpose**: Synchronize all slaves to common time reference

### Slave Transmission

- **Packet Format**: `NODE_ID (1 digit) + Counter (4 digits)`
- **Examples**: 
  - `"20005"` = Node 2, Counter 5
  - `"30012"` = Node 3, Counter 12
- **Timing**: Staggered 1 second apart to prevent collisions

### Data Logging Format

Master logs data as NDJSON (newline-delimited JSON):
```json
{"node":2,"counter":5,"raw":"20005","frame":42}
{"node":3,"counter":12,"raw":"30012","frame":42}
{"node":2,"counter":6,"raw":"20006","frame":43}
```

Upload format (JSON array):
```json
[
{"node":2,"counter":5,"raw":"20005","frame":42},
{"node":3,"counter":12,"raw":"30012","frame":42},
{"node":2,"counter":6,"raw":"20006","frame":43}
]
```

## 📶 LoRa Parameters

Default configuration:
- **Frequency**: 916.3 MHz (adjust for your region's ISM band)
- **Bandwidth**: 250.0 kHz (wider bandwidth for faster transmission)
- **Spreading Factor**: 9 (faster, less range than SF11)
- **TX Power**: 14 dBm (adjustable for range/power tradeoff)

**Critical**: All nodes (master and slaves) must use identical LoRa parameters for proper communication.

## 🔋 Power Management

### Master Node
- **Always Active**: Master stays awake to receive slave transmissions
- **WiFi Power**: WiFi consumes additional power when connected
- **SPIFFS**: File system operations have minimal power impact

### Slave Nodes
- **Deep Sleep**: Slaves sleep for 10 seconds after each transmission
- **Wake Timing**: Wake up before next frame to resynchronize
- **RTC Memory**: Counter persists across deep sleep cycles
- **Power Efficiency**: Deep sleep significantly reduces power consumption

**Estimated Battery Life**:
- **Master**: Several days to weeks (depends on WiFi usage and power supply)
- **Slaves**: Several weeks to months (with 10-second sleep cycles)

## 🐛 Troubleshooting

### Slaves Not Synchronizing

- Verify master is broadcasting FRAME messages (check Serial output)
- Ensure all nodes use identical LoRa parameters
- Check that slaves are within range of master
- Verify antenna connections

### Collision Issues

- Ensure each slave has unique NODE_ID (2, 3, 4, etc.)
- Check that `PER_NODE_DELAY_MS` is sufficient (default 1000ms)
- Verify slaves are receiving FRAME messages for synchronization

### Master Not Receiving Slave Data

- Check Serial output for "RX SLAVE" messages
- Verify slaves are transmitting at correct time offsets
- Ensure LoRa parameters match exactly
- Check for interference or range issues

### WiFi Upload Failures

- Verify WiFi credentials are correct
- Check that webhook URL is accessible
- Monitor Serial output for HTTP status codes
- Data is retained in SPIFFS for retry if upload fails

### SPIFFS Issues

- SPIFFS is automatically formatted on first use
- Check Serial output for "SPIFFS mount" messages
- If file system fails, master will continue operation without logging

### Deep Sleep Issues (Slaves)

- Verify `SLEEP_SECONDS` matches frame period (10 seconds)
- Check that RTC memory is preserving counter (check Serial on wake)
- Ensure GPIO 0 is not pulled low (prevents wake)

## 🔧 Advanced Configuration

### Adjusting Frame Period

Modify in both master and slave code:
```cpp
#define FRAME_PERIOD_MS 15000  // 15 seconds between frames
```

Also update slave sleep duration:
```cpp
#define SLEEP_SECONDS 15  // Match frame period
```

### Changing TDMA Timing

Modify slave transmission offsets:
```cpp
#define BASE_OFFSET_MS      300      // Delay after FRAME
#define PER_NODE_DELAY_MS   1500     // 1.5 seconds between nodes
```

### Adjusting Upload Interval

Modify master upload frequency:
```cpp
const unsigned long UPLOAD_INTERVAL_MS = 30000;   // Upload every 30 seconds
```

### Changing LoRa Parameters

Modify for your needs (must match across all nodes):
```cpp
#define SPREADING_FACTOR 11  // Higher = more range, slower
#define TXPOWER 22          // Higher = more range, more power
#define BANDWIDTH 125.0     // Lower = less data rate, more range
```

## 📝 License

This project is open source. Feel free to use, modify, and distribute as needed.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📧 Contact

For questions or issues, please open an issue on GitHub.

---

**Note**: 
- Ensure all nodes use identical LoRa parameters for proper communication
- The frequency must be legal for use in your region
- Master node requires WiFi connectivity for data upload functionality
- TDMA timing requires accurate frame synchronization - ensure slaves receive FRAME messages reliably
