#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi & Supabase info
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* supabase_url = "https://YOUR_PROJECT.supabase.co/rest/v1/irrigation_data"; // Replace with your endpoint
const char* supabase_api_key = "YOUR_SUPABASE_API_KEY";

// Dummy sensor data
float weight = 123.4;
float moisture = 45.6;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  sendSensorData();
}

void loop() {
  // In practice: measure data here and send every N seconds
  delay(60000); // 1 minute
}

void sendSensorData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(supabase_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabase_api_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_api_key);

    // Build JSON
    StaticJsonDocument<200> doc;
    doc["weight"] = weight;
    doc["moisture"] = moisture;
    doc["device_id"] = "sensor-esp32"; // for identification

    String jsonData;
    serializeJson(doc, jsonData);

    int httpCode = http.POST(jsonData);
    String response = http.getString();

    Serial.println("Supabase response code: " + String(httpCode));
    Serial.println("Response: " + response);

    http.end();
  }
}
