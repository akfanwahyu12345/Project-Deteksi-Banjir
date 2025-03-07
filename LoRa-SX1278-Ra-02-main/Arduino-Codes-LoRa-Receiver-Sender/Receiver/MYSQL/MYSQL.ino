#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi
const char* ssid = "KOTA MALANG";  // Ganti dengan SSID WiFi Anda
const char* password = "nasibayam";  // Ganti dengan password WiFi Anda

// Konfigurasi server PHP
const char* serverUrl = "http://127.0.0.1/sensor_sungai/device_data_streams.php"; // Ganti dengan alamat server Anda

// Pin konfigurasi LoRa
#define SS 18
#define RST 14
#define DIO0 26

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Receiver dengan HTTP POST");

    // Koneksi ke WiFi
    WiFi.begin(ssid, password);
    Serial.print("Menghubungkan ke WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Terhubung!");

    // Inisialisasi LoRa
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(915E6)) { // Sesuaikan frekuensi dengan modul Anda
        Serial.println("Gagal menginisialisasi LoRa");
        while (1);
    }

    Serial.println("LoRa berhasil diinisialisasi");
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String receivedData = "";
        while (LoRa.available()) {
            receivedData += (char)LoRa.read();
        }

        Serial.print("Data diterima: ");
        Serial.println(receivedData);

        // Parsing JSON
        StaticJsonDocument<512> jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, receivedData);

        if (error) {
            Serial.print("Error parsing JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Ekstrak data dari JSON
        int device_id = jsonDoc["device_id"];
        float debit_air = jsonDoc["debit_air"];
        const char* debit_air_status = jsonDoc["debit_air_status"];
        float tinggi_air = jsonDoc["tinggi_air"];
        const char* tinggi_air_status = jsonDoc["tinggi_air_status"];
        float curah_hujan = jsonDoc["curah_hujan"];
        const char* curah_hujan_status = jsonDoc["curah_hujan_status"];
        float battery = jsonDoc["battery"];

        // Kirim data ke server
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(serverUrl);
            http.addHeader("Content-Type", "application/json");

            // Buat JSON untuk dikirim
            StaticJsonDocument<512> postData;
            postData["device_id"] = device_id;
            postData["debit_air"] = debit_air;
            postData["debit_air_status"] = debit_air_status;
            postData["tinggi_air"] = tinggi_air;
            postData["tinggi_air_status"] = tinggi_air_status;
            postData["curah_hujan"] = curah_hujan;
            postData["curah_hujan_status"] = curah_hujan_status;
            postData["battery"] = battery;

            String jsonString;
            serializeJson(postData, jsonString);

            // Kirim data ke server
            int httpResponseCode = http.POST(jsonString);
            Serial.print("HTTP Response Code: ");
            Serial.println(httpResponseCode);
            Serial.println(http.getString());

            http.end();
        } else {
            Serial.println("Gagal menghubungi server, WiFi tidak terhubung");
        }
    }
}
