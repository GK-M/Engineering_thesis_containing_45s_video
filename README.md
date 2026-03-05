# Diploma Thesis Repository

**Project and Implementation of a Microprocessor Platform for Building Distributed I/O Using a Selected Protocol Supported by Siemens S7-1200 PLCs0**

This repository contains my diploma thesis project focused on building a **distributed I/O platform** based on **Modbus TCP/IP**, where a **Siemens S7-1200 PLC** acts as the **Modbus client** and **ESP32 nodes** act as **Modbus servers**.

![zd](https://github.com/user-attachments/assets/f4a550b4-8877-4559-bd35-0c70119be33d)
![Obraz1.png]
## System concept

A local **LAN network** connects the PLC and multiple ESP32-based I/O nodes. Each ESP32 collects sensor data, performs basic local processing (min/max tracking, event detection), and exposes registers to the PLC via **Modbus TCP/IP**. The PLC then processes the data and presents it on an HMI.

## ESP32 node functionalities

* **Temperature monitoring (DS18B20)**

  * Current temperature reading
  * Tracking and reporting **minimum** and **maximum** values
* **Acoustic event detection (MAX4466 microphone module)**

  * Detecting crack/pop-like impulse events
* **Production line part counting (VL53L1X ToF sensor)**

  * Counting items on a production line
  * Tracking **minimum** and **maximum** distance readings
* **Status signaling**

  * LED indicators for device state and detected events/conditions
* **Power redundancy**

  * Dual power supply: **wired power** and **battery backup**

## PLC and HMI implementation (S7-1200)

* Modbus TCP/IP **client** implementation to read/write ESP32 registers
* PLC program for data handling, scaling, alarms/limits, and diagnostics
* **HMI screens** to visualize measurements, counters, min/max values, and device status


## Mechanical and hardware work

* 3D modeling and fabrication of an **enclosure** for all Modbus server nodes
* Design of an **interconnection/interface board** for wiring and integration
* Preparation of **technical drawings** and documentation

