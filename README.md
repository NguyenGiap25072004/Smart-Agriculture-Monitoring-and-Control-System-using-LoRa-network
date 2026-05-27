# 🌿 Smart Agriculture Monitoring and Control System using LoRa & Edge AI

![Hardware](https://img.shields.io/badge/Hardware-ESP32%20%7C%20LoRa%20SX1278-blue)
![AI](https://img.shields.io/badge/AI_Model-LSTM%20Time--Series-orange)
![Cloud](https://img.shields.io/badge/Cloud-Firebase%20Realtime%20DB-yellow)
![Language](https://img.shields.io/badge/Language-C++%20%7C%20Python%20%7C%20JS-success)

> A comprehensive, end-to-end IoT and Deep Learning solution for precision agriculture. This project integrates a local LoRa P2P network with an Edge AI Server running an LSTM time-series forecasting model to proactively manage greenhouse microclimates and optimize irrigation.

---

## 📖 Table of Contents
- [Project Overview](#-project-overview)
- [Key Features](#-key-features)
- [System Architecture](#-system-architecture)
- [Hardware Components](#-hardware-components)
- [Repository Structure](#-repository-structure)
- [Installation & Setup Guide](#-installation--setup-guide)
  - [1. Firebase Setup](#1-firebase-setup)
  - [2. Hardware Firmware (ESP32)](#2-hardware-firmware-esp32)
  - [3. Edge AI Server (Python)](#3-edge-ai-server-python)
  - [4. Web Dashboard](#4-web-dashboard)
- [Operating Modes](#-operating-modes)
- [Author](#-author)

---

## 🚀 Project Overview

This repository contains the complete source code for a lab-scale mini-greenhouse prototype. The system uses a **Star Network Topology** where multiple distributed **Slave Nodes** collect environmental telemetry (Temperature, Humidity, Light, Soil Moisture) and transmit it via **LoRa 433MHz** to a central **Gateway Master**. The Master synchronizes this data with **Firebase Cloud**, which interfaces with a real-time **Web Dashboard** and a **Python Edge AI Server**.

---

## ✨ Key Features

* **Robust LoRa P2P Communication:** Sequential polling mechanism that completely eliminates radio channel collisions.
* **Three Flexible Control Modes:**
  * 🎛️ **MANUAL:** Remote manual control via the Web UI.
  * ⚙️ **AUTO:** Localized, threshold-based intermittent irrigation and deadband cooling logic ensuring fail-safe operations.
  * 🧠 **AI (Predictive):** A 2-layer LSTM deep learning model forecasts soil moisture 30 minutes into the future to trigger proactive irrigation before drought stress occurs.
* **Real-Time Web Dashboard:** Interactive dynamic charts (Chart.js) and system status banners built with asynchronous event listeners.
* **Automated Data Pipeline:** Background scripts for data logging, cleaning, and model inference.

---

## 🏗 System Architecture

<img width="1528" height="766" alt="image" src="https://github.com/user-attachments/assets/0baf31ba-54a8-4283-852b-f9e6a364200d" />

1. **Uplink:** Sensors -> ESP32 Slaves -> LoRa -> Gateway Master -> Firebase Cloud.
2. **Downlink:** Web UI / AI Server -> Firebase Cloud -> Gateway Master -> LoRa -> ESP32 Slaves -> Relays/Actuators.

---

## 🛠 Hardware Components

| Component | Role / Quantity |
| :--- | :--- |
| **ESP32 Development Board** | Microcontroller for Master (1x) and Slaves (2x). |
| **LoRa Ra-02 (SX1278)** | 433MHz wireless transceiver modules (3x). |
| **DHT11** | Air temperature and humidity sensor (2x). |
| **BH1750 (GY-30)** | I2C light intensity sensor (2x). |
| **Capacitive Soil Moisture** | Analog soil moisture sensor (2x). |
| **Relay Module (Isolated)** | Actuator control (2x). |
| **Mini DC Water Pump & 5V Fan** | Physical actuators for irrigation and ventilation. |

---

## 📁 Repository Structure

```text
📦 Smart-Agriculture-System
 ┣ 📂 1_Firmware_ESP32
 ┃ ┣ 📂 Gateway_Master
 ┃ ┃     ┣ 📜 gateway_master.ino # C++ code for the central coordinating ESP32
 ┃ ┃     ┗ 📜 web_dashboard.h    # Main HTML structure
 ┃ ┗ 📂 Node_Slave
 ┃       ┣ 📜 node_slave1.ino    # C++ code for the distributed sensor ESP32s 1
 ┃       ┗ 📜 node_slave2.ino    # C++ code for the distributed sensor ESP32s 2           
 ┣ 📂 2_Edge_AI_Server
 ┃ ┣ 📜 data_logger.py      # Background script to fetch and clean Firebase data
 ┃ ┣ 📜 train_model.py      # Script to train the LSTM model (64-32 nodes)
 ┃ ┣ 📜 ai_inference.py     # Real-time inference script for active AI control
 ┃ ┗ 📂 models              # Saved .h5 LSTM models and .gz scalers
 ┣ 📜 firebase_key.json     # Firebase integration and Chart.js logic
 ┣ 📂 datasets              # Sample .csv files collected during field testing
 ┗ 📜 README.md

---

##⚙️ Installation & Setup Guide

```text
Follow these sequential steps to replicate and run the project successfully.

1. Firebase Setup
Create a new project on Firebase Console.

Create a Realtime Database (Recommended region: asia-southeast1).

Set the database Security Rules to allow read/write access. (See Appendix B in thesis for strict security rules).

Navigate to Project Settings > Service Accounts and generate a new private key. Save this as firebase_key.json and place it inside the 2_Edge_AI_Server/ directory.

2. Hardware Firmware (ESP32)
Install the Arduino IDE.

Install the ESP32 Core by Espressif via the Boards Manager.

Install required libraries via Library Manager: LoRa (by Sandeep Mistry), Firebase-ESP32, DHT sensor library, BH1750.

Open the Gateway_Master.ino and configure your Wi-Fi credentials and Firebase RTDB URL/Auth token. Flash it to the Master ESP32.

Open Node_Slave.ino, configure the Node IDs (S1, S2), and flash them to the respective Slave ESP32s.

3. Edge AI Server (Python)
Ensure you have Python 3.10+ installed.

Open your terminal and navigate to the AI folder:

Bash
cd 2_Edge_AI_Server
Install the required deep learning and data processing dependencies:

Bash
pip install tensorflow keras pandas numpy scikit-learn joblib firebase-admin
Run the Data Logger: (To start collecting real-time training data)

Bash
python data_logger.py
Run AI Inference: (Once the model is trained, start the background AI worker)

Bash
python ai_inference.py

---
4. Web Dashboard
Connect all devices and paste the Local IP to run your website in any Browswer (Google Chrome / Edge).

Update the Firebase initialization config block inside script.js with your specific Firebase project credentials.

The dashboard will automatically pull historical data, render the charts, and allow you to switch control modes.
---

## 🎮 Operating Modes
You can switch between these modes via the Dropdown menu on the Web Dashboard:

* MANUAL: Overrides all logic. Use the UI toggle switches to turn the Pump/Fan ON or OFF.

* AUTO: The Gateway Master takes over.

  ** Pump: Activates for 10s if soil moisture < 55%, then waits 3 mins.

  ** Fan: Turns on at >32°C and off at <29°C (3°C deadband).

* AI: The Python Edge Server analyzes the last 60 mins of data and predicts the moisture 30 mins ahead. If the predicted moisture drops below the safety threshold, it proactively triggers the pump.
---

## 👨‍💻 Author
Nguyen Huu Giap Computer Science and Engineering

Vietnam Japan University, VNU Hanoi

Contact: [Insert your LinkedIn/Email here]

This repository is part of my undergraduate Graduation Thesis. If you find this project helpful or use it as a reference, leaving a ⭐ on the repository is highly appreciated!
