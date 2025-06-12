Automatic IoT Plant Irrigation System with ESP32 & Supabase

This project is a cloud-based automatic irrigation system for plant care, using **ESP32** microcontrollers, **Supabase** for data storage and remote control, and a **React** dashboard for easy management.  

Your friends can easily deploy this system at home without messing with direct device configurations — everything is managed through the cloud!

---

## Architecture Overview

- **ESP32 Sensor Node**  
   Reads soil moisture and plant weight  
   Sends data to Supabase REST API

- **ESP32 Actuator Node**  
   Polls Supabase for irrigation commands  
   Activates a relay to control watering

- **Supabase**  
   Stores sensor data (`irrigation_data`)  
   Stores and manages irrigation commands 
   (`irrigation_commands`)

- **React Dashboard**  
   Displays live data from Supabase  
   Allows users to start/stop irrigation remotely

---

## Features

 Wi-Fi-based communication (no cables!)  
 Cloud-based data storage & control  
 React dashboard for remote monitoring and control  
 Easy deployment at any location with simple Wi-Fi setup  
 Minimal ESP32 storage footprint – cloud handles the heavy lifting!

---

## Project Structure

The project is organized as follows:

```
📁 wateringsystem/
├── 📄 README.md                 # This file: Project overview, setup, and usage
├── 📄 LICENSE                   # Project license
├── 📄 .gitignore                # Specifies intentionally untracked files that Git should ignore
├── 📁 actuator/                 # ESP32 actuator node (PlatformIO project)
│   ├── 📄 platformio.ini         # PlatformIO configuration for the actuator
│   ├── 📁 src/
│   │   └── 📄 main.cpp          # Source code for the actuator ESP32
│   ├── 📁 include/
│   │   ├── 📄 config.h          # (Gitignored) Actual WiFi & Supabase credentials
│   │   └── 📄 config.h.example  # Template for config.h
│   └── 📁 lib/                  # Local libraries (if any)
├── 📁 sensor/                   # ESP32 sensor node (PlatformIO project)
│   ├── 📄 platformio.ini         # PlatformIO configuration for the sensor
│   ├── 📁 src/
│   │   └── 📄 main.cpp          # Source code for the sensor ESP32
│   ├── 📁 include/
│   │   ├── 📄 config.h          # (Gitignored) Actual WiFi & Supabase credentials
│   │   └── 📄 config.h.example  # Template for config.h
│   └── 📁 lib/
├── 📁 supabase/
│   └── 📄 schema.sql            # SQL schema for creating Supabase tables and RLS policies
└── 📁 dashboard/                # (Placeholder for React/Next.js dashboard application)
    └── README.md               # TODO: Add dashboard setup instructions
```

---

##  Setup

### 1️ Supabase Backend

1.  **Create a Supabase Project:**
    *   Go to [Supabase.io](https://supabase.io) and create a new project.
    *   Save your **Project URL**, **anon key**, and **service_role key**. You'll need these.

2.  **Set up Database Schema:**
    *   In your Supabase project, go to the "SQL Editor".
    *   Open the `supabase/schema.sql` file from this repository.
    *   Copy its content and run it in the Supabase SQL editor. This will:
        *   Create the `irrigation_data` table for sensor readings.
        *   Create the `irrigation_commands` table for controlling the actuator.
        *   Set up basic Row Level Security (RLS) policies.
    *   **Important RLS Note:** The provided `schema.sql` sets up initial RLS policies. Review and customize them according to your security needs. For ESP32 devices, it's recommended to use the **anon key** and restrict its permissions via RLS.

### 2️ ESP32 Sensor Node

1.  **Hardware Setup:**
    *   Connect your soil moisture sensor, HX711 load cell (or other sensors) to your ESP32.
    *   Update the placeholder pin definitions in `sensor/src/main.cpp` if needed.

2.  **Firmware Configuration:**
    *   Navigate to the `sensor/include/` directory.
    *   Copy `config.h.example` to a new file named `config.h` in the same directory.
    *   Open `sensor/include/config.h` and replace the placeholder values for:
        *   `WIFI_SSID`
        *   `WIFI_PASSWORD`
        *   (Supabase credentials should be pre-filled if you used the script, otherwise update `SUPABASE_PROJECT_ID` and `SUPABASE_ANON_KEY`)
    *   **Important:** The `sensor/include/config.h` file is already listed in `.gitignore` to prevent your actual credentials from being committed to the repository.

3.  **Build and Upload:**
    *   Open the `sensor/` directory in PlatformIO.
    *   Build and upload the firmware to your ESP32 sensor node.
    *   Monitor the serial output to verify connection and data sending. The device will print its unique `Device ID` (derived from MAC address), which you might need for the actuator or dashboard.

### 3️ ESP32 Actuator Node

1.  **Hardware Setup:**
    *   Connect your relay module to the ESP32.
    *   Update any placeholder pin definitions in `actuator/src/main.cpp` for relay control.

2.  **Firmware Configuration:**
    *   Navigate to the `actuator/include/` directory.
    *   Copy `config.h.example` to a new file named `config.h` in the same directory.
    *   Open `actuator/include/config.h` and replace the placeholder values for:
        *   `WIFI_SSID`
        *   `WIFI_PASSWORD`
        *   (Supabase credentials should be pre-filled, otherwise update `SUPABASE_PROJECT_ID` and `SUPABASE_ANON_KEY`)
    *   **Important:** The `actuator/include/config.h` file is already listed in `.gitignore`.

3.  **Build and Upload:**
    *   Open the `actuator/` directory in PlatformIO.
    *   Build and upload the firmware to your ESP32 actuator node.
    *   Monitor serial output for command polling and actions.

### 4️ React Dashboard (Future Implementation)

*   The `dashboard/` directory is a placeholder for the web interface.
*   **TODO:** Add setup instructions once the dashboard is developed.
    *   Typically, this would involve:
        ```bash
        cd dashboard
        npm install
        # Configure Supabase client (API URL, anon key) in the dashboard code
        npm start
        ```

---

##  How It Works (Updated)

1.  **ESP32 Sensor Node:**
    *   Connects to Wi-Fi.
    *   Generates a unique `device_id` from its MAC address.
    *   Periodically reads sensor data (e.g., HX711 load cell for weight, capacitive soil moisture sensor – **currently uses dummy data, requires implementing actual sensor reads**).
    *   Sends JSON payloads (e.g., `{ "weight": 123.4, "moisture": 45.6, "device_id": "ABCDEF123456" }`) to the `irrigation_data` table in Supabase via REST API using its `anon key`.

2.  **ESP32 Actuator Node:**
    *   Connects to Wi-Fi.
    *   Generates its own unique `device_id` (from MAC address).
    *   Periodically polls the `irrigation_commands` table in Supabase, filtering for commands matching its `device_id` (fetches the latest command for itself) using its `anon key`.
    *   If a command `{ "start": true, ... }` is found, it activates the irrigation relay and sends an acknowledgement update to Supabase.
    *   If a command `{ "start": false, ... }` is found, it deactivates the relay and sends an acknowledgement.

3.  **Supabase:**
    *   Stores historical sensor data in `irrigation_data`.
    *   Stores irrigation commands in `irrigation_commands` (includes an `acknowledged` field).
    *   Manages access control via Row Level Security (RLS) policies defined in `supabase/schema.sql`.

4.  **React Dashboard (Future):**
    *   Will connect to Supabase using the `anon key`.
    *   Will display live and historical data from `irrigation_data`.
    *   Will allow users to send "Start/Stop irrigation" commands by inserting/updating rows in the `irrigation_commands` table.

---

##  Future Improvements

- Web-based Wi-Fi provisioning (WiFiManager for ESP32).  
- OTA updates for ESP32 firmware.  
- Secure communication with SSL/TLS (currently using HTTPS REST).  
- More advanced data visualization in the dashboard.  
- Actuator to handle `duration_seconds` from commands for timed irrigation. (Implemented)

---

##  Screenshots

...

---

##  Contributing

Pull requests and ideas are always welcome! Feel free to submit a PR or open an issue if you have suggestions.

---

##  Acknowledgments

Supabase for their backend services.  
ArduinoJson for making JSON on microcontrollers easy.  
ESP-IDF / ESP32 Arduino core.