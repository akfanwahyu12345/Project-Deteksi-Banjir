#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>
//program inisialisasi sensor utama
#include <LoRa.h>
#include <ArduinoJson.h>
#include <Fuzzy.h>
#define SS 5
#define RST 15
#define DIO0 17
const int relay = 26;
//program inisialisasi wifi
const char* ssid = "akfan111";
const char* password = "12345678";
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
#define LED_BUILTIN 2
#define SENSOR 35

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

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
//webserver OTA
WebServer server(80);

void setup() {
  // Pin configurations
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  pinMode(ANALOG_IN_PIN, INPUT);
  pinMode(Led, OUTPUT);
  pinMode(OutputDO, INPUT);
  pinMode(OutputAO, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(Buzzer, OUTPUT);

  // ADC configurations tegangan sensor
  analogSetAttenuation(ADC_11db);

  // Set up water flow interrupts
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  // Fuzzy Setup - Create FuzzyInput and FuzzyOutput variables
  //sensorhujan
  rainInput.addFuzzySet(new FuzzySet(2800, 2801, 4095, 4095));  // aman
  rainInput.addFuzzySet(new FuzzySet(1800, 1800, 3069, 2800));  // waspada
  rainInput.addFuzzySet(new FuzzySet(800, 800, 2046, 2047));    // siaga
  rainInput.addFuzzySet(new FuzzySet(0, 0, 1022, 1023));        // bahaya

  //sensor ultrasonik
  heightInput.addFuzzySet(new FuzzySet(130, 131, 1000, 1000));  // aman
  heightInput.addFuzzySet(new FuzzySet(110, 120, 129, 130));    // waspada
  heightInput.addFuzzySet(new FuzzySet(100, 110, 109, 120));    // siaga
  heightInput.addFuzzySet(new FuzzySet(0, 85, 99, 100));        // bahaya

  //sensor waterflow/debit air
  flowInput.addFuzzySet(new FuzzySet(0, 0, 9, 10));       // aman
  flowInput.addFuzzySet(new FuzzySet(10, 15, 20, 25));    // waspada
  flowInput.addFuzzySet(new FuzzySet(25, 26, 28, 30));    // siaga
  flowInput.addFuzzySet(new FuzzySet(30, 31, 100, 100));  // bahaya

  //luaran resiko
  floodOutput.addFuzzySet(new FuzzySet(0, 0, 25, 50));        // aman
  floodOutput.addFuzzySet(new FuzzySet(25, 50, 75, 100));     // waspada
  floodOutput.addFuzzySet(new FuzzySet(75, 100, 125, 150));   // siaga
  floodOutput.addFuzzySet(new FuzzySet(125, 150, 175, 200));  // bahaya

  Serial.begin(9600);
  while (!Serial)
    ;
  // Serial.println("Sender Host");
  LoRa.setPins(SS, RST, DIO0);
  Serial.println("Lora aktif");
  if (!LoRa.begin(LORA_FREQUENCY)) {
    // Serial.println("LoRa Error");
    delay(100);
    while (1)
      ;
  }
  // LoRa.setTxPower(18, PA_OUTPUT_PA_BOOST_PIN);  // Daya 18 dBm
  // LoRa.setSpreadingFactor(10);                  // SF10
  // LoRa.setSignalBandwidth(125E3);               // Bandwidth 125 kHz
  // LoRa.setCodingRate4(7);                       // CR 4/7
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "Hello from ESP32!");
  });

  ElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
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
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");  // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");
  }


  // Store the data for trimmed mean calculation
  rainData[currentIndex] = rainValue;
  heightData[currentIndex] = distance;
  flowData[currentIndex] = flowRate;
  currentIndex = (currentIndex + 1) % NUM_SAMPLES;  // Wrap around to start of array when it's full

  // Fuzzy Logic Execution
  fuzzy.setInput(0, rainMM);    // Set rain input value (index 0 for rain input)
  fuzzy.setInput(1, distance);  // Set height input value (index 1 for height input)
  fuzzy.setInput(2, flowRate);  // Set flow input value (index 2 for flow input)

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
  Serial.print(" --, ");
  Serial.print("Ketinggian Air: ");
  Serial.print(trimmedHeight);
  Serial.print(" cm, ");
  Serial.print("Debit Air: ");
  Serial.print(trimmedFlow);
  Serial.print(" L/min, ");
  Serial.print("Risiko Banjir: ");
  Serial.print(floodRisk);
  Serial.print("Baterai: ");
  Serial.print(batteryLevel);
  Serial.print("%");

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
  delay(500);
}
float readUltrasonicSensor() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(1000);
  digitalWrite(TRIG_PIN, HIGH);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return ((duration / 2.0) * (SOUND_VELOCITY / 10000.0) + 4.5);  // Distance in cm
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
