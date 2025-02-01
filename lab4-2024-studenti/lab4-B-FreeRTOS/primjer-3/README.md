# GPIO Response Time Measurement System
================================

![srsv-lab4](https://github.com/user-attachments/assets/93238681-3d4f-4ed9-b1b7-d14e9fa763bd)

Description
-----------
This project implements a real-time response measurement system using a Wio Terminal and ESP32-C6. The Wio Terminal board sends GPIO requests at random intervals (interval range configurable) and measures response times from the ESP32-C6, which simulates processing delays with busy waiting. Developed for the Real-time Systems course at Faculty of Electrical Engineering and Computing (FER), University of Zagreb.

Hardware Requirements
-------------------
- Wio Terminal (ATSAMD51)
- ESP32-C6 Development Board
- Connecting wires
- USB cables for programming

Connections
----------
Wio Terminal          ESP32-C6
---------------------------------
PIN 16 (BCM23)   ->   GPIO 11
PIN 18 (BCM24)   <-   GPIO 10
GND             <->   GND

Software Requirements
-------------------
- Arduino IDE (with Wio Terminal board support)
- ESP-IDF for ESP32-C6
- FreeRTOS Arduino Library for Wio Terminal

Project Structure
---------------
lab4/
.
├── dev1_request_processing_esp32c6
│   ├── CMakeLists.txt
│   ├── main
│   │   ├── CMakeLists.txt
│   │   └── main.c
│   ├── README.md
│   └── sdkconfig
├── dev2_response_measurement_wio_terminal
│   └── main
│       ├── Free_Fonts.h
│       ├── lcd_backlight.hpp
│       └── main.ino
└── readme.txt

Implementation Details
--------------------
Task System:
- Request Task:
  - Generates random-interval GPIO requests (500-2000ms)
  - Monitors system state
  - Priority: High

- Timeout Task:
  - Checks for response timeouts (250ms threshold)
  - Updates timeout statistics
  - Priority: Medium

- Display Task:
  - Updates TFT display with real-time info
  - Handles button input for statistics
  - Priority: Low

Statistics Tracked:
- Minimum response time
- Maximum response time
- Average response time
- Total number of requests
- Number of timeouts
- Timeout percentage

Output Display:
- Real-time request/response logging:
  - Yellow: Request sent
  - Green: Response received
  - Red: Timeout occurred
- Final statistics on button press

Features
--------
- FreeRTOS task management
- Interrupt-driven response detection
- Real-time statistics collection
- TFT display visualization
- Configurable timing parameters
- Button-triggered test completion

Building and Running
-------------------
1. Wio Terminal:
   - Open response_measurement.ino in Arduino IDE
   - Select "Wio Terminal" board
   - Compile and upload

2. ESP32-C6:
   - Navigate to esp32c6 directory
   - Run: idf.py build
   - Run: idf.py -p PORT flash

Usage
-----
1. Connect hardware according to connection diagram 
2. Power both devices (power on esp32 first)
3. System automatically begins sending requests
4. Press button C to end test and view statistics

Author
------
Jakov Jovic
Faculty of Electrical Engineering and Computing
University of Zagreb
