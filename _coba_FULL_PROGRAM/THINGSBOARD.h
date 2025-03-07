#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>

#include <WiFi.h>
#include <HTTPClient.h>
//const char *url = "http://demo.thingsboard.io/api/v1/t2kt1QuyO4zMMUbTbaRS/telemetry"; //http://demo.thingsboard.io/api/v1/ri379gqpxrwt767b6qep/telemetry

String payloadAN;
void aturAssistNow();

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("Chandra's Skripsi");
  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

  // Inisialisasi GPS
  //  Serial.println("Mengatur AssistNow Online...");
  //  aturAssistNow();
}

void kirimData(double lat, double lng, bool geofence, String Pengguna, bool statHB, String persenBat) {
  WiFiClient client;
  HTTPClient http;
  String server = "http://demo.thingsboard.io/api/v1/t2kt1QuyO4zMMUbTbaRS/telemetry"; //http://demo.thingsboard.io/api/v1/yqlVrHDSfAHd864Bekkp/telemetry

  http.addHeader("Content-Type", "application/json");
  http.begin(client, server);

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["latitude"] = String(lat, 8);
  doc["longitude"] = String(lng, 8);
  doc["isInside"] = geofence == true ? "in" : "out";
  doc["user"] = Pengguna;
  doc["status"] = statHB == true ? "on" : "off";
  doc["batt"] = persenBat;

  size_t n = serializeJson(doc, buffer);
  int rc = http.POST(buffer);
  
  http.end(); // Ensure http.end() is called
}

/*
  int kirimData(float lat, float lng, bool geofence, String Pengguna, bool statHB, int persenBat)
  {
  // Check Wi-Fi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    return -1;
  }
  String Geofence, StatHB;
  if(geofence) Geofence = "in"; else Geofence = "out";
  if(statHB) StatHB = "on"; else StatHB = "off";
  String jsonData = "{\"latitude\": " + String(lat, 6) + ", \"longitude\": " + String(lng, 6) + ", \"geofence\": " + Geofence + ", \"Pengguna\": " + Pengguna + ", \"statHB\": " + StatHB + ", \"Baterai\": " + String(persenBat) + "}";
  Serial.println(jsonData);

  HTTPClient http;
  String server = "http://demo.thingsboard.io/api/v1/Jlc6I5D2gaM0vcodc2Ut/telemetry";
  http.begin(server);
  http.addHeader("Content-Type", "application/json");

  // Make the HTTP POST request
  int httpResponseCode = http.POST(jsonData);

  // Check for a successful response
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("HTTP Response: " + response);
    Serial.println(httpResponseCode);
    return httpResponseCode; // Successfully sent telemetry data
  }
  else
  {
    Serial.println("HTTP POST failed");
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    return -2; // Failed to send telemetry data
  }

  // Close the HTTP connection
  http.end();
  }
*/

void aturAssistNow() {
  HTTPClient http;
  // Alamat server AssistNow Online
  String url = "http://online-live1.services.u-blox.com/GetOnlineData.ashx?token=pi9b0Ln9TiyRvq4jLqD7RA;format=mga;gnss=gps,glo;datatype=eph,alm,aux,pos;pacc=300000";

  // Mulai permintaan HTTP
  http.begin(url);

  // Kirim permintaan GET dan tunggu respons
  int httpCode = http.GET();

  if (httpCode > 0) {
    // Respons berhasil diterima
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Data AssistNow Online berhasil diterima.");

      // Baca dan tampilkan data dari server
      payloadAN = http.getString();
      Serial.println(payloadAN);

    }
  } else {
    // Gagal terhubung ke server
    Serial.println("Gagal terhubung ke server AssistNow Online.");
  }

  // Akhiri sesi HTTP
  http.end();
}
