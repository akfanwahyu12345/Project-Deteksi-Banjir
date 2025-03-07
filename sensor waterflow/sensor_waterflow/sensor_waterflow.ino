#include <LoRa.h>
#include <ArduinoJson.h>

// LoRa Pins
#define SS 5
#define RST 14
#define DIO0 2

const unsigned long LORA_FREQUENCY = 433E6;  // 433 MHz

// Water Flow Sensor
#define WATERFLOW_PIN 15
volatile int flowCount = 0;
float flowRate = 0.0;
unsigned long lastTime = 0;
const float FLOW_CALIBRATION = 4.5;

// Trimmed Mean Configuration
#define NUM_SAMPLES 10
float flowData[NUM_SAMPLES];
int currentIndex = 0;

// Function Prototype
float calculateTrimmedMean(float data[], int dataSize);
String evaluateFlowCondition(float flowRate);

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  Serial.println("Water Flow Sensor Sender");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa Initialization Failed!");
    while (1)
      ;
  }

  pinMode(WATERFLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), countFlow, RISING);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {
    flowRate = (flowCount / FLOW_CALIBRATION);
    flowCount = 0;
    lastTime = currentTime;

    flowData[currentIndex] = flowRate;
    currentIndex = (currentIndex + 1) % NUM_SAMPLES;

    float trimmedFlow = calculateTrimmedMean(flowData, NUM_SAMPLES);
    String condition = evaluateFlowCondition(trimmedFlow);

    StaticJsonDocument<256> jsonDoc;
    jsonDoc["sensor_id"] = "flow";
    jsonDoc["value"] = trimmedFlow;
    jsonDoc["condition"] = condition;
    jsonDoc["unit"] = "L/min";

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    LoRa.beginPacket();
    LoRa.print(jsonString);
    LoRa.endPacket();

    Serial.print("Sending Flow Data: ");
    Serial.println(jsonString);
  }
}

void countFlow() {
  flowCount++;
}

float calculateTrimmedMean(float data[], int dataSize) {
  for (int i = 0; i < dataSize - 1; i++) {
    for (int j = i + 1; j < dataSize; j++) {
      if (data[i] > data[j]) {
        float temp = data[i];
        data[i] = data[j];
        data[j] = temp;
      }
    }
  }
  int trimCount = dataSize / 10;
  if (trimCount < 1) trimCount = 1;
  int newSize = dataSize - 2 * trimCount;
  float sum = 0;
  for (int i = trimCount; i < newSize + trimCount; i++) {
    sum += data[i];
  }
  return sum / newSize;
}

String evaluateFlowCondition(float flowRate) {
  if (flowRate < 10) {
    return "Aman";
  } else if (flowRate >= 10 && flowRate < 20) {
    return "Siaga";
  } else if (flowRate >= 20 && flowRate < 30) {
    return "Waspada";
  } else {
    return "Bahaya";
  }
}
