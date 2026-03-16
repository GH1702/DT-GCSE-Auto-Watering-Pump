# ESP32 Automatic Plant Watering System

## Overview

This project is an automatic plant watering system built using an ESP32 microcontroller. The system monitors soil moisture levels and automatically waters plants when the soil becomes too dry.

It is designed to water up to 4 plants individually, using soil moisture sensors and small water pumps. The system also provides information through a small OLED display and can connect to WiFi for remote control and notifications.

The goal of the project is to make plant care easier by automatically maintaining the correct soil moisture level.

Test the WEB UI here - https://gh1702.github.io/DT-GCSE-Auto-Watering-Pump/

## Features

- Automatic watering based on soil moisture levels  
- Supports 4 plants independently  
- Capacitive soil moisture sensors for accurate readings  
- 4 channel relay module to control pumps  
- 4 DC submersible water pumps  
- 0.96 inch I2C OLED display for status information  
- Ultrasonic sensor to monitor water tank level  
- WiFi connectivity using ESP32  
- Web interface for monitoring and control  
- Webhook support for notifications (WhatsApp via CallMeBot)  
- MQTT support planned for integration with home automation  
- Physical buttons for manual control  
- Portable design powered from a USB power bank  

---

## Hardware Used

- ESP32 or ESP32U microcontroller  
- 4 x Capacitive Soil Moisture Sensors  
- 4 x 5V Mini Water Pumps  
- 4 Channel Relay Module  
- 0.96 inch I2C OLED Display  
- Ultrasonic Distance Sensor for water tank level  
- Water tank or reservoir  
- Silicone tubing  
- Power bank USB  

---

## System Layout

Each plant has its own sensor and pump.

1. The soil moisture sensor measures how wet the soil is.  
2. The ESP32 reads the sensor value.  
3. If the soil is too dry, the ESP32 activates the relay.  
4. The relay turns on the water pump.  
5. Water is pumped from the tank to the plant.  

The ultrasonic sensor checks how much water is left in the tank.

---

## Web Interface

The ESP32 hosts a small web server that allows you to:

- View soil moisture levels  
- See pump status  
- Manually activate pumps  
- Monitor the water tank level  

This can be accessed through the ESP32 IP address on your local network.

---

## Notifications

The system can send WhatsApp notifications using the CallMeBot API.

For example, it can notify when:

- The water tank is empty  
- A plant has been watered  
- A sensor reading is abnormal  

---

## Future Improvements

Possible improvements include:

- Full MQTT dashboard integration  
- Historical moisture logging  
- Mobile friendly web interface  
- Adjustable watering schedules  
- Battery level monitoring  
- Calibration system for moisture sensors  

---

## Project Purpose

This project was created as part of a Design and Technology coursework project. It demonstrates:

- Embedded programming  
- Electronics integration  
- Sensor data processing  
- IoT connectivity  
- Product design  

---

## License

This project is for educational use.


Here is a simulation of the WEB UI
https://es-d-2019274820260318-019cf8e5-2368-7f46-ad5e-acafadcbe6a8.codepen.dev/
