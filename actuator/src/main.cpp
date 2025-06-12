#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* supabase_url = "https://YOUR_PROJECT.supabase.co/rest/v1/irrigation_commands?select=*" ;
const char* supabase_api_key = "YOUR_SUPABASE_API_KEY";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
}

void loop() {
  checkForCommand();
  delay(10000); // poll every 10 seconds
}

void checkForCommand() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(supabase_url);
    http.addHeader("apikey", supabase_api_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_api_key);

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Payload: " + payload);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error && doc.size() > 0) {
        // Supabase returns an array of commands
        bool start = doc[0]["start"]; // Example field
        if (start) {
          Serial.println("Start irrigation!");
          // TODO: trigger relay
        } else {
          Serial.println("Stop irrigation.");
          // TODO: stop relay
        }
      }
    } else {
      Serial.println("Error on HTTP GET");
    }
    http.end();
  }
}
