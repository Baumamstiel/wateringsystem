🌱 Automatic IoT Plant Irrigation System with ESP32 & Supabase

This project is a cloud-based automatic irrigation system for plant care, using **ESP32** microcontrollers, **Supabase** for data storage and remote control, and a **React** dashboard for easy management.  

Your friends can easily deploy this system at home without messing with direct device configurations — everything is managed through the cloud!

---

## 🏗️ Architecture Overview

- **ESP32 Sensor Node**  
  📦 Reads soil moisture and plant weight  
  ☁️ Sends data to Supabase REST API

- **ESP32 Actuator Node**  
  📦 Polls Supabase for irrigation commands  
  💧 Activates a relay to control watering

- **Supabase**  
  📊 Stores sensor data (`irrigation_data`)  
  🟢 Stores and manages irrigation commands (`irrigation_commands`)

- **React Dashboard**  
  👨‍💻 Displays live data from Supabase  
  🕹️ Allows users to start/stop irrigation remotely

---

## 🚀 Features

✅ Wi-Fi-based communication (no cables!)  
✅ Cloud-based data storage & control  
✅ React dashboard for remote monitoring and control  
✅ Easy deployment at any location with simple Wi-Fi setup  
✅ Minimal ESP32 storage footprint – cloud handles the heavy lifting!

---

## ⚙️ How It Works

1. **ESP32 Sensor Node**  
   - Reads sensor data (e.g., HX711 load cell for weight, capacitive soil moisture sensor).  
   - Sends JSON payloads (e.g., `{ "weight": 123.4, "moisture": 45.6 }`) to Supabase REST API every minute.

2. **ESP32 Actuator Node**  
   - Periodically polls Supabase for the latest irrigation command.  
   - If `start` is `true`, activates the irrigation relay.

3. **Supabase**  
   - Acts as a central MQTT-like broker but via REST.  
   - Stores historical data for analysis.

4. **React Dashboard**  
   - Displays live data from the database.  
   - Allows users to send “Start/Stop irrigation” commands to Supabase.

---

## 📦 Project Structure
```
📁 firmware/
├── esp32_sensor_node/  
│   └──  # ESP32 code for sensor data upload
├── esp32_actuator_node/   
│   └──  # ESP32 code for irrigation relay control
📁 dashboard/
├── public/   
│   └──  # React public assets
├── src/   
│   └──  # React components
├── package.json   
│   └──  # React dependencies
📁 supabase/
├── schema.sql   
│   └──  # SQL schema for tables
└── README.md   
    └──  # Supabase setup instructions
```

---

## 🛠️ Setup

### 1️⃣ Supabase

- Create a new project at [Supabase.io](https://supabase.io).
- Create two tables:
  - `irrigation_data` (id, weight, moisture, device_id, timestamp).
  - `irrigation_commands` (id, start, timestamp).
- Set up **Row Level Security (RLS)** to allow inserts/reads for your ESP32 clients.
- Save the `API_URL` and `API_KEY` for later use.

### 2️⃣ ESP32 Sensor Node

- Flash the code from `firmware/esp32_sensor_node/`.
- Configure Wi-Fi SSID, password, and Supabase credentials.
- Upload using PlatformIO or Arduino IDE.

### 3️⃣ ESP32 Actuator Node

- Flash the code from `firmware/esp32_actuator_node/`.
- Similarly configure Wi-Fi and Supabase credentials.

### 4️⃣ React Dashboard

- Install dependencies:
  ```bash
  cd dashboard
  npm install


🌟 Future Improvements

🌐 Web-based Wi-Fi provisioning (WiFiManager for ESP32).
📦 OTA updates for ESP32 firmware.
🔒 Secure communication with SSL/TLS (currently using HTTPS REST).
🌈 More advanced data visualization in the dashboard.

📸 Screenshots


(Add real screenshots here!)

🤝 Contributing

Pull requests and ideas are always welcome! Feel free to submit a PR or open an issue if you have suggestions.

✨ Acknowledgments

Supabase for their awesome backend services.
ArduinoJson for making JSON on microcontrollers easy.
ESP-IDF / ESP32 Arduino core.