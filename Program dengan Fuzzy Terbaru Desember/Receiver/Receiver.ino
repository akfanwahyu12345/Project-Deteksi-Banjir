#include <LoRa.h>
#include <ArduinoJson.h>  // Pastikan Anda telah menginstal library ArduinoJson
#define SS 5
#define RST 14
#define DIO0 2

// Definisikan frekuensi yang sama dengan sender
const unsigned long LORA_FREQUENCY = 433E6;  // 433 MHz
void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.println("Receiver Host");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa Error");
    while (1)
      ;
  }
}
void loop() {
  // int packetSize = LoRa.parsePacket();
  // if (packetSize) {
  //   Serial.print("Re");
  //   while (LoRa.available()) {
  //     String data = LoRa.readString();
  //     Serial.println(data);
  //   }
  // }
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received Data: ");
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    Serial.println(receivedData);
    // Dapatkan RSSI dan SNR
    float rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    // Tampilkan data yang diterima
    Serial.print("Received Data: ");
    Serial.println(receivedData);

    // Tampilkan RSSI dan SNR
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm, SNR: ");
    Serial.print(snr);
    Serial.println(" dB");

    // Visualisasi Indikator Kekuatan Sinyal
    displaySignalStrength(rssi);

    // Parsing JSON untuk menampilkan frekuensi
    StaticJsonDocument<512> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, receivedData);

    if (!error) {
      JsonArray jsonArray = jsonDoc.as<JsonArray>();
      for (JsonObject obj : jsonArray) {
        if (strcmp(obj["sensor"], "Frekuensi LoRa") == 0) {
          float frequency = obj["value"];
          const char* unit = obj["unit"];
          Serial.print("Frekuensi LoRa: ");
          Serial.print(frequency);
          Serial.print(" ");
          Serial.println(unit);
        }
      }
    } else {
      Serial.println("Error parsing JSON");
    }
  }
}

// Fungsi untuk menampilkan indikator kekuatan sinyal void displaySignalStrength(float rssi) {
void displaySignalStrength(float rssi) {
  Serial.print("Signal Strength: ");
  if (rssi > -50) {
    Serial.println("★★★★★");
  } else if (rssi > -60) {
    Serial.println("★★★★☆");
  } else if (rssi > -70) {
    Serial.println("★★★☆☆");
  } else if (rssi > -80) {
    Serial.println("★★☆☆☆");
  } else {
    Serial.println("★☆☆☆☆");
  }
}
