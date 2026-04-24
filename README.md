# COM3505: ESP32 IoT Environmental Monitor & LED Controller

## Overview
This project implements an interactive Internet of Things (IoT) system powered by an ESP32-S3-mini-1 microcontroller. Users can interact with a responsive web interface hosted on a Python Flask server to manually control a 6-LED array or trigger one of four dynamic animation patterns (Blink, Chase, Rainbow, Fire). Additionally, the dashboard features an automatically updating chart displaying live temperature readings from a physical TMP36 sensor.

## Prerequisites
Before running this project, ensure you have the following installed:
* **Python 3.x** * **VS Code** with the **PlatformIO** extension installed

## Installation
1. Open your terminal and navigate to your desired directory.
2. Clone the repository:
   ```bash
   git clone git@github.com:jLupu1/com3505-assignment.git
   cd com3505-assignment
   ```

# Running Instructions
## 1. Setting up the Flask Server
Navigate to the software directory:

```Bash
cd software
```

Open app.py. Scroll to the bottom of the file and ensure your desired port is set:

```Python
app.run(host='0.0.0.0', port=6767) 
```
Start the server:

```Bash
python app.py
```

Important: Note the local network IP address output by Flask in the terminal (e.g., 192.168.1.X). Do not use 127.0.0.1 or localhost, as the ESP32 needs the actual network IP to connect!

## 2. Starting the Hardware (ESP32)
Plug your ESP32-S3-mini-1 into your computer via USB.
Open VS Code and use PlatformIO's "Pick a folder" feature to open the `/hardware` directory.

Open the main.cpp file and configure your local Wi-Fi credentials:

```C++
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
```
In the `setup()` function, update the WebSocket initialization with the IP address and Port from Step 1:

```C++
// Replace "IP_ADDRESS" and PORT with your Flask server details
webSocket.begin("192.168.1.X", 6767, "/hardware");
```
Click the Upload arrow in the PlatformIO toolbar to flash the code to the ESP32.
Open the PlatformIO Serial Monitor to verify the Wi-Fi connection and sensor outputs.
