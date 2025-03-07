// Define pin connections
#define TRIG_PIN 32  // Pin untuk TRIG
#define ECHO_PIN 35  // Pin untuk ECHO

// Konstanta
const int SOUND_VELOCITY = 340;  // Kecepatan suara dalam m/s
const int MIN_CYCLE_DELAY = 60;  // Delay minimum antar pengukuran dalam ms

void setup() {
  // Konfigurasi pin
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  Serial.println("K02 Ultrasonic Sensor Initialized");
}

void loop() {
  // Step 1: Kirim sinyal trigger
  digitalWrite(TRIG_PIN, HIGH);  // Pastikan trigger LOW
  delayMicroseconds(1000);
  digitalWrite(TRIG_PIN, HIGH);  // Kirim sinyal trigger HIGH
  delayMicroseconds(1000);         // Tunggu minimal 10uS

  // Step 2: Baca sinyal echo
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Step 3: Hitung jarak
  float distance = (duration / 2.0) * (SOUND_VELOCITY / 10000.0);  // Konversi ke cm

  // Step 4: Tampilkan hasil
  if (distance >= 0 && distance <= 500) {  // Sensor memiliki blind zone 25cm
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("Out of range or below minimum measurable distance!");
  }

  // Step 5: Tunggu sebelum pengukuran berikutnya
  delay(MIN_CYCLE_DELAY);
}
