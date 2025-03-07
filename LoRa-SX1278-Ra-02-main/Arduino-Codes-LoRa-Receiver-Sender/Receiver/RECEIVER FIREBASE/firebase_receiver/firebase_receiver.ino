#include <ArduinoOTA.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "LoRa.h"
#include <ArduinoJson.h>  // Ensure the library is installed

// WiFi and Firebase configuration details
#define WIFI_SSID "KOTA MALANG"
#define WIFI_PASSWORD "nasibayam"
#define API_KEY "AIzaSyBDEm3nrxZVU3j7I_VMNwzb1KSlpXaBOh4"
#define DATABASE_URL "https://deteksi-banjir-bdf54-default-rtdb.firebaseio.com/"

// LoRa pin configuration
#define SS 5
#define RST 15
#define DIO0 17
#define LORA_FREQUENCY 433E6  // 433 MHz

FirebaseData firebaseData;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  ArduinoOTA.begin();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.print("Raw Data: ");
    Serial.println(receivedData);  // Debug output
    // Firebase.RTDB.setString(&fbdo, "distance/water", receivedData);
    // Upload the raw data as a string to Firebase
    if (Firebase.RTDB.setString(&firebaseData, "path/to/data", receivedData)) {
      Serial.println("Data water sender uploaded successfully.");
    } else {
      Serial.print("Failed to upload data: ");
      Serial.println(firebaseData.errorReason());
    }
    // Parse JSON after printing raw data
    StaticJsonDocument<512> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, receivedData);
    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
    } else {
      // Process JSON
    }
  }
}


void displaySignalStrength(float rssi) {
  Serial.print("Signal Strength: ");
  if (rssi > -50) Serial.println("★★★★★");
  else if (rssi > -60) Serial.println("★★★★☆");
  else if (rssi > -70) Serial.println("★★★☆☆");
  else if (rssi > -80) Serial.println("★★☆☆☆");
  else Serial.println("★☆☆☆☆");
}
