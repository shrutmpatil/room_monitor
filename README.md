# Real-Time Room Environment Monitor

### Distributed ESP32 Sensor Network & Centralized Dashboard

## Project Overview

This project implements a robust, distributed environmental monitoring framework using a dual-node ESP32 architecture. It features a **Server Node** (Access Point) acting as the central controller and a **Client Node** acting as a remote telemetry station.

The system aggregates real-time data (Temperature, Humidity, Air Quality) from multiple physical locations, hosting a local Web Dashboard for visualization, remote messaging, and dynamic alarm configuration.

## Key Features

* **Dual-Node Architecture:** Distinguishes between a central Server and a remote Client for covered area expansion.
* **Centralized Web Dashboard:** A responsive HTML interface hosted on the ESP32 for monitoring both nodes simultaneously.
* **Bi-Directional Communication:** The Client sends sensor data via HTTP POST, while the Server responds with alarm configurations and text messages.
* **Persistent Configuration:** Alarm thresholds are saved in Non-Volatile Memory (Preferences), ensuring settings remain after power cycles.
* **Real-Time Alarms:** Immediate visual and audible feedback triggers when environmental metrics exceed defined safety limits.

---

## System Architecture

The firmware is divided into two distinct roles running on identical hardware configurations.

### 1. Server Node (Room 1)
* **Network Host:** Establishes the `RoomMonitor` Wi-Fi Access Point.
* **Aggregator:** Collects local sensor data and processes incoming JSON payloads from the Client.
* **Interface:** Hosts the Web UI.
* **Feedback:** Controls the primary alarm buzzer.

### 2. Client Node (Room 2)
* **Remote Station:** Automatically bridges to the Server's AP.
* **Telemetry:** Captures local metrics and transmits data every 5 seconds.
* **Display:** Renders global messages received from the Server.

---

## Hardware Configuration

### Component Bill of Materials
* 2x ESP32 Development Boards
* 2x DHT11 Sensors (Temperature & Humidity)
* 2x MQ-135 Sensors (Air Quality)
* 2x I2C LCD Displays (16x2)
* 2x Active Buzzers

### Pin Map
The wiring is standardized across both Server and Client nodes.

| Component | Interface | ESP32 GPIO |
| :--- | :--- | :--- |
| **DHT11** | Digital Data | `D4` |
| **MQ-135** | Analog Input | `D34` |
| **Buzzer** | Digital I/O | `D13` |
| **I2C LCD** | SDA | `D21` |
| **I2C LCD** | SCL | `D22` |

---

## Software Dependencies

Ensure the following libraries are installed in your Arduino IDE environment:

* **WiFi** (Built-in)
* **WebServer** (Built-in)
* **HTTPClient** (Built-in)
* **Wire** (Built-in)
* **Preferences** (Built-in)
* **LiquidCrystal_I2C**
* **DHT sensor library**
* **ArduinoJson** (Version 6.x or higher)

---

## Setup & Deployment

### 1. Network Credentials
The following credentials are hardcoded into the firmware for node-to-node handshakes.

* **SSID:** `RoomMonitor`
* **Password:** `12345678`

### 2. Uploading Firmware

**Server Node:**
1.  Open `ESP32_Server_Final.ino`.
2.  Connect the primary ESP32.
3.  Select Board/Port and Upload.

**Client Node:**
1.  Open `client.ino`.
2.  Connect the secondary ESP32.
3.  Select Board/Port and Upload.

---

## Operational Manual

### Accessing the Dashboard
1.  Connect your PC or Mobile device to the Wi-Fi network: **RoomMonitor**.
2.  Navigate to: `http://192.168.4.1`.
3.  Login using the Web Password: `status@V!t`.

### Dashboard Controls
* **Live Data:** View cards for Room 1 (Local) and Room 2 (Remote).
* **Message:** Broadcast scrolling text to specific LCDs.
* **Thresholds:** Update trigger points for alarms. These are saved to flash memory.

### Alarm States
* **Normal Operation:** The Server emits a single confidence beep every 5 seconds (Heartbeat).
* **Critical Alert:** If any sensor exceeds the threshold:
    * LCDs display: `!! EVACUATE - ALERT`
    * Server Buzzer sounds continuously.

---

## License

This project is open-source and available for modification and distribution.
