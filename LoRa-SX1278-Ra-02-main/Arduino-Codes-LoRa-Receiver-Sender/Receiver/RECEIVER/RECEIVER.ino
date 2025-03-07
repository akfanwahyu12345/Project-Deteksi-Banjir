#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "KOTA MALANG";  // Ganti dengan SSID WiFi Anda
const char* password = "nasibayam";  // Ganti dengan password WiFi Anda
const char* serverUrl = "http://192.168.100.12:8000/sensor_sungai-main/insert.php"; // Laravel API

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    Serial.print("Menghubungkan ke WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Terhubung!");
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<512> jsonDoc;
        jsonDoc["device_id"] = 1;
        jsonDoc["debit_air"] = 50.5;
        jsonDoc["debit_air_status"] = "Normal";
        jsonDoc["tinggi_air"] = 30.2;
        jsonDoc["tinggi_air_status"] = "Aman";
        jsonDoc["curah_hujan"] = 12.3;
        jsonDoc["curah_hujan_status"] = "Ringan";
        jsonDoc["battery"] = 85;

        String jsonString;
        serializeJson(jsonDoc, jsonString);

        Serial.println("\n🌍 Mengirim Data ke Laravel...");
        Serial.println(jsonString);

        int httpResponseCode = http.POST(jsonString);
        Serial.print("📩 HTTP Response Code: ");
        Serial.println(httpResponseCode);
        Serial.println("📜 Respon Server: ");
        Serial.println(http.getString());

        http.end();
    } else {
        Serial.println("❌ WiFi tidak terhubung!");
    }
    http.begin(serverUrl);
    http.GET();

    delay(5000);
}
