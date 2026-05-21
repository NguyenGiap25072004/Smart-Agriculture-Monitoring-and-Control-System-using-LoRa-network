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

*(Optional: Add your architecture diagram here)*
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
 ┃ ┣ 📂 Gateway_Master      # C++ code for the central coordinating ESP32
 ┃ ┗ 📂 Node_Slave          # C++ code for the distributed sensor ESP32s
 ┣ 📂 2_Edge_AI_Server
 ┃ ┣ 📜 data_logger.py      # Background script to fetch and clean Firebase data
 ┃ ┣ 📜 train_model.py      # Script to train the LSTM model (64-32 nodes)
 ┃ ┣ 📜 ai_inference.py     # Real-time inference script for active AI control
 ┃ ┗ 📂 models              # Saved .h5 LSTM models and .gz scalers
 ┣ 📂 3_Web_Dashboard
 ┃ ┣ 📜 index.html          # Main HTML structure
 ┃ ┣ 📜 style.css           # UI Styling
 ┃ ┗ 📜 script.js           # Firebase integration and Chart.js logic
 ┣ 📂 datasets              # Sample .csv files collected during field testing
 ┗ 📜 README.md
