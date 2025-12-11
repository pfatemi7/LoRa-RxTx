# LoRa Master-Slave Network

A simple, low-power LoRa network implementation with automatic master-slave configuration and failover. This project creates a star network topology where Node ID 1 acts as the master by default, with automatic promotion of other nodes if the master fails.

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

This project implements a simple LoRa-based master-slave network where:
- **Master Node (ID 1)**: Transmits periodic heartbeat packets with counter
- **Slave Nodes (ID 2+)**: Listen for master packets and track missed cycles
- **Automatic Failover**: Slaves automatically promote to master if master fails (10 missed cycles)
- **Low Power**: Deep sleep operation with configurable wake intervals
- **Interrupt-Based RX**: Uses hardware interrupts for efficient packet reception

## ✨ Features

- **Simple Configuration**: Node ID 1 is master by default, others are slaves
- **Automatic Master Promotion**: Slaves become master after 10 missed cycles
- **Interrupt-Based Reception**: Hardware interrupt (DIO1) for efficient packet detection
- **OLED Display**: Real-time status and data display
- **Low Power Operation**: Deep sleep mode with configurable wake intervals (default 5 seconds)
- **RTC Memory**: State persistence across deep sleep cycles
- **Triple Redundancy**: Master transmits heartbeat 3 times for reliability

## 🔧 Hardware Requirements

- **Heltec ESP32 LoRa V3** (or compatible ESP32 with LoRa module)
- **Power Supply**: Battery or USB power (for long-term deployment, use battery)

### Pin Connections

- **LoRa Module**: Integrated on Heltec board
  - DIO1 Pin: Used for RX interrupt callback
- **OLED Display**: Integrated (used for status display)

## 💻 Software Requirements

- **Arduino IDE** (1.8.x or 2.x) or **PlatformIO**
- **ESP32 Board Support** for Arduino IDE
- **Required Libraries**:
  - `heltec_unofficial.h` - Heltec ESP32 LoRa board support
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

- Open `LoRa_rx_tx/LoRa_rx_tx.ino` in Arduino IDE
- Select your board: **Tools → Board → ESP32 Arduino → Heltec ESP32 LoRa V3**
- Select the correct port: **Tools → Port**

## ⚙️ Configuration

Before uploading, configure the following in `LoRa_rx_tx.ino`:

### Node ID
```cpp
#define NODE_ID 1      // 1 = master, others = slaves
```

### LoRa Parameters
```cpp
#define FREQUENCY 916.0        // MHz (adjust for your region)
#define BANDWIDTH 125.0        // kHz
#define SPREADING_FACTOR 9     // 6-12 (lower = faster, less range)
#define TXPOWER 14             // dBm (adjust for range/power consumption)
```

### Timing Parameters
```cpp
#define HEARTBEAT_GAP 150      // ms between master heartbeat bursts
#define HEARTBEAT_COUNT 3      // number of heartbeat bursts
#define LISTEN_WINDOW 6500     // slave listening window (ms) - slaves wait longer
#define FAIL_LIMIT 10          // missed cycles before slave promotes to master
#define SLEEP_MS 5000          // sleep duration between cycles (ms)
```

## 🔄 How It Works

### Initial Boot

1. **Node ID 1**: Automatically becomes master on boot
2. **Other Nodes**: Start as slaves and listen for master packets

### Master Node Operation

1. **Wake from Sleep**:
   - Increments counter
   - Formats packet: `"MASTER|" + counter`
   - Example: `"MASTER|42"`

2. **Transmission**:
   - Displays "MASTER" and payload on OLED
   - Transmits packet **three times** with 150ms delay between bursts
   - Enters deep sleep for 5 seconds (configurable)

### Slave Node Operation

1. **Wake from Sleep**:
   - Shows "SLAVE Listening..." on OLED
   - Starts listening for master packets using interrupt-based RX

2. **Reception**:
   - Listens for up to 6.5 seconds (LISTEN_WINDOW) - extended window for better reliability
   - Uses hardware interrupt (DIO1) to detect incoming packets
   - If packet starts with "MASTER|":
     - Resets miss counter
     - Displays "RX MASTER" and packet on OLED
   - If no packet received:
     - Increments miss counter
     - Displays "NO MASTER" and miss count (format: "Miss=X")

3. **Master Promotion**:
   - If miss count >= 10 (FAIL_LIMIT):
     - Promotes itself to master
     - Displays "PROMOTING BECOME MASTER"
     - Sets `isMaster = true`
     - Resets miss counter

4. **Sleep**:
   - Enters deep sleep for 5 seconds (configurable)

### State Persistence

All state variables use `RTC_DATA_ATTR` to persist across deep sleep:
- `isMaster`: Current role (master or slave)
- `counter`: Master's transmission counter
- `missCount`: Slave's missed packet counter

## 📁 Project Structure

```
LoRa-RxTx/
├── LoRa_rx_tx/
│   └── LoRa_rx_tx.ino    # Main Arduino sketch
├── .gitignore            # Git ignore file
└── README.md             # This file
```

## 🚀 Usage

1. **Configure** Node ID in the code (ID 1 = master, others = slaves)
2. **Upload** the sketch to your Heltec ESP32 LoRa board
3. **Power on** Node ID 1 (it will become master)
4. **Power on** additional nodes with different IDs (they will become slaves)
5. **Monitor** via Serial Monitor (115200 baud) and OLED display

### Serial Monitor Output

Serial output is available for debugging (115200 baud).

### OLED Display States

- **Boot**: "BOOT MASTER" or "BOOT SLAVE"
- **Master**: "MASTER [payload]"
- **Slave Listening**: "SLAVE Listening..."
- **Slave Received**: "RX MASTER [payload]"
- **No Master**: "NO MASTER Miss=[count]"
- **Promotion**: "PROMOTING BECOME MASTER"

## 📡 Network Protocol

### Packet Format

- **Master Packet**: `"MASTER|" + counter`
- **Example**: `"MASTER|42"` = Master, Counter 42

### Master Transmission

- Master transmits each packet **three times** with 150ms delay between bursts
- Provides redundancy for better reliability in noisy environments

### Network Topology

- **Star Topology**: One master, multiple slaves
- **Range**: Typically 1-5 km line-of-sight (depends on environment and LoRa parameters)
- **Scalability**: Multiple slaves can listen to one master
- **Failover**: Automatic master promotion if master fails

## 📶 LoRa Parameters

Default configuration:
- **Frequency**: 916.0 MHz (adjust for your region's ISM band)
- **Bandwidth**: 125.0 kHz
- **Spreading Factor**: 9 (faster, less range than SF11)
- **TX Power**: 14 dBm (adjustable for range/power tradeoff)

**Note**: Ensure LoRa parameters match across all nodes in your network.

## 🔋 Power Management

- **Deep Sleep**: Device sleeps for 5 seconds between cycles (configurable via `SLEEP_MS`)
- **OLED Display**: Powered off during sleep (Vext = HIGH) and during radio initialization
- **Safe Boot Sequence**: OLED and peripherals kept off during radio initialization to prevent interference
- **Radio Reset**: Hardware reset performed during initialization for reliable operation
- **RTC Memory**: State preserved during deep sleep
- **Interrupt-Based RX**: More power-efficient than polling

**Estimated Battery Life**: With a 2000mAh battery and 5-second wake intervals, the device can run for several weeks to months depending on transmission frequency and power settings.

## 🐛 Troubleshooting

### Multiple Masters Appearing

- Ensure only Node ID 1 is configured as master initially
- Check that `NODE_ID` is correctly set in each node's code
- Verify that slaves are receiving master packets (check miss count)

### Slave Not Receiving Packets

- Verify master is transmitting (check OLED display)
- Ensure nodes are within range (typically 1-5 km line-of-sight)
- Check that LoRa parameters match across all nodes
- Verify antenna connections
- Check for interference on the selected frequency
- Increase `LISTEN_WINDOW` if needed

### Slave Promoting Too Quickly

- Increase `FAIL_LIMIT` to allow more missed cycles before promotion
- Check master transmission reliability
- Verify master is actually transmitting

### Master Not Transmitting

- Check OLED display for "MASTER" status
- Verify `NODE_ID == 1` or `isMaster == true`
- Check radio initialization
- Verify power supply is stable

### OLED Display Not Working

- Verify `Vext` pin is set to LOW (powers on display)
- Check display initialization
- Verify display library is properly installed

### Deep Sleep Issues

- Ensure GPIO 0 is not pulled low (prevents wake)
- Check that RTC pins are not disturbed during sleep
- Verify power supply is stable
- Check that wake timer is properly configured

## 🔧 Advanced Configuration

### Changing Sleep Duration

Modify `SLEEP_MS` to change how often nodes wake:
```cpp
#define SLEEP_MS 10000  // Wake every 10 seconds
```

### Adjusting Listen Window

Modify `LISTEN_WINDOW` for better reception (default is 6500ms):
```cpp
#define LISTEN_WINDOW 8000  // Listen for 8 seconds
```

### Changing Failover Threshold

Modify `FAIL_LIMIT` to change when slaves promote (default is 10):
```cpp
#define FAIL_LIMIT 15  // Promote after 15 missed cycles
```

### Adjusting Heartbeat Gap

Modify `HEARTBEAT_GAP` to change timing between bursts (default is 150ms):
```cpp
#define HEARTBEAT_GAP 200  // 200ms between bursts
```

### Adjusting LoRa Parameters

Modify for your needs:
```cpp
#define SPREADING_FACTOR 11  // Higher = more range, slower
#define TXPOWER 22          // Higher = more range, more power
```

### Changing Frequency

Modify for your region:
```cpp
#define FREQUENCY 868.0  // European ISM band
#define FREQUENCY 915.0  // North American ISM band
```

## 📝 License

This project is open source. Feel free to use, modify, and distribute as needed.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📧 Contact

For questions or issues, please open an issue on GitHub.

---

**Note**: Ensure all nodes use the same LoRa parameters (frequency, bandwidth, spreading factor, etc.) for proper communication. The frequency must be legal for use in your region. Node ID 1 is the default master; other nodes will automatically promote if the master fails.
