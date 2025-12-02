# Real-Time Room Environment Monitor

<div align="center">

### Distributed ESP32 Sensor Network & Centralized Dashboard

![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)
![IoT](https://img.shields.io/badge/IoT-Enabled-brightgreen?style=for-the-badge)

</div>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [System Architecture](#-system-architecture)
- [Hardware Configuration](#-hardware-configuration)
- [Software Dependencies](#-software-dependencies)
- [Setup & Deployment](#-setup--deployment)
- [Operational Manual](#-operational-manual)
- [Collaborators](#-collaborators)
- [License](#-license)

---

## Overview

This project implements a **robust, distributed environmental monitoring framework** using a dual-node ESP32 architecture. It features a **Server Node** (Access Point) acting as the central controller and a **Client Node** acting as a remote telemetry station.

The system aggregates real-time data (Temperature, Humidity, Air Quality) from multiple physical locations, hosting a local Web Dashboard for visualization, remote messaging, and dynamic alarm configuration.

---

## Key Features

<table>
<tr>
<td width="50%">

### Dual-Node Architecture
Distinguishes between a central Server and a remote Client for covered area expansion.

### Centralized Web Dashboard
A responsive HTML interface hosted on the ESP32 for monitoring both nodes simultaneously.

### Bi-Directional Communication
The Client sends sensor data via HTTP POST, while the Server responds with alarm configurations and text messages.

</td>
<td width="50%">

### Persistent Configuration
Alarm thresholds are saved in Non-Volatile Memory (Preferences), ensuring settings remain after power cycles.

### Real-Time Alarms
Immediate visual and audible feedback triggers when environmental metrics exceed defined safety limits.

### Wireless Monitoring
Monitor environmental conditions from anywhere within the Wi-Fi network range.

</td>
</tr>
</table>

---

## System Architecture

The firmware is divided into two distinct roles running on identical hardware configurations.

### Server Node (Room 1)

```
┌─────────────────────────────────────┐
│   Server Node Functions             │
├─────────────────────────────────────┤
│ • Network Host (Access Point)       │
│ • Data Aggregator                   │
│ • Web Interface Host                │
│ • Primary Alarm Controller          │
└─────────────────────────────────────┘
```

- **Network Host:** Establishes the `RoomMonitor` Wi-Fi Access Point
- **Aggregator:** Collects local sensor data and processes incoming JSON payloads from the Client
- **Interface:** Hosts the Web UI at `192.168.4.1`
- **Feedback:** Controls the primary alarm buzzer

### Client Node (Room 2)

```
┌─────────────────────────────────────┐
│    Client Node Functions            │
├─────────────────────────────────────┤
│ • Remote Telemetry Station          │
│ • Automatic Connection              │
│ • Data Transmission (5s interval)   │
│ • Message Display                   │
└─────────────────────────────────────┘
```

- **Remote Station:** Automatically bridges to the Server's AP
- **Telemetry:** Captures local metrics and transmits data every 5 seconds
- **Display:** Renders global messages received from the Server

---

## Hardware Configuration

### Component Bill of Materials

| Quantity | Component | Purpose |
|:--------:|:----------|:--------|
| 2x | **ESP32 Development Boards** | Main microcontroller |
| 2x | **DHT11 Sensors** | Temperature & Humidity measurement |
| 2x | **MQ-135 Sensors** | Air Quality detection |
| 2x | **I2C LCD Displays (16x2)** | Local data visualization |
| 2x | **Active Buzzers** | Audio alarm feedback |

### Pin Map

The wiring is **standardized** across both Server and Client nodes.

| Component | Interface | ESP32 GPIO | Description |
|:----------|:----------|:-----------|:------------|
| **DHT11** | Digital Data | `GPIO 4` | Temperature/Humidity sensor |
| **MQ-135** | Analog Input | `GPIO 34` | Air quality sensor |
| **Buzzer** | Digital I/O | `GPIO 13` | Alarm buzzer |
| **I2C LCD (SDA)** | I2C Data | `GPIO 21` | LCD data line |
| **I2C LCD (SCL)** | I2C Clock | `GPIO 22` | LCD clock line |

### Wiring Diagram

```
ESP32 Node Configuration:
┌──────────────────┐
│                  │
│   DHT11 ──────── GPIO 4
│   MQ-135 ─────── GPIO 34
│   Buzzer ─────── GPIO 13
│   LCD SDA ────── GPIO 21
│   LCD SCL ────── GPIO 22
│                  │
└──────────────────┘
```

---

## Software Dependencies

Ensure the following libraries are installed in your Arduino IDE environment:

### Built-in Libraries
-  **WiFi** - Wireless networking
-  **WebServer** - HTTP server functionality
-  **HTTPClient** - HTTP client requests
-  **Wire** - I2C communication
-  **Preferences** - Non-volatile storage

### External Libraries (Install via Library Manager)
-  **LiquidCrystal_I2C** - LCD display control
-  **DHT sensor library** - DHT11 sensor interface
-  **ArduinoJson** (Version 6.x or higher) - JSON parsing

---

## 1. Setup & Deployment

###  Network Credentials

The following credentials are hardcoded into the firmware for node-to-node handshakes:

```
 SSID: RoomMonitor
 Password: 12345678
 Server IP: 192.168.4.1
```

### 2. Uploading Firmware

#### **Server Node:**
1. Open `ESP32_Server_Final.ino` in Arduino IDE
2. Connect the primary ESP32 via USB
3. Select **Board:** `ESP32 Dev Module`
4. Select the correct **Port**
5. Click **Upload** 

#### **Client Node:**
1. Open `client.ino` in Arduino IDE
2. Connect the secondary ESP32 via USB
3. Select **Board:** `ESP32 Dev Module`
4. Select the correct **Port**
5. Click **Upload** 

### 3. Power-Up Sequence

1. Power on the **Server Node** first
2. Wait 10 seconds for AP initialization
3. Power on the **Client Node**
4. Verify connection on LCD displays

---

## Operational Manual

### Accessing the Dashboard

1. Connect your PC or Mobile device to the Wi-Fi network: **`RoomMonitor`**
2. Open a web browser and navigate to: **`http://192.168.4.1`**
3. Login using the Web Password: **`status@V!t`**

### Dashboard Controls

<table>
<tr>
<td width="33%">

#### Live Data
View real-time sensor readings for:
- **Room 1** (Server - Local)
- **Room 2** (Client - Remote)

</td>
<td width="33%">

#### Message Broadcast
Send scrolling text messages to:
- Room 1 LCD
- Room 2 LCD
- Both displays

</td>
<td width="33%">

#### Threshold Configuration
Update alarm trigger points:
- Temperature limits
- Humidity limits
- Air quality limits

*Saved to flash memory*

</td>
</tr>
</table>

### Alarm States

#### Normal Operation
- Server emits a **single confidence beep** every 5 seconds (Heartbeat)
- LCDs display current sensor readings
- Dashboard shows green status indicators

#### Critical Alert
When any sensor exceeds the configured threshold:
- **LCDs display:** `!! EVACUATE - ALERT`
- **Server Buzzer:** Sounds continuously
- **Dashboard:** Shows red warning indicators

---

## 👥 Collaborators

<div align="center">

### Development Team

<table>
<tr>
<td align="center">
<a href="https://github.com/shrutmpatil">
<img src="https://img.shields.io/badge/Shrut-Patil-blue?style=for-the-badge&logo=github" alt="Shrut Patil"/>
</a>
<br/>
<a href="https://www.linkedin.com/in/shrutmpatil/">
<img src="https://img.icons8.com/color/48/000000/linkedin.png" width="40"/>
</a>
</td>
<td align="center">
<a href="https://github.com/siddhilad920">
<img src="https://img.shields.io/badge/Siddhi-Lad-lightgrey?style=for-the-badge&logo=github" alt="Siddhi Lad"/>
</a>
<br/>
<a href="https://www.linkedin.com/in/lad-siddhi/">
<img src="https://img.icons8.com/color/48/000000/linkedin.png" width="40"/>
</a>
</td>
<td align="center">
<a href="https://github.com/shrutmpatil">
<img src="https://img.shields.io/badge/Maitry-Mohite-orange?style=for-the-badge&logo=github" alt="Maitry Mohite"/>
</a>
<br/>
<a href="https://www.linkedin.com/in/maitry-mohite-2560422b4/">
<img src="https://img.icons8.com/color/48/000000/linkedin.png" width="40"/>
</a>
</td>
<td align="center">
<a href="https://github.com/shrutmpatil">
<img src="https://img.shields.io/badge/Tanvi-Yerram-pink?style=for-the-badge&logo=github" alt="Tanvi Yerram"/>
</a>
<br/>
<a href="https://www.linkedin.com/in/tanvi-yerram-552789304/">
<img src="https://img.icons8.com/color/48/000000/linkedin.png" width="40"/>
</a>
</td>
</tr>
</table>

</div>

---

## License

This project is **open-source** and available for modification and distribution.


---

<div align="center">

### ⭐ Star this project if you find it useful!


![Visitors](https://visitor-badge.laobi.icu/badge?page_id=room-environment-monitor)

</div>
