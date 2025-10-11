**Real-Time Room Environment Monitor (ESP32 AP & Client)**

This project implements a distributed environmental monitoring system using two ESP32 microcontrollers. One ESP32 acts as a Server and Access Point (AP), while the second acts as a Client, connecting to the AP to report its sensor readings. The Server aggregates data, handles system alarms, manages persistent configuration thresholds, and provides a web-based dashboard for control and viewing.

**System Architecture**
The system consists of two main components running dedicated code:

_Server ESP32 (Room 1):_

    Creates a Wi-Fi Access Point (RoomMonitor).
    Reads its local sensors (Temp, Hum, Air Quality).
    Hosts a Web UI for monitoring and setting thresholds.
    Receives and processes data from the Client ESP32.
    Controls its local LCD and buzzer for real-time status and alarming.

_Client ESP32 (Room 2):_

    Connects to the Server's AP (RoomMonitor).
    Reads its local sensors (Temp, Hum, Air Quality).
    POSTs its data (JSON payload) to the Server every 5 seconds.
    Receives configuration (like the Air Quality alarm threshold) and messages from the Server in the response.
    Displays local sensor data and scrolling messages on its LCD.

**Hardware Requirements**
The project requires several hardware components, with two of each main item for the dual-room setup (Server and Client):

    2 ESP32 Dev Boards (one for the Server, one for the Client).
    2 DHT11 Sensors for measuring Temperature and Humidity.
    2 MQ-135 Sensors for monitoring Air Quality.
    2 I2C 16x2 LCDs for displaying status and messages.
    2 Buzzers for alarm and status feedback.

**Pin Assignments (Check both code files)**
The hardware components share the following specific pin assignments across both the Server and Client boards:

_DHT Data Pin: D4_

_MQ-135 Analog Pin: D34_

_Buzzer Pin: D13_

_I2C (SDA/SCL): Standard ESP32 I2C Pins (usually D21/D22)._

**Software and Dependencies**
The project uses the following standard Arduino libraries. Ensure they are installed in your Arduino IDE:

    WiFi (Built-in)
    WebServer (Built-in)
    Wire (Built-in for I2C)
    LiquidCrystal_I2C (Install via Library Manager)
    DHT (Install via Library Manager)
    ArduinoJson (Version 6 or later recommended - Install via Library Manager)
    Preferences (Built-in - Server only for saving thresholds)
    HTTPClient (Built-in - Client only for communication)

**Setup and Configuration**
1. Wi-Fi Credentials
For the system to communicate, the following Wi-Fi and access credentials must be set correctly in both the Server and Client code:

      a.AP/Client SSID: "RoomMonitor"
   
      b.AP/Client Password: "12345678"
   
      c.Server Web Password (for dashboard login): "status@V!t"
   
      d.Server IP: "192.168.4.1" (This is the Server's Access Point IP address).

**Note: You must upload the client.ino code to the first ESP32 and the ESP32_Server_Final.ino (the provided server code) to the second ESP32.**

2. Uploading the Code
   
     a.Open the Server code (ESP32_Server_Final.ino) in the Arduino IDE.
   
     b.Select the correct ESP32 board and COM port.
   
     c.Upload the code.
   
     d.Open the Client code (client.ino) in the Arduino IDE.
   
     e.Select the correct ESP32 board and COM port.
   
     f.Upload the code.

**Operation**

Server Initialization

    After booting, the Server ESP32 will create the RoomMonitor Wi-Fi network.
    
    The Server's LCD will display Room 1 sensor data on Line 1.

Client Connection

    The Client ESP32 will automatically connect to the RoomMonitor network.
    
    The Client's LCD will display Room 2 sensor data on Line 1.

Web Interface Usage

    Connect your PC or phone to the RoomMonitor Wi-Fi network.
    
    Open a web browser and navigate to the Server's IP address: http://192.168.4.1/
    
    Log in using the configured web password: status@V!t

Key Web UI Controls:

    The web interface provides the following key controls and monitoring features:
    
    a.Room 1 (Server): Displays live sensor data and the current alarm status for the server's local environment.
    
    b.Room 2 (Client): Displays live sensor data and the current alarm status reported by the remote client.
    
    c.Message: Allows you to send a scrolling text message to the LCD of Room 1, Room 2, or both simultaneously.
    
    d.Thresholds: Used to adjust the Temperature, Humidity, and Air Quality values that trigger an alarm for both rooms. These updated settings are persistent and saved to the ESP32's flash memory.

Alarming

    An alarm state is triggered if Temperature, Humidity, or Air Quality in either room exceeds the configured threshold.
    
    LCD Alert: Both Server and Client LCDs will show !! EVACUATE - ALERT on Line 2.

Buzzer: The Server's buzzer will sound continuously.

Status Beep: If no alarm is active, the Server's buzzer will produce a single short beep every 5 seconds, confirming a successful data update.
