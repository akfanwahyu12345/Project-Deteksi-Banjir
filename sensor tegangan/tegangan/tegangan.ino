const int numReadings = 27;
int readings[numReadings];      // Array untuk menyimpan pembacaan
int readIndex = 0;              // Index pembacaan saat ini
float total = 0;                // Total pembacaan
float average = 0;              // Rata-rata pembacaan
float voltage_sensor = 0;

void setup() {
  Serial.begin(9600);
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;  // Inisialisasi array
  }
}

void loop() {
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(voltage_sensor);
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  average = total / numReadings;
  float voltage = (average * 2.5) / 1023;  // Konversi ke volt

  Serial.print("Nilai Tegangan: ");
  Serial.println(voltage);
  delay(1000);
}
