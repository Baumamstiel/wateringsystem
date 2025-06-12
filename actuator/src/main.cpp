#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h" // Include the new config file

// --- WIFI & SUPABASE CREDENTIALS ---
// Credentials are now sourced from config.h
// const char* ssid = "YOUR_WIFI_SSID"; // From config.h
// const char* password = "YOUR_WIFI_PASSWORD"; // From config.h
// const char* supabase_project_id = "pedfmureqmgrgoytgtkx"; // From config.h
// const char* supabase_anon_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InBlZGZtdXJlcW1ncmdveXRndGt4Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDk0NzIwNDEsImV4cCI6MjA2NTA0ODA0MX0.5jXdxoqGBEiwytHvIkkmcWUzQoMxLlfNf0FTCT6GR-s"; // From config.h

String supabase_commands_table_url = ""; // Will be constructed in setup
String actuatorDeviceID = ""; // Will be set in setup

// --- RELAY CONFIGURATION ---
const int RELAY_PIN = 16; // Example GPIO pin for the relay, adjust to your wiring
                          // Choose a suitable GPIO pin on your ESP32 that is not used for other purposes.

// --- IRRIGATION STATE ---
bool isIrrigating = false;
unsigned long irrigationStartTime = 0;
int currentIrrigationDuration = 0; // in seconds

void setup() {
  Serial.begin(115200);
  delay(100); // Short delay for serial initialization

  actuatorDeviceID = getDeviceID();
  Serial.print("Actuator Device ID: ");
  Serial.println(actuatorDeviceID);

  // Construct Supabase URLs
  supabase_commands_table_url = "https://" + String(SUPABASE_PROJECT_ID) + ".supabase.co/rest/v1/irrigation_commands";

  // Initialize Relay Pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is OFF by default
  Serial.println("Relay pin initialized and set to OFF.");

  Serial.println("\\nConnecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Use credentials from config.h
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
  manageIrrigationCycle(); // Manage timed irrigation
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
  http.addHeader("apikey", SUPABASE_ANON_KEY); // Use credential from config.h
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY)); // Use credential from config.h
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
    String getUrl = supabase_commands_table_url + \\
                    "?select=*&device_id=eq." + actuatorDeviceID + \\
                    "&acknowledged=is.false" + // Only fetch unacknowledged commands
                    "&order=timestamp.asc&limit=1"; // Get the oldest unacknowledged command
                    // Fetching oldest unacknowledged ensures we process in order.

    Serial.print("Checking for commands at: ");
    Serial.println(getUrl);

    http.begin(getUrl);
    http.addHeader("apikey", SUPABASE_ANON_KEY); // Use credential from config.h
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY)); // Use credential from config.h

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

        // It's important to acknowledge a command *after* its action is fully processed
        // or, in the case of timed irrigation, after it's initiated.

        if (command.containsKey("start")) {
          bool start_command = command["start"];

          if (start_command) {
            Serial.println("Received START irrigation command!");
            digitalWrite(RELAY_PIN, HIGH); // Turn relay ON
            isIrrigating = true;
            irrigationStartTime = millis();
            
            if (command.containsKey("duration_seconds") && !command["duration_seconds"].isNull()) {
              currentIrrigationDuration = command["duration_seconds"].as<int>();
              Serial.print("Irrigation duration set to: ");
              Serial.print(currentIrrigationDuration);
              Serial.println(" seconds.");
            } else {
              currentIrrigationDuration = 0; // Means indefinite, or rely on manual stop
              Serial.println("No duration specified, pump will run until a STOP command or manual stop.");
            }
            Serial.println("Relay turned ON.");
            acknowledgeCommand(commandId); // Acknowledge after initiating
          } else { // start_command is false
            Serial.println("Received STOP irrigation command.");
            digitalWrite(RELAY_PIN, LOW); // Turn relay OFF
            isIrrigating = false;
            irrigationStartTime = 0;
            currentIrrigationDuration = 0;
            Serial.println("Relay turned OFF.");
            acknowledgeCommand(commandId); // Acknowledge after stopping
          }
        } else {
          Serial.println("Command payload does not contain 'start' field.");
          // If a command doesn't have 'start', it's malformed for our use case.
          // We could acknowledge it to remove it from the queue or log an error.
          // For now, let's acknowledge it to prevent it from blocking other commands.
          acknowledgeCommand(commandId);
        }
      } else {
        Serial.println("No new unacknowledged commands found.");
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
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Use credentials from config.h
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

void manageIrrigationCycle() {
  if (isIrrigating && currentIrrigationDuration > 0) {
    unsigned long elapsedTime = millis() - irrigationStartTime;
    if (elapsedTime >= (unsigned long)currentIrrigationDuration * 1000) {
      Serial.println("Irrigation duration elapsed. Turning relay OFF.");
      digitalWrite(RELAY_PIN, LOW);
      isIrrigating = false;
      // Optionally, reset irrigationStartTime and currentIrrigationDuration here,
      // but they will be reset by the next command anyway.
      // Consider if a separate "completed" state is needed or if just setting isIrrigating = false is enough.
    }
  }
}
