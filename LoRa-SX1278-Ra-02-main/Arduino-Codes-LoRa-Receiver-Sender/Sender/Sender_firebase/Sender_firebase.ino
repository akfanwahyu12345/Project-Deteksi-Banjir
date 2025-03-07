#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "KOTA MALANG"
#define WIFI_PASSWORD "nasibayam"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBDEm3nrxZVU3j7I_VMNwzb1KSlpXaBOh4"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://deteksi-banjir-bdf54-default-rtdb.firebaseio.com/"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

#include <LoRa.h>
#include <ArduinoJson.h>
#include <Fuzzy.h>
volatile int flow_frequency;  // Measures flow sensor pulses
#define SS 5
#define RST 15
#define DIO0 17
const int relay = 26;

const unsigned long LORA_FREQUENCY = 433E6;  // 433 MHz
String data = "makan";

// Sensor Ultrasonik SR04M-2
#define TRIG_PIN 33
#define ECHO_PIN 25
const int SOUND_VELOCITY = 340;
const int MIN_CYCLE_DELAY = 60;

// Sensor Tegangan
#define ANALOG_IN_PIN 27
// #define REF_VOLTAGE 3.3
#define ADC_RESOLUTION 4096.0
#define R1 30000.0
#define R2 7500.0
float ref_voltage = 12.8;
float faktorTrim = 0.20;  // 5%-20%
int jumlahSampel = 50;
float maxV = 14.8;  // maksimum tegangan baterai
float minV = 3.7;   // minimum tegangan baterai

// Sensor Curah Hujan
int Led = 13;
int OutputDO = 34;
int OutputAO = 32;
int Buzzer = 26;

// Sensor Aliran Air
#define WATERFLOW_PIN 35
float vol = 0.0, l_minute;
unsigned char flowsensor = 34;  // Sensor Input
unsigned long currentTime;
unsigned long cloopTime;
float volume_per_pulse = 0.0;

// Fuzzy Logic
Fuzzy fuzzy;
FuzzyInput rainInput;
FuzzyInput heightInput;
FuzzyInput flowInput;
FuzzyOutput floodOutput;

// Array for storing sensor data for trimmed mean calculation
#define NUM_SAMPLES 10
float rainData[NUM_SAMPLES];
float heightData[NUM_SAMPLES];
float flowData[NUM_SAMPLES];
int currentIndex = 0;

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  pinMode(ANALOG_IN_PIN, INPUT);
  pinMode(Led, OUTPUT);
  pinMode(OutputDO, INPUT);
  pinMode(OutputAO, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(WATERFLOW_PIN, INPUT_PULLUP);

  // ADC configurations tegangan sensor
  analogSetAttenuation(ADC_11db);

  // Set up water flow interrupts
  // attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), countFlow, RISING);
  attachInterrupt(digitalPinToInterrupt(flowsensor), flow, RISING);  // Setup Interrupt
  currentTime = millis();
  cloopTime = currentTime;
  // Calibrate and set the value of volume_per_pulse here
  volume_per_pulse = 0.004;

  // Fuzzy Setup - Create FuzzyInput and FuzzyOutput variables
  //sensorhujan
  rainInput.addFuzzySet(new FuzzySet(2800, 2801, 4095, 4095));  // aman
  rainInput.addFuzzySet(new FuzzySet(1800, 1800, 3069, 3071));  // waspada
  rainInput.addFuzzySet(new FuzzySet(800, 800, 2046, 2047));    // siaga
  rainInput.addFuzzySet(new FuzzySet(0, 0, 1022, 1023));        // bahaya

  //sensor ultrasonik
  heightInput.addFuzzySet(new FuzzySet(130, 131, 1000, 1000));  // aman
  heightInput.addFuzzySet(new FuzzySet(110, 120, 129, 130));    // waspada
  heightInput.addFuzzySet(new FuzzySet(100, 110, 109, 120));    // siaga
  heightInput.addFuzzySet(new FuzzySet(0, 85, 99, 100));        // bahaya

  //sensor waterflow
  flowInput.addFuzzySet(new FuzzySet(0, 0, 9, 10));       // aman
  flowInput.addFuzzySet(new FuzzySet(10, 10, 29, 30));    // waspada
  flowInput.addFuzzySet(new FuzzySet(20, 20, 29, 30));    // siaga
  flowInput.addFuzzySet(new FuzzySet(30, 31, 100, 100));  // bahaya

  //luaran resiko
  floodOutput.addFuzzySet(new FuzzySet(0, 0, 25, 50));        // aman
  floodOutput.addFuzzySet(new FuzzySet(25, 50, 75, 100));     // waspada
  floodOutput.addFuzzySet(new FuzzySet(75, 100, 125, 150));   // siaga
  floodOutput.addFuzzySet(new FuzzySet(125, 150, 175, 200));  // bahaya

  while (!Serial)
    ;
  // Serial.println("Sender Host");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    // Serial.println("LoRa Error");
    delay(100);
    while (1)
      ;
  }
}

void loop() {
  digitalWrite(relay, LOW);
  // Reading sensors
  // Serial.println("===========================================");
  // sensor tegangan
  int batteryLevel = persenBAT();

  // Rain Sensor
  int rainValue = analogRead(OutputAO);            // Assumed range 0 - 1023
  float rainMM = map(rainValue, 0, 1023, 0, 100);  // Convert to mm

  // Water Height Sensor
  float distance = readUltrasonicSensor();  // In cm

  // Flow Sensor
  // unsigned long currentTime = millis();
  // if (currentTime - lastTime >= 1000) {
  //   flowRate = (flowCount / FLOW_CALIBRATION);  // L/min
  //   flowCount = 0;                              // Reset pulse count
  //   lastTime = currentTime;
  // }
  currentTime = millis();
  // Every second, calculate and print litres/hour
  if (currentTime >= (cloopTime + 1000)) {
    cloopTime = currentTime;  // Updates cloopTime
    if (flow_frequency != 0) {
      // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
      l_minute = (flow_frequency * volume_per_pulse * 60);
      // Serial.print("Rate: ");
      // Serial.print(l_minute);
      // Serial.println(" L/M");

      vol = vol + l_minute / 60;
      // Serial.print("Vol:");
      // Serial.print(vol);
      // Serial.println(" L, ");
      // Serial.print("Vol:");
      // Serial.print(vol * 1000);
      // Serial.println(" mL");
      flow_frequency = 0;  // Reset Counter
    } else {
      // Serial.println("Rate:0 L/M ");
    }
  }


  // Store the data for trimmed mean calculation
  rainData[currentIndex] = rainValue;
  heightData[currentIndex] = distance;
  flowData[currentIndex] = l_minute;
  currentIndex = (currentIndex + 1) % NUM_SAMPLES;  // Wrap around to start of array when it's full

  // Fuzzy Logic Execution
  fuzzy.setInput(0, rainMM);    // Set rain input value (index 0 for rain input)
  fuzzy.setInput(1, distance);  // Set height input value (index 1 for height input)
  fuzzy.setInput(2, l_minute);  // Set flow input value (index 2 for flow input)

  fuzzy.fuzzify();  // Fuzzify inputs

  // Get flood risk output
  float floodRisk = floodOutput.getCrispOutput();
  float floodRiskrain = rainInput.getCrispInput();
  float floodRiskheight = heightInput.getCrispInput();
  float floodRiskflow = flowInput.getCrispInput();

  // Calculate Trimmed Mean for the three sensors
  float trimmedRain = calculateTrimmedMean(rainData, NUM_SAMPLES);
  float trimmedHeight = calculateTrimmedMean(heightData, NUM_SAMPLES);
  float trimmedFlow = calculateTrimmedMean(flowData, NUM_SAMPLES);

  // Displaying results
  // Serial.print("Curah Hujan: ");
  Serial.println(rainValue);
  // Serial.print(" --, ");
  // Serial.print("Ketinggian Air: ");
  // Serial.print(trimmedHeight);
  // Serial.print(" cm, ");
  // Serial.print("Debit Air: ");
  // Serial.print(trimmedFlow);
  // Serial.print(" L/min, ");
  // Serial.print("Risiko Banjir: ");
  // Serial.print(floodRisk);
  // Serial.print("Baterai: ");
  // Serial.print(batteryLevel);
  // Serial.print("%");

  // Flood Risk Category based on fuzzy output
  // if (floodRisk <= 25) {
  //   Serial.println("Rendah");
  // } else if (floodRisk <= 70) {
  //   Serial.println("Sedang");
  // } else {
  //   Serial.println("Tinggi");
  // }
  // Flood Risk Category based on fuzzy output
  if (floodRisk <= 50) {
    // Serial.println("Aman");  // Menunjukkan tidak ada atau risiko sangat rendah
    digitalWrite(Led, LOW);  // LED OFF
    digitalWrite(Buzzer, LOW);
  } else if (floodRisk <= 100) {
    // Serial.println("Waspada");  // Risiko sedang, tindakan pencegahan mungkin diperlukan
    digitalWrite(Led, LOW);  // LED OFF
    digitalWrite(Buzzer, LOW);
  } else if (floodRisk <= 150) {
    // Serial.println("Siaga");  // Risiko tinggi, siap untuk tindakan evakuasi atau lainnya
    digitalWrite(Led, LOW);  // LED OFF
    digitalWrite(Buzzer, LOW);
  } else {
    // Serial.println("Bahaya");    // Risiko sangat tinggi, tindakan segera diperlukan
    digitalWrite(Led, HIGH);     // LED ON
    digitalWrite(Buzzer, HIGH);  // Buzzer ON
  }

  // // Flood Risk Detection and Warning
  // if (floodRisk > 70) {
  //   digitalWrite(Led, HIGH);     // LED ON
  //   digitalWrite(Buzzer, HIGH);  // Buzzer ON
  // } else {
  //   digitalWrite(Led, LOW);     // LED OFF
  //   digitalWrite(Buzzer, LOW);  // Buzzer OFF
  // }

  // Membuat string data untuk dikirim melalui LoRa
  // String sendData = "Curah Hujan: " + String(trimmedRain) + " mm, " + "Ketinggian Air: " + String(trimmedHeight) + " cm, " + "Debit Air: " + String(trimmedFlow) + " L/min, " + "Risiko Banjir: " + String(floodRisk);
  // Membuat string data untuk dikirim melalui LoRa
  // String sendData = "Curah Hujan: " + String(trimmedRain) + " mm, " + "Ketinggian Air: " + String(trimmedHeight) + " cm, " + "Debit Air: " + String(trimmedFlow) + " L/min, " + "Risiko Banjir: " + String(floodRisk)+ "#";

  // Membuat objek JSON
  StaticJsonDocument<512> jsonDoc;  // Buffer untuk JSON
  JsonArray jsonArray = jsonDoc.to<JsonArray>();

  // Menambahkan data sensor ke array JSON
  JsonObject bat = jsonArray.createNestedObject();
  bat["sensor"] = "Baterai Level";
  bat["value"] = batteryLevel;
  bat["unit"] = "%";

  JsonObject rain = jsonArray.createNestedObject();
  rain["sensor"] = "Curah Hujan";
  rain["value"] = trimmedRain;
  rain["unit"] = "mm";

  JsonObject height = jsonArray.createNestedObject();
  height["sensor"] = "Ketinggian Air";
  height["value"] = trimmedHeight;
  height["unit"] = "cm";

  JsonObject flow = jsonArray.createNestedObject();
  flow["sensor"] = "Debit Air";
  flow["value"] = trimmedFlow;
  flow["unit"] = "L/min";

  JsonObject floadrain = jsonArray.createNestedObject();
  floadrain["sensor"] = "Curah Hujan";
  floadrain["value"] = floodRiskrain;  // Use previously calculated float floodRiskrain
  floadrain["category"] = (floodRiskrain >= 2800) ? "T" : (floodRiskrain >= 1800 && floodRiskrain <= 2800) ? "R"
                                                        : (floodRiskrain >= 800 && floodRiskrain <= 1800)  ? "Sd"
                                                                                                           : "L";

  JsonObject floadheight = jsonArray.createNestedObject();
  floadheight["sensor"] = "Kondisi Air";
  floadheight["value"] = floodRiskheight;  // Use previously calculated float floodRiskheight
  floadheight["category"] = (floodRiskheight >= 130) ? "R" : (floodRiskheight >= 110 && floodRiskheight <= 130) ? "Sd"
                                                           : (floodRiskheight >= 100 && floodRiskheight <= 110) ? "T"
                                                                                                                : "ST";

  JsonObject floadflow = jsonArray.createNestedObject();
  floadflow["sensor"] = "Arus Air Sungai";
  floadflow["value"] = floodRiskflow;  // Use previously calculated float floodRiskflow
  floadflow["category"] = (floodRiskflow <= 10) ? "Lm" : (floodRiskflow > 10 && floodRiskflow <= 30) ? "Sd"
                                                       : (floodRiskflow > 30 && floodRiskflow <= 50) ? "Cp"
                                                                                                     : "SL";

  JsonObject floodRiskJson = jsonArray.createNestedObject();
  floodRiskJson["sensor"] = "Risiko Banjir";
  floodRiskJson["value"] = floodRisk;  // Gunakan float floodRisk yang dihitung sebelumnya
  floodRiskJson["category"] = (floodRisk <= 50) ? "Aman" : (floodRisk <= 100) ? "Waspada"
                                                         : (floodRisk <= 150) ? "Siaga"
                                                                              : "Bahaya";
  // Tambahkan frekuensi LoRa ke JSON
  JsonObject frequencyJson = jsonArray.createNestedObject();
  frequencyJson["sensor"] = "Frekuensi LoRa";
  frequencyJson["value"] = LORA_FREQUENCY / 1E6;  // Convert to MHz
  frequencyJson["unit"] = "MHz";

  // Serialisasi JSON menjadi string
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Serial.print("Sending Data: ");
  // Serial.println(jsonString);
  LoRa.beginPacket();
  LoRa.print(jsonString);
  LoRa.endPacket();

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int baterai
    if (Firebase.RTDB.setInt(&fbdo, "test/int", batteryLevel)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    // count++;

    // Write an Float number on the database path test/float curah hujan
    if (Firebase.RTDB.setFloat(&fbdo, "test/float", trimmedRain)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float ketinggian air
    if (Firebase.RTDB.setFloat(&fbdo, "test/float1", trimmedHeight)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float debit air
    if (Firebase.RTDB.setFloat(&fbdo, "test/float2", trimmedFlow)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    // Write an Float number on the database path test/float frekuensi lora
    if (Firebase.RTDB.setFloat(&fbdo, "test/float3", LORA_FREQUENCY)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, "test/sensors", jsonString)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  delay(500);
}


float readUltrasonicSensor() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(1000);
  digitalWrite(TRIG_PIN, HIGH);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return (duration / 2.0) * (SOUND_VELOCITY / 10000.0);  // Distance in cm
}

void flow() {
  flow_frequency++;
}

float readBatteryVoltage() {
  int adc_value = analogRead(ANALOG_IN_PIN);
  float adc_voltage = (adc_value * ref_voltage) / 1023.0;  // Convert ADC value to voltage
  float in_voltage = adc_voltage * ((R1 + R2) / R2);       // Calculate input voltage based on voltage divider
  return in_voltage;
}

int persenBAT() {
  float nilaiSensor[jumlahSampel];

  for (int i = 0; i < jumlahSampel; i++) {
    nilaiSensor[i] = readBatteryVoltage();
  }

  // Sorting
  for (int i = 0; i < jumlahSampel - 1; i++) {
    for (int j = i + 1; j < jumlahSampel; j++) {
      if (nilaiSensor[i] > nilaiSensor[j]) {
        float temp = nilaiSensor[i];
        nilaiSensor[i] = nilaiSensor[j];
        nilaiSensor[j] = temp;
      }
    }
  }

  // Trimmed mean
  float sum = 0;
  int validCount = jumlahSampel * (1 - faktorTrim);
  for (int i = jumlahSampel * (faktorTrim / 2); i < jumlahSampel * (1 - (faktorTrim / 2)); i++) {
    sum += nilaiSensor[i];
  }

  float meanVoltage = sum / validCount;
  int batteryPercent = map(meanVoltage * 100, minV * 100, maxV * 100, 0, 100);

  return batteryPercent;
}

// Function to calculate trimmed mean
float calculateTrimmedMean(float data[], int dataSize) {
  // Sort the data array
  float temp;
  for (int i = 0; i < dataSize - 1; i++) {
    for (int j = i + 1; j < dataSize; j++) {
      if (data[i] > data[j]) {
        // Swap the elements
        temp = data[i];
        data[i] = data[j];
        data[j] = temp;
      }
    }
  }

  // Remove 10% smallest and largest values
  //menghilangkan 10% data dari ujung data sehingga 20% data extrem dihilangkan dari kalkulasi rata rata
  //digunakan untuk dataset dengan sedikit atau sedang fluktuasi. Ini membantu menghilangkan outlier tanpa kehilangan terlalu banyak data.
  int trimCount = dataSize / 10;
  int newSize = dataSize - 2 * trimCount;

  // Sum the remaining values
  float sum = 0;
  for (int i = trimCount; i < newSize + trimCount; i++) {
    sum += data[i];
  }

  // Return the average
  return sum / newSize;
}

// Fungsi untuk menampilkan indikator kekuatan sinyal di Sender
void displaySignalStrength(float rssi) {
  Serial.print("Signal Strength from Receiver: ");
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