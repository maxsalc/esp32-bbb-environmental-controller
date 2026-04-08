# Environmental Control System (ESP32 + BeagleBone Black)

## Overview

This project monitors room temperature and humidity and automatically turns a fan on or off. Instead of modifying the fan electrically, a servo physically presses the button on a wireless remote.

An ESP32 handles the sensor readings and control logic, while a BeagleBone Black logs data and hosts a simple web dashboard that can be accessed from a phone or computer on the same network.

## Hardware

- ESP32  
- BeagleBone Black running Debian
- DHT22 temperature/humidity sensor  
- SG90 servo motor  
- Wireless fan remote  
- Breadboard and jumper wires  

## Software / Tools

- C++ (Arduino framework, PlatformIO) on ESP32  
- Python on BeagleBone Black  
- Flask (web dashboard)  
- UART communication between boards  

## How it works

The ESP32 continuously reads temperature and humidity from the DHT22 and runs a basic state machine. Under normal conditions it stays in a monitoring state. When the temperature passes a set threshold, the servo moves and physically presses the fan remote button, then returns to monitoring.

At the same time, the ESP32 sends status updates over UART in a simple format like:

TEMP_F=77.36, HUM=48.10, FAN=OFF, STATE=MONITORING

The BeagleBone Black reads this data from /dev/ttyS1, parses it, logs it into a CSV file, and displays the latest values through a small Flask web app.

## Running the system

Flash the ESP32 using PlatformIO and connect the sensor, servo, and UART lines (shared ground required).

On the BeagleBone Black:

- Navigate to the linux_app folder  
- Create and activate a virtual environment  
- Install requirements  
- Run the logger and web app  

Then open:

http://<bbb-ip>:5000

## Example data

The system logs data in CSV format:

timestamp,temp_f,humidity,fan,state  
2026-04-03 22:25:49,77.36,48.10,OFF,MONITORING  

## Notes

The fan is controlled by physically pressing the remote, not by wiring into the fan itself. Communication between devices is done over UART, and the web page refreshes automatically to show the latest data.

## Limitations

- No error checking on UART data  
- Fan state is assumed, not measured  
- Manual startup (no auto-run on boot)  

## Possible improvements

- Send commands from BBB back to ESP32  
- Add error handling / data validation  
- Set up automatic startup on boot  
- Improve the web interface  