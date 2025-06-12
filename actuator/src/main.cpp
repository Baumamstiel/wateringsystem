#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- WIFI & SUPABASE CREDENTIALS ---
// TODO: Replace with your actual credentials
// Consider using a config.h file (add to .gitignore) or WiFiManager for better security and flexibility.
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// const char* supabase_url = "https://YOUR_PROJECT_ID.supabase.co/rest/v1/irrigation_commands?select=*&order=timestamp.desc&limit=1"; // Old URL
const char* supabase_project_id = "YOUR_PROJECT_ID"; // Define your Supabase project ID here
const char* supabase_anon_key = "YOUR_SUPABASE_ANON_KEY"; // Use ANON KEY for client-side access

String supabase_commands_table_url = ""; // Will be constructed in setup
String actuatorDeviceID = ""; // Will be set in setup

// --- RELAY CONFIGURATION ---
const int RELAY_PIN = 16; // Example GPIO pin for the relay, adjust to your wiring
                          // Choose a suitable GPIO pin on your ESP32 that is not used for other purposes.

void setup() {
  Serial.begin(115200);
  delay(100); // Short delay for serial initialization

  actuatorDeviceID = getDeviceID();
  Serial.print("Actuator Device ID: ");
  Serial.println(actuatorDeviceID);

  // Construct Supabase URLs
  supabase_commands_table_url = "https://" + String(supabase_project_id) + ".supabase.co/rest/v1/irrigation_commands";

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

void acknowledgeCommand(long commandId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot acknowledge command, WiFi disconnected.");
    return;
  }

  Serial.print("Acknowledging command ID: ");
  Serial.println(commandId);

  HTTPClient http;
  String patchUrl = supabase_commands_table_url + "?id=eq." + String(commandId);

  http.begin(patchUrl);
  http.addHeader("apikey", supabase_anon_key);
  http.addHeader("Authorization", "Bearer " + String(supabase_anon_key));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal"); // Ask Supabase to return no content

  StaticJsonDocument<64> patchDoc;
  patchDoc["acknowledged"] = true;
  String patchBody;
  serializeJson(patchDoc, patchBody);

  Serial.println("PATCH URL: " + patchUrl);
  Serial.println("PATCH Body: " + patchBody);

  int httpCode = http.PATCH(patchBody);

  if (httpCode == HTTP_CODE_NO_CONTENT) { // Supabase returns 204 No Content on successful PATCH with Prefer: return=minimal
    Serial.println("Command acknowledged successfully.");
  } else {
    Serial.print("Error acknowledging command. HTTP Code: ");
    Serial.println(httpCode);
    String response = http.getString(); // Get response body for debugging
    Serial.println("Response: " + response);
  }
  http.end();
}

// Function to get a unique device ID (using MAC address)
String getDeviceID() {
  char chipid_str[13];
  uint64_t chipid = ESP.getEfuseMac(); // The ESP32 Chip ID is essentially its MAC address
  snprintf(chipid_str, sizeof(chipid_str), "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(chipid_str);
}

void checkForCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Filter commands for this specific actuator, get the latest unacknowledged, or latest overall if none unacknowledged.
    // This logic can be refined. For now, just getting the latest for this device.
    String getUrl = supabase_commands_table_url + \
                    "?select=*&device_id=eq." + actuatorDeviceID + \
                    "&order=timestamp.desc&limit=1"; 
                    // Consider adding "&acknowledged=is.false" to only get pending commands,
                    // but this might miss a 'stop' if the 'start' was missed and acknowledged.
                    // Simpler to get the latest command for the device and let its state (start: true/false) dictate action.

    Serial.print("Checking for commands at: ");
    Serial.println(getUrl);

    http.begin(getUrl);
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
        
        long commandId = -1;
        if (command.containsKey("id")) {
          commandId = command["id"].as<long>();
        } else {
          Serial.println("Command payload does not contain 'id' field.");
          http.end();
          return;
        }

        if (command.containsKey("start")) {
          bool start = command["start"];
          if (start) {
            Serial.println("Received START irrigation command!");
            digitalWrite(RELAY_PIN, HIGH); // Turn relay ON
            Serial.println("Relay turned ON.");
            acknowledgeCommand(commandId);
          } else {
            Serial.println("Received STOP irrigation command.");
            digitalWrite(RELAY_PIN, LOW); // Turn relay OFF
            Serial.println("Relay turned OFF.");
            acknowledgeCommand(commandId);
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
