#include <Arduino.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <Fuzzy.h>
#include <FuzzyRule.h>
#include <FuzzyComposition.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyOutput.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzySet.h>

// Pin Definitions
#define RAIN_SENSOR_PIN 32
#define TRIG_PIN 33
#define ECHO_PIN 25
#define WATERFLOW_PIN 35
#define LORA_SS 5
#define LORA_RST 15
#define LORA_DIO0 17

// Constants for LoRa
const unsigned long LORA_FREQUENCY = 433E6;  // 433 MHz

// Constants for sensor processing
const int SOUND_VELOCITY = 340;  // Speed of sound in m/s
const float FLOW_CALIBRATION = 4.5;

// Fuzzy Logic System
Fuzzy fuzzy;
FuzzyInput rainInput(1);
FuzzyInput heightInput(2);
FuzzyInput flowInput(3);
FuzzyOutput floodRiskOutput(4);

void setupFuzzy() {
  // Rain Sensor Fuzzy Sets
  FuzzySet *bahaya_hujan = new FuzzySet(0, 0, 1022, 1023);
  FuzzySet *siaga_hujan = new FuzzySet(800, 800, 2046, 2047);
  FuzzySet *waspada_hujan = new FuzzySet(1800, 1800, 3069, 3071);
  FuzzySet *aman_hujan = new FuzzySet(2800, 2801, 4095, 4095);

  rainInput.addFuzzySet(bahaya_hujan);
  rainInput.addFuzzySet(siaga_hujan);
  rainInput.addFuzzySet(waspada_hujan);
  rainInput.addFuzzySet(aman_hujan);
  fuzzy.addFuzzyInput(&rainInput);

  // Ultrasonic Sensor Fuzzy Sets
  FuzzySet *aman_airmeluap = new FuzzySet(130, 131, 1000, 1000);
  FuzzySet *waspada_airmeluap = new FuzzySet(110, 120, 129, 130);
  FuzzySet *siaga_airmeluap = new FuzzySet(100, 110, 109, 120);
  FuzzySet *bahaya_airmeluap = new FuzzySet(0, 85, 99, 100);

  heightInput.addFuzzySet(aman_airmeluap);
  heightInput.addFuzzySet(waspada_airmeluap);
  heightInput.addFuzzySet(siaga_airmeluap);
  heightInput.addFuzzySet(bahaya_airmeluap);
  fuzzy.addFuzzyInput(&heightInput);

  // Waterflow Sensor Fuzzy Sets
  FuzzySet *aman_debitair = new FuzzySet(0, 0, 9, 10);
  FuzzySet *waspada_debitair = new FuzzySet(10, 10, 29, 30);
  FuzzySet *siaga_debitair = new FuzzySet(20, 20, 29, 30);
  FuzzySet *bahaya_debitair = new FuzzySet(30, 31, 100, 100);

  flowInput.addFuzzySet(aman_debitair);
  flowInput.addFuzzySet(waspada_debitair);
  flowInput.addFuzzySet(siaga_debitair);
  flowInput.addFuzzySet(bahaya_debitair);
  fuzzy.addFuzzyInput(&flowInput);

  // // Define the output risk levels
  // FuzzySet *lowRisk = new FuzzySet(0, 25, 50, 75);
  // FuzzySet *mediumRisk = new FuzzySet(50, 60, 70, 80);
  // FuzzySet *highRisk = new FuzzySet(70, 80, 90, 100);

  // floodRiskOutput.addFuzzySet(lowRisk);
  // floodRiskOutput.addFuzzySet(mediumRisk);
  // floodRiskOutput.addFuzzySet(highRisk);
  // fuzzy.addFuzzyOutput(&floodRiskOutput);
  // Define the output risk levels for flood risk
  FuzzySet *aman = new FuzzySet(0, 0, 25, 50);          // Aman
  FuzzySet *waspada = new FuzzySet(25, 50, 75, 100);      // Waspada
  FuzzySet *siaga = new FuzzySet(75, 100, 125, 150);  // Siaga
  FuzzySet *bahaya = new FuzzySet(125, 150, 175, 200);  // Bahaya

  floodRiskOutput.addFuzzySet(aman);
  floodRiskOutput.addFuzzySet(waspada);
  floodRiskOutput.addFuzzySet(siaga);
  floodRiskOutput.addFuzzySet(bahaya);
  fuzzy.addFuzzyOutput(&floodRiskOutput);
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(WATERFLOW_PIN, INPUT_PULLUP);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }

  setupFuzzy();
}

void loop() {
  int rainValue = analogRead(RAIN_SENSOR_PIN);
  float distance = readUltrasonicDistance();
  float flowRate = calculateFlowRate();

  // Apply fuzzy inputs
  fuzzy.setInput(1, rainValue);
  fuzzy.setInput(2, distance);
  fuzzy.setInput(3, flowRate);

  // Perform fuzzy calculation
  fuzzy.fuzzify();

  float floodRisk = fuzzy.defuzzify(4);

  // Prepare data for transmission
  StaticJsonDocument<512> doc;
  doc["rain"] = rainValue;
  doc["distance"] = distance;
  doc["flowRate"] = flowRate;
  doc["floodRisk"] = floodRisk;

  // Serialize JSON to send over LoRa
  String jsonString;
  serializeJson(doc, jsonString);
  LoRa.beginPacket();
  LoRa.print(jsonString);
  LoRa.endPacket();

  Serial.println(jsonString);  // Debug output to serial
  delay(2000);                 // Wait for 2 seconds before next loop
}

float readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return (duration / 2.0) * (SOUND_VELOCITY / 10000.0);  // Convert to distance
}

float calculateFlowRate() {
  static unsigned long lastMillis = 0;
  static int pulseCount = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis > 1000) {
    lastMillis = currentMillis;
    float rate = pulseCount / FLOW_CALIBRATION;
    pulseCount = 0;  // Reset the count
    return rate;
  }
  return 0;
}

void ISR_flowPulse() {
  pulseCount++;  // Increment the pulse count
}
