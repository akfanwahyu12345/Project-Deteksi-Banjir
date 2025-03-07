#include <LoRa.h>
#include <ArduinoJson.h>  // Pastikan library ini telah diinstal

#define SS 5
#define RST 14
#define DIO0 2
#define LED_PIN 13  // LED indikator

// Definisikan frekuensi LoRa (512 MHz)
const unsigned long LORA_FREQUENCY = 410E6;

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  // Konfigurasi LED sebagai output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Awalnya LED mati

  // Inisialisasi LoRa
  Serial.println("Receiver Host");
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa Error");
    while (1)
      ;
  }
  LoRa.setSpreadingFactor(10);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(7);
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    Serial.print("Received Data: ");
    String receivedData = "";

    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.println(receivedData);

    // Nyalakan LED ketika menerima data
    digitalWrite(LED_PIN, HIGH);
    delay (3000);

    // Dapatkan kualitas sinyal
    float rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm, SNR: ");
    Serial.print(snr);
    Serial.println(" dB");

    // Menampilkan indikator kekuatan sinyal
    displaySignalStrength(rssi);

    // Parsing JSON untuk menampilkan frekuensi (jika data dalam format JSON)
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

  } else {
    // Jika tidak menerima data, matikan LED
    digitalWrite(LED_PIN, LOW);
  }
}

// Fungsi indikator kekuatan sinyal LoRa
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
