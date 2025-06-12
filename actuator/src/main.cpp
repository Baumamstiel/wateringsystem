#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- WIFI & SUPABASE CREDENTIALS ---
// TODO: Replace with your actual credentials
// Consider using a config.h file (add to .gitignore) or WiFiManager for better security and flexibility.
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* supabase_url = "https://YOUR_PROJECT_ID.supabase.co/rest/v1/irrigation_commands?select=*&order=timestamp.desc&limit=1"; // Fetches the latest command
const char* supabase_anon_key = "YOUR_SUPABASE_ANON_KEY"; // Use ANON KEY for client-side access

// --- RELAY CONFIGURATION ---
const int RELAY_PIN = 16; // Example GPIO pin for the relay, adjust to your wiring
                          // Choose a suitable GPIO pin on your ESP32 that is not used for other purposes.

void setup() {
  Serial.begin(115200);
  delay(100); // Short delay for serial initialization

  // Initialize Relay Pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is OFF by default
  Serial.println("Relay pin initialized and set to OFF.");

  Serial.println("\\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  checkForCommand();
  delay(10000); // Poll Supabase every 10 seconds
}

void checkForCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    Serial.print("Checking for commands at: ");
    Serial.println(supabase_url);

    http.begin(supabase_url);
    http.addHeader("apikey", supabase_anon_key); // Standard header for Supabase
    http.addHeader("Authorization", "Bearer " + String(supabase_anon_key)); // Standard header for Supabase

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.print("HTTP Code: ");
      Serial.println(httpCode);
      Serial.println("Payload: " + payload);

      StaticJsonDocument<256> doc; // Reduced size as we only expect one command
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        http.end();
        return;
      }

      if (doc.is<JsonArray>() && doc.as<JsonArray>().size() > 0) {
        JsonObject command = doc.as<JsonArray>()[0]; // Get the first (and only) command object
        if (command.containsKey("start")) {
          bool start = command["start"];
          if (start) {
            Serial.println("Received START irrigation command!");
            digitalWrite(RELAY_PIN, HIGH); // Turn relay ON
            Serial.println("Relay turned ON.");
            // TODO: Consider sending an acknowledgement back to Supabase
            // e.g., update the 'acknowledged' field for this command
          } else {
            Serial.println("Received STOP irrigation command.");
            digitalWrite(RELAY_PIN, LOW); // Turn relay OFF
            Serial.println("Relay turned OFF.");
            // TODO: Consider sending an acknowledgement back to Supabase
          }
        } else {
          Serial.println("Command payload does not contain 'start' field.");
        }
      } else {
        Serial.println("No commands found or payload is not a non-empty array.");
      }
    } else {
      Serial.print("Error on HTTP GET: ");
      Serial.println(httpCode);
      Serial.print("Response: ");
      Serial.println(http.getString());
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected. Trying to reconnect...");
    WiFi.begin(ssid, password); // Attempt to reconnect
    int reconnectAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && reconnectAttempts < 20) { // Try for 10 seconds
        delay(500);
        Serial.print(".");
        reconnectAttempts++;
    }
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("\\nFailed to reconnect to WiFi.");
    } else {
        Serial.println("\\nWiFi reconnected!");
    }
  }
}
