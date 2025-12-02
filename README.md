# LoRa Mesh Network with MQTT Integration

A low-power IoT sensor node project using LoRa mesh networking, WiFi, and MQTT for remote environmental monitoring. This project implements a mesh network where nodes can relay messages and transmit temperature/humidity sensor data.

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
- [MQTT Topics](#mqtt-topics)
- [LoRa Parameters](#lora-parameters)
- [Power Management](#power-management)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## 🎯 Overview

This project creates a mesh network of LoRa-enabled sensor nodes that:
- Read temperature and humidity data from DHT11 sensors
- Relay messages between nodes in a mesh network
- Transmit sensor data via LoRa and MQTT
- Operate in deep sleep mode to minimize power consumption (wakes every hour)

## ✨ Features

- **LoRa Mesh Networking**: Nodes can receive and relay packets from other nodes
- **Dual Communication**: Data sent via both LoRa radio and MQTT
- **Low Power Operation**: Deep sleep mode with 1-hour wake intervals
- **Sensor Integration**: DHT11 temperature and humidity sensor
- **OLED Power Management**: OLED display is powered off to save energy
- **Automatic Mesh Relay**: 3-second window for receiving and relaying mesh packets

## 🔧 Hardware Requirements

- **Heltec ESP32 LoRa V3** (or compatible ESP32 with LoRa module)
- **DHT11** temperature and humidity sensor
- **Power Supply**: Battery or USB power (for long-term deployment, use battery)

### Pin Connections

- **DHT11 Data Pin**: GPIO 47
- **LoRa Module**: Integrated on Heltec board
- **OLED Display**: Integrated (powered off in this project)

## 💻 Software Requirements

- **Arduino IDE** (1.8.x or 2.x) or **PlatformIO**
- **ESP32 Board Support** for Arduino IDE
- **Required Libraries**:
  - `heltec_unofficial.h` - Heltec ESP32 LoRa board support
  - `WiFi.h` - ESP32 WiFi library (built-in)
  - `PubSubClient.h` - MQTT client library
  - `DHT.h` - DHT sensor library
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

- **PubSubClient** by Nick O'Leary
- **DHT sensor library** by Adafruit
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

### WiFi Credentials
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### MQTT Settings
```cpp
const char* mqtt_server = "broker.hivemq.com";  // Or your MQTT broker
const int mqtt_port = 1883;
const char* mqtt_out_topic = "parhamMesh/out";
```

### Node ID
```cpp
uint32_t NODE_ID = 2;  // Unique ID for each node (0-9)
```

### LoRa Parameters (if needed)
```cpp
#define FREQUENCY        916.0    // MHz (adjust for your region)
#define BANDWIDTH        125.0    // kHz
#define SPREADING_FACTOR 11       // 6-12
#define TXPOWER          22       // dBm
```

### Deep Sleep Duration
```cpp
esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL); // 1 hour in microseconds
```

## 🔄 How It Works

1. **Initialization**: 
   - Board wakes from deep sleep
   - Initializes DHT11 sensor
   - Powers off OLED display
   - Configures LoRa radio parameters
   - Connects to WiFi and MQTT broker

2. **Mesh Window (3 seconds)**:
   - Listens for incoming LoRa packets
   - Relays valid 5-character packets to other nodes
   - Publishes received packets to MQTT

3. **Sensor Reading**:
   - Reads temperature and humidity from DHT11
   - Formats data as: `NODE_ID + Temperature (2 digits) + Humidity (2 digits)`
   - Example: Node 2, 25°C, 60% → `"22560"`

4. **Data Transmission**:
   - Sends sensor data via LoRa radio
   - Publishes same data to MQTT topic

5. **Power Down**:
   - Disconnects WiFi
   - Puts LoRa radio to sleep
   - Enters deep sleep for 1 hour

## 📁 Project Structure

```
LoRa-RxTx/
├── LoRa_rx_tx/
│   └── LoRa_rx_tx.ino    # Main Arduino sketch
├── .gitignore            # Git ignore file
└── README.md             # This file
```

## 🚀 Usage

1. **Configure** WiFi, MQTT, and Node ID in the code
2. **Upload** the sketch to your Heltec ESP32 LoRa board
3. **Monitor** via Serial Monitor (115200 baud) for debugging
4. **Subscribe** to MQTT topic `parhamMesh/out` to receive sensor data
5. **Deploy** multiple nodes with different NODE_IDs to create a mesh network

### Serial Monitor Output

The device will output initialization messages and sensor readings via Serial Monitor at 115200 baud.

### MQTT Monitoring

Subscribe to the MQTT topic to monitor all sensor data:

```bash
# Using mosquitto_sub
mosquitto_sub -h broker.hivemq.com -t "parhamMesh/out"

# Or use any MQTT client
```

## 📡 MQTT Topics

- **Output Topic**: `parhamMesh/out`
  - Format: `NODE_ID + Temperature + Humidity` (e.g., `"22560"`)
  - Contains both relayed mesh packets and local sensor readings

## 📶 LoRa Parameters

- **Frequency**: 916.0 MHz (adjust for your region's ISM band)
- **Bandwidth**: 125 kHz
- **Spreading Factor**: 11
- **Coding Rate**: 5
- **Preamble Length**: 12
- **TX Power**: 22 dBm
- **CRC**: Enabled

**Note**: Ensure LoRa parameters match across all nodes in your mesh network.

## 🔋 Power Management

- **Deep Sleep**: Device sleeps for 1 hour between readings
- **OLED Off**: Display is powered off to save energy
- **WiFi Disconnect**: WiFi is disconnected before sleep
- **LoRa Sleep**: Radio module enters sleep mode

**Estimated Battery Life**: With a 2000mAh battery and 1-hour wake intervals, the device can run for several months.

## 🐛 Troubleshooting

### Device Not Connecting to WiFi
- Check SSID and password
- Ensure WiFi is 2.4 GHz (ESP32 doesn't support 5 GHz)
- Increase timeout in code if needed

### MQTT Connection Failed
- Verify MQTT broker is accessible
- Check firewall settings
- Try a different MQTT broker (e.g., `test.mosquitto.org`)

### No LoRa Reception
- Verify all nodes use the same LoRa parameters
- Check antenna connections
- Ensure nodes are within range (typically 1-5 km line-of-sight)
- Verify frequency is legal in your region

### Sensor Reading Errors
- Check DHT11 wiring (power, ground, data pin 47)
- Ensure DHT11 has proper pull-up resistor
- Verify sensor is not damaged

### Deep Sleep Issues
- Ensure GPIO 0 is not pulled low (prevents wake)
- Check that RTC pins are not disturbed during sleep
- Verify power supply is stable

## 📝 License

This project is open source. Feel free to use, modify, and distribute as needed.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📧 Contact

For questions or issues, please open an issue on GitHub.

---

**Note**: Remember to change WiFi credentials and MQTT settings before deploying. For production use, consider using environment variables or a configuration file instead of hardcoding credentials.

