#include <Arduino.h>

// Definisi pin
int port_sensor_hujan = 33;

// Setup
void setup() {
  Serial.begin(9600);
  pinMode(port_sensor_hujan, INPUT);
}

// Loop
void loop() {
  int sensor_hujan = analogRead(port_sensor_hujan);
  String status = "";

  // Logika fuzzy untuk menentukan status berdasarkan range nilai yang disesuaikan
  if (sensor_hujan < 1023) {
    status = "Bahaya";
  } else if (sensor_hujan >= 800 && sensor_hujan < 2047) {
    status = "Waspada";
  } else if (sensor_hujan >= 1800 && sensor_hujan < 3071) {
    status = "Siaga";
  } else if (sensor_hujan >= 2800) {
    status = "Aman";
  }

  // Cetak status ke Serial Monitor
  Serial.print("Nilai Sensor: ");
  Serial.print(sensor_hujan);
  Serial.print(" - Status: ");
  Serial.println(status);

  delay(1500);
}
