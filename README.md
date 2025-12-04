# LoRa Master-Slave Network

A low-power LoRa network implementation with automatic master-slave discovery. This project creates a simple star network topology where one node acts as the master (transmitter) and other nodes act as slaves (receivers), with automatic role assignment and master recovery.

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

This project implements a LoRa-based master-slave network where:
- **Master Node**: Transmits periodic counter packets to synchronize the network
- **Slave Nodes**: Listen for master packets and display received data
- **Auto-Discovery**: Nodes automatically determine their role on first boot
- **Master Recovery**: Slaves automatically re-enter discovery mode if master is lost
- **Low Power**: Deep sleep operation with configurable wake intervals

## ✨ Features

- **Automatic Master-Slave Discovery**: Nodes automatically determine their role
- **Master Node**: Transmits counter packets with triple redundancy for reliability
- **Slave Nodes**: Listen and display received master packets
- **Master Recovery**: Automatic re-discovery if master connection is lost
- **OLED Display**: Real-time status and data display
- **Low Power Operation**: Deep sleep mode with 30-second wake intervals (configurable)
- **RTC Memory**: State persistence across deep sleep cycles
- **Hardware Reset**: Automatic SX1262 radio module reset on boot

## 🔧 Hardware Requirements

- **Heltec ESP32 LoRa V3** (or compatible ESP32 with LoRa module)
- **Power Supply**: Battery or USB power (for long-term deployment, use battery)

### Pin Connections

- **LoRa Module**: Integrated on Heltec board
  - CS Pin: GPIO 8
  - RST Pin: GPIO 12
  - BUSY Pin: GPIO 13
  - DIO1 Pin: GPIO 14
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
RTC_DATA_ATTR uint32_t NODE_ID = 1;  // Unique ID for each node (0-9)
```

### Timing Parameters
```cpp
#define PERIOD_MS 30000        // Wake interval in milliseconds (30 seconds)
#define LISTEN_MS 2000         // Discovery listen window (2 seconds)
#define NORMAL_LISTEN_MS 1800  // Normal slave listen window (1.8 seconds)
```

### LoRa Parameters
```cpp
#define FREQ 916.0  // Frequency in MHz (adjust for your region)
```

The following LoRa parameters are hardcoded:
- **Bandwidth**: 125.0 kHz
- **Spreading Factor**: 11
- **Coding Rate**: 5
- **CRC**: Enabled

## 🔄 How It Works

### Initial Boot (Discovery Mode)

1. **First Boot**: When a node boots for the first time (`paired == false`):
   - Enters discovery mode
   - Powers on OLED display
   - Shows "DISCOVERY LISTENING..."
   - Listens for 2 seconds for incoming LoRa packets
   - If a valid 5-character packet is received:
     - Node becomes **SLAVE**
     - Shows "SLAVE MODE PAIRED"
     - Saves state to RTC memory
     - Enters deep sleep
   - If no packet is received:
     - Node becomes **MASTER**
     - Shows "MASTER MODE ACTIVE"
     - Saves state to RTC memory

### Master Node Operation

1. **Wake from Sleep**:
   - Increments counter (wraps at 9999)
   - Formats packet: `NODE_ID (1 digit) + Counter (4 digits)`
   - Example: Node 1, counter 42 → `"10042"`

2. **Transmission**:
   - Displays "MASTER TX" and packet on OLED
   - Transmits packet **three times** with 120ms delay between transmissions
   - Logs transmission to Serial Monitor
   - Enters deep sleep for 30 seconds

### Slave Node Operation

1. **Wake from Sleep**:
   - Shows "SLAVE MODE LISTENING" on OLED
   - Starts listening for master packets

2. **Reception**:
   - Listens for 1.8 seconds
   - If valid 5-character packet received:
     - Resets miss counter
     - Displays "SLAVE RX" and packet on OLED
     - Enters deep sleep
   - If no packet received:
     - Increments miss counter
     - If miss count >= 1:
       - Shows "MASTER LOST RE-DISCOVERING"
       - Performs ESP restart to re-enter discovery mode
     - If miss count < 1:
       - Shows "NO DATA ONE MISS" (tolerates one missed packet)
       - Enters deep sleep

### State Persistence

All state variables use `RTC_DATA_ATTR` to persist across deep sleep:
- `paired`: Whether node has completed discovery
- `isMaster`: Current role (master or slave)
- `counter`: Master's transmission counter
- `missCount`: Slave's missed packet counter
- `NODE_ID`: Unique node identifier

## 📁 Project Structure

```
LoRa-RxTx/
├── LoRa_rx_tx/
│   └── LoRa_rx_tx.ino    # Main Arduino sketch
├── .gitignore            # Git ignore file
└── README.md             # This file
```

## 🚀 Usage

1. **Configure** Node ID and timing parameters in the code
2. **Upload** the sketch to your Heltec ESP32 LoRa board
3. **Power on** the first node (it will become master)
4. **Power on** additional nodes (they will become slaves)
5. **Monitor** via Serial Monitor (115200 baud) and OLED display

### Serial Monitor Output

The master node will output transmission logs:
```
TX: 10042
TX: 10043
...
```

### OLED Display States

- **Discovery**: "DISCOVERY LISTENING..."
- **Master**: "MASTER MODE ACTIVE" / "MASTER TX [packet]"
- **Slave Paired**: "SLAVE MODE PAIRED"
- **Slave Listening**: "SLAVE MODE LISTENING"
- **Slave Received**: "SLAVE RX [packet]"
- **Master Lost**: "MASTER LOST RE-DISCOVERING"
- **No Data**: "NO DATA ONE MISS"

## 📡 Network Protocol

### Packet Format

- **Length**: 5 characters
- **Format**: `NODE_ID (1 digit) + Counter (4 digits)`
- **Example**: `"10042"` = Node 1, Counter 42

### Master Transmission

- Master transmits each packet **three times** with 120ms delay
- Provides redundancy for better reliability in noisy environments

### Network Topology

- **Star Topology**: One master, multiple slaves
- **Range**: Typically 1-5 km line-of-sight (depends on environment)
- **Scalability**: Multiple slaves can listen to one master

## 📶 LoRa Parameters

- **Frequency**: 916.0 MHz (adjust for your region's ISM band)
- **Bandwidth**: 125 kHz
- **Spreading Factor**: 11
- **Coding Rate**: 5
- **CRC**: Enabled

**Note**: Ensure LoRa parameters match across all nodes in your network.

## 🔋 Power Management

- **Deep Sleep**: Device sleeps for 30 seconds between operations (configurable)
- **OLED Display**: Powered on during operation, off during sleep
- **Radio Sleep**: Radio module enters sleep mode between operations
- **RTC Memory**: State preserved during deep sleep

**Estimated Battery Life**: With a 2000mAh battery and 30-second wake intervals, the device can run for several weeks to months depending on transmission frequency.

## 🐛 Troubleshooting

### Node Stuck in Discovery Mode
- Ensure at least one node is powered on and functioning
- Check that LoRa parameters match across all nodes
- Verify antenna connections
- Check Serial Monitor for error messages

### Slave Not Receiving Packets
- Verify master is transmitting (check Serial Monitor)
- Ensure nodes are within range (typically 1-5 km line-of-sight)
- Check that LoRa frequency is legal in your region
- Verify antenna connections on both nodes
- Check for interference on the selected frequency

### Master Not Transmitting
- Check Serial Monitor for transmission logs
- Verify OLED display shows "MASTER TX" messages
- Check radio initialization (hardware reset should occur on boot)
- Verify power supply is stable

### Frequent Master Loss (Slaves Re-discovering)
- Master may be out of range or powered off
- Check master node power supply
- Verify master is actually transmitting (Serial Monitor)
- Check for interference or obstacles between nodes
- Consider increasing `NORMAL_LISTEN_MS` for better reception

### OLED Display Not Working
- Verify `Vext` pin is set to LOW (powers on display)
- Check display initialization in `heltec_setup()`
- Verify display library is properly installed

### Deep Sleep Issues
- Ensure GPIO 0 is not pulled low (prevents wake)
- Check that RTC pins are not disturbed during sleep
- Verify power supply is stable
- Check that wake timer is properly configured

## 🔧 Advanced Configuration

### Changing Wake Interval

Modify `PERIOD_MS` to change how often nodes wake:
```cpp
#define PERIOD_MS 60000  // Wake every 60 seconds
```

### Adjusting Listen Windows

Modify discovery and normal listen windows:
```cpp
#define LISTEN_MS 3000         // Longer discovery window
#define NORMAL_LISTEN_MS 2500  // Longer slave listen window
```

### Changing Frequency

Modify `FREQ` for your region:
```cpp
#define FREQ 868.0  // European ISM band
#define FREQ 915.0  // North American ISM band
```

## 📝 License

This project is open source. Feel free to use, modify, and distribute as needed.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📧 Contact

For questions or issues, please open an issue on GitHub.

---

**Note**: Ensure all nodes use the same LoRa parameters (frequency, bandwidth, spreading factor, etc.) for proper communication. The frequency must be legal for use in your region.
