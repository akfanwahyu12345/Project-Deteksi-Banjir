// Sensor ultrasonik SR04M-2
#define TRIG_PIN 32              // Pin untuk TRIG
#define ECHO_PIN 35              // Pin untuk ECHO
const int SOUND_VELOCITY = 340;  // Kecepatan suara dalam m/s
const int MIN_CYCLE_DELAY = 60;  // Delay minimum antar pengukuran dalam ms

// Sensor tegangan
#define ANALOG_IN_PIN 27  // ESP32 pin GPIO27 (ADC0) connected to voltage sensor
#define REF_VOLTAGE 3.3
#define ADC_RESOLUTION 4096.0
#define R1 30000.0  // resistor values in voltage sensor (in ohms)
#define R2 7500.0   // resistor values in voltage sensor (in ohms)

// Sensor curah hujan
int Led = 13;       // nama alias pin 13 yaitu LED
int OutputDO = 34;  // nama alias pin 34 yaitu Output DO
int OutputAO = 33;  // nama alias pin 33 yaitu Output AO
int Buzzer = 26;    // buzzer

// Variabel untuk mengontrol delay buzzer
unsigned long previousMillis = 0;
const unsigned long buzzerInterval = 3000;  // Interval 3 detik

// Sensor aliran air
#define WATERFLOW_PIN 15   // Pin untuk sensor aliran air
volatile int flowCount = 0;  // Variabel untuk menghitung pulsa dari sensor aliran air
float flowRate = 0.0;   // Laju aliran air (liter per menit)
unsigned long lastTime = 0;
const float FLOW_CALIBRATION = 4.5;  // Faktor kalibrasi untuk sensor aliran air

void setup() {
  // Konfigurasi pin
  pinMode(Led, OUTPUT);
  pinMode(OutputDO, INPUT);
  pinMode(OutputAO, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(WATERFLOW_PIN, INPUT_PULLUP);  // Menggunakan pull-up resistor untuk sensor aliran air

  // Konfigurasi ADC
  analogSetAttenuation(ADC_11db);

  // Mengatur interupsi untuk sensor aliran air
  attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), countFlow, RISING);  // Menghitung pulsa saat sensor aliran air mendeteksi aliran

  // Inisialisasi serial
  Serial.begin(115200);
  Serial.println("K02 Sensor System Initialized");
}
// Fungsi untuk menghitung pulsa dari sensor aliran air
void countFlow() {
  flowCount++;  // Menambah jumlah pulsa saat aliran terdeteksi
}
// Fungsi pembacaan sensor curah hujan
void readRainSensor() {
  float sensorValue = analogRead(OutputAO);
  int digitalValue = digitalRead(OutputDO);

  if (digitalValue == LOW) {
    Serial.print("Nilai Sen-Hujan :");
    Serial.println(OutputAO);
    digitalWrite(Led, HIGH);
    Serial.println("Curah Hujan Tinggi");
    // Menyalakan buzzer setiap 3 detik sekali
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= buzzerInterval) {
      previousMillis = currentMillis;
      digitalWrite(Buzzer, HIGH);  // Nyalakan buzzer
      delay(500);                  // Buzzer menyala selama 1/2 detik
      digitalWrite(Buzzer, LOW);   // Matikan buzzer
      Serial.println("Buzzer Menyala");
    }
  } else {
    Serial.print("Nilai Sen-Hujan :");
    Serial.println(OutputAO);
    digitalWrite(Led, LOW);
    Serial.println("Curah Hujan Rendah");
    Serial.println("Buzzer Mati");
  }
}

// Fungsi pembacaan sensor tegangan
void readVoltageSensor() {
  int adcValue = analogRead(ANALOG_IN_PIN);
  float voltageADC = ((float)adcValue * REF_VOLTAGE) / ADC_RESOLUTION;
  float voltageIn = voltageADC * (R1 + R2) / R2;

  Serial.print("Tegangan = ");
  Serial.println(voltageIn, 2);
}

// Fungsi pembacaan sensor ultrasonik
void readUltrasonicSensor() {
  digitalWrite(TRIG_PIN, HIGH);  // Pastikan trigger LOW
  delayMicroseconds(1000);
  digitalWrite(TRIG_PIN, HIGH);  // Kirim sinyal trigger HIGH

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration / 2.0) * (SOUND_VELOCITY / 10000.0);  // Konversi ke cm

  if (distance >= 0 && distance <= 500) {  // Sensor memiliki blind zone 25cm
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("Out of range or below minimum measurable distance!");
  }
}

// Fungsi pembacaan sensor aliran air
void readWaterFlowSensor() {
  // Menghitung laju aliran setiap detik
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {
    // Hitung laju aliran (liter per menit)
    flowRate = (flowCount / FLOW_CALIBRATION);  // Faktor kalibrasi mungkin berbeda untuk sensor yang digunakan
    flowCount = 0;  // Reset jumlah pulsa untuk pengukuran berikutnya
    lastTime = currentTime;

    Serial.print("Laju Aliran Air: ");
    Serial.print(flowRate, 2);  // Menampilkan aliran dalam liter per menit
    Serial.println(" L/min");
  }
}

void loop() {
  // Panggil fungsi pembacaan masing-masing sensor
  Serial.println("===========================================");
  readRainSensor();
  readVoltageSensor();
  readUltrasonicSensor();
  readWaterFlowSensor();

  delay(2000);  // Waktu tunda 1 detik
}
