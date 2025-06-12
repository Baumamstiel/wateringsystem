#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // WiFiManager library
#include "HX711.h" // For load cell amplifier
#include "config.h" // Include the new config file

// --- WIFI & SUPABASE CREDENTIALS ---
// Credentials are now sourced from config.h
// const char* ssid = "YOUR_WIFI_SSID";
// const char* password = "YOUR_WIFI_PASSWORD";
// const char* supabase_url = "https://pedfmureqmgrgoytgtkx.supabase.co/rest/v1/irrigation_data"; // From config.h
// const char* supabase_anon_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InBlZGZtdXJlcW1ncmdveXRndGt4Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDk0NzIwNDEsImV4cCI6MjA2NTA0ODA0MX0.5jXdxoqGBEiwytHvIkkmcWUzQoMxLlfNf0FTCT6GR-s"; // From config.h

String supabase_data_table_url = ""; // Will be constructed in setup

// --- SENSOR CONFIGURATION ---
// HX711 Load Cell
const int HX711_DOUT_PIN = 4; // Example GPIO pin, adjust to your wiring
const int HX711_SCK_PIN = 5;  // Example GPIO pin, adjust to your wiring
HX711 scale;
float calibration_factor = -7050; // This value must be calibrated for your specific load cell setup

// Soil Moisture Sensor (Analog)
const int SOIL_MOISTURE_PIN = A0; // Example Analog pin (check ESP32 ADC compatible pins, e.g., GPIO 36, 39, etc.)
                                  // For ESP32, analog pins are typically GPIOs 32-39 (ADC1) and 0, 2, 4, 12-15, 25-27 (ADC2)
                                  // Using GPIO 36 (ADC1_CH0) as an example if A0 is not directly mapped.
// const int SOIL_MOISTURE_PIN = 36;


// How often to send data (in milliseconds)
const unsigned long SEND_INTERVAL_MS = 60000; // 1 minute
unsigned long previousMillis = 0;

// Function to get a unique device ID (using MAC address)
String getDeviceID() {
  char chipid_str[13];
  uint64_t chipid = ESP.getEfuseMac(); // The ESP32 Chip ID is essentially its MAC address
  snprintf(chipid_str, sizeof(chipid_str), "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(chipid_str);
}

void setup() {
  Serial.begin(115200);
  delay(100); // Short delay for serial initialization

  // WiFiManager
  WiFiManager wm;
  // wm.resetSettings(); // Uncomment to reset saved settings for testing

  // Set custom parameters if needed, e.g., for Supabase credentials
  // WiFiManagerParameter custom_supabase_project_id("supabase_id", "Supabase Project ID", SUPABASE_PROJECT_ID, 40);
  // WiFiManagerParameter custom_supabase_anon_key("supabase_key", "Supabase Anon Key", SUPABASE_ANON_KEY, 200);
  // wm.addParameter(&custom_supabase_project_id);
  // wm.addParameter(&custom_supabase_anon_key);

  Serial.println("Starting WiFiManager...");
  if (!wm.autoConnect("PlantSensorSetupAP")) { // AP name
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart(); // Restart ESP if it cannot connect
    delay(5000);
  }

  Serial.println("\\\\nWi-Fi connected via WiFiManager!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Device ID: ");
  Serial.println(getDeviceID());

  // Update Supabase credentials if they were part of WiFiManager custom parameters
  // strcpy(SUPABASE_PROJECT_ID, custom_supabase_project_id.getValue());
  // strcpy(SUPABASE_ANON_KEY, custom_supabase_anon_key.getValue());

  // Construct Supabase URL for the data table
  supabase_data_table_url = "https://" + String(SUPABASE_PROJECT_ID) + ".supabase.co/rest/v1/irrigation_data";
  Serial.println("Supabase URL: " + supabase_data_table_url);


  // --- Initialize Sensors ---
  Serial.println("Initializing sensors...");

  // Initialize HX711
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  scale.set_scale(calibration_factor); // Adjust this factor during calibration
  scale.tare();                      // Reset the scale to 0

  Serial.println("HX711 initialized and tared.");
  // For soil moisture, analogRead setup is usually just ensuring the pin is an input, which is default for analogRead.
  // pinMode(SOIL_MOISTURE_PIN, INPUT); // Not strictly necessary for analogRead on ESP32 for ADC1 pins

  Serial.println("Sensors initialized.");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= SEND_INTERVAL_MS) {
    previousMillis = currentMillis;
    readAndSendSensorData();
  }

  // WiFiManager handles reconnection automatically, so explicit reconnection logic might not be needed
  // or can be simplified. If WiFi drops, WiFiManager might try to reconnect or re-enter AP mode
  // depending on its configuration. For now, we'll rely on its default behavior.
  // if (WiFi.status() != WL_CONNECTED) {
  //   Serial.println("WiFi Disconnected. WiFiManager should handle reconnection.");
  //   // Potentially add a fallback or alert if WiFi remains disconnected for too long
  // }
}

void readAndSendSensorData() {
  if (WiFi.status() == WL_CONNECTED) {
    // --- Read actual sensor data ---
    float weight = 0.0;
    if (scale.is_ready()) {
      weight = scale.get_units(5); // Read average of 5 readings
      Serial.print("Raw Weight: "); Serial.println(weight);
    } else {
      Serial.println("HX711 not found.");
    }

    // Read Soil Moisture
    // The raw analog value will depend on your sensor and ESP32's ADC resolution (typically 0-4095 for 12-bit)
    // You might need to map this value to a percentage or a calibrated range.
    int rawMoisture = analogRead(SOIL_MOISTURE_PIN);
    // Example: map rawMoisture (e.g., 0-4095) to a 0-100% scale.
    // This mapping is highly dependent on your specific sensor's characteristics (dry vs. wet readings).
    // float moisture = map(rawMoisture, 4095, 1000, 0, 100); // Example: 4095=dry (0%), 1000=wet (100%)
    // For simplicity, we'll send the raw value or a direct conversion for now.
    float moisture = (float)rawMoisture; // Sending raw ADC value for now. Calibrate and map as needed.
    Serial.print("Raw Moisture (ADC): "); Serial.println(rawMoisture);


    Serial.println("Preparing to send sensor data...");
    Serial.print("Weight: "); Serial.println(weight);
    Serial.print("Moisture: "); Serial.println(moisture);

    HTTPClient http;
    http.begin(supabase_data_table_url); // Use constructed URL
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_ANON_KEY); // Use credential from config.h
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY)); // Use credential from config.h

    StaticJsonDocument<256> doc; // Increased size slightly for more data if needed
    doc["weight"] = weight;
    doc["moisture"] = moisture;
    doc["device_id"] = getDeviceID();
    // Add other data fields as necessary, e.g.:
    // doc["temperature"] = readTemperature();

    String jsonData;
    serializeJson(doc, jsonData);
    Serial.println("Sending JSON: " + jsonData);

    int httpCode = http.POST(jsonData);
    
    if (httpCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Code: "); Serial.println(httpCode);
      Serial.println("Response: " + response);
      if (httpCode == HTTP_CODE_CREATED) { // 201
        Serial.println("Data successfully sent to Supabase.");
      } else {
        Serial.print("Error sending data. Supabase response: ");
        Serial.println(response);
      }
    } else {
      Serial.print("Error on HTTP POST: ");
      Serial.println(httpCode);
      Serial.print("HTTP error: ");
      Serial.println(http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Cannot send data, WiFi disconnected.");
  }
}
