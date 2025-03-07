#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "LoRa.h"
#include <ArduinoJson.h>  // Ensure this library is installed

// WiFi credentials
#define WIFI_SSID "KOTA MALANG"
#define WIFI_PASSWORD "nasibayam"

// Firebase credentials
#define FIREBASE_HOST "https://deteksi-banjir-bdf54-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "AIzaSyBDEm3nrxZVU3j7I_VMNwzb1KSlpXaBOh4"

// LoRa settings
#define SS 5
#define RST 15
#define DIO0 2
const unsigned long LORA_FREQUENCY = 433E6;  // 433 MHz

// Initialize Firebase and WiFi clients
FirebaseData firebaseData;

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Serial.println("LoRa Receiver");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    float rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm, SNR: ");
    Serial.println(snr);
    displaySignalStrength(rssi);

    StaticJsonDocument<512> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, receivedData);
    if (!error) {
      serializeJsonPretty(jsonDoc, Serial);
      if (Firebase.setJSON(firebaseData, "/sensorData", jsonDoc)) {
        Serial.println("Data uploaded successfully.");
      } else {
        Serial.println("Failed to upload data. Reason: " + firebaseData.errorReason());
      }
    } else {
      Serial.println("Failed to parse JSON!");
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
