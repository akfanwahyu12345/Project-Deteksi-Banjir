#include <Fuzzy.h>

// Sensor Ultrasonik SR04M-2
#define TRIG_PIN 32
#define ECHO_PIN 35
const int SOUND_VELOCITY = 340;
const int MIN_CYCLE_DELAY = 60;

// Sensor Tegangan
#define ANALOG_IN_PIN 27
#define REF_VOLTAGE 3.3
#define ADC_RESOLUTION 4096.0
#define R1 30000.0
#define R2 7500.0

// Sensor Curah Hujan
int Led = 13;
int OutputDO = 34;
int OutputAO = 33;
int Buzzer = 26;

// Sensor Aliran Air
#define WATERFLOW_PIN 15
volatile int flowCount = 0;
float flowRate = 0.0;
unsigned long lastTime = 0;
const float FLOW_CALIBRATION = 4.5;

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
  // Pin configurations
  pinMode(Led, OUTPUT);
  pinMode(OutputDO, INPUT);
  pinMode(OutputAO, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(WATERFLOW_PIN, INPUT_PULLUP);

  // ADC configurations
  analogSetAttenuation(ADC_11db);

  // Set up water flow interrupts
  attachInterrupt(digitalPinToInterrupt(WATERFLOW_PIN), countFlow, RISING);

  // Initialize Serial
  Serial.begin(115200);
  Serial.println("K02 Sensor System Initialized");

  // Fuzzy Setup - Create FuzzyInput and FuzzyOutput variables
  rainInput.addFuzzySet(new FuzzySet(0, 20, 30, 40));     // Low
  rainInput.addFuzzySet(new FuzzySet(30, 50, 60, 70));    // Medium
  rainInput.addFuzzySet(new FuzzySet(60, 80, 100, 100));  // High

  heightInput.addFuzzySet(new FuzzySet(0, 50, 100, 150));     // Low
  heightInput.addFuzzySet(new FuzzySet(100, 200, 250, 300));  // Medium
  heightInput.addFuzzySet(new FuzzySet(250, 400, 450, 500));  // High

  flowInput.addFuzzySet(new FuzzySet(0, 10, 15, 20));    // Low
  flowInput.addFuzzySet(new FuzzySet(15, 30, 40, 50));   // Medium
  flowInput.addFuzzySet(new FuzzySet(50, 70, 80, 100));  // High

  floodOutput.addFuzzySet(new FuzzySet(0, 25, 50, 75));    // Low
  floodOutput.addFuzzySet(new FuzzySet(50, 60, 70, 80));   // Medium
  floodOutput.addFuzzySet(new FuzzySet(70, 80, 90, 100));  // High
}

void loop() {
  // Reading sensors
  Serial.println("===========================================");

  // Rain Sensor
  int rainValue = analogRead(OutputAO);            // Assumed range 0 - 1023
  float rainMM = map(rainValue, 0, 1023, 0, 100);  // Convert to mm

  // Water Height Sensor
  float distance = readUltrasonicSensor();  // In cm

  // Flow Sensor
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {
    flowRate = (flowCount / FLOW_CALIBRATION);  // L/min
    flowCount = 0;                              // Reset pulse count
    lastTime = currentTime;
  }

  // Store the data for trimmed mean calculation
  rainData[currentIndex] = rainMM;
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

  // Calculate Trimmed Mean for the three sensors
  float trimmedRain = calculateTrimmedMean(rainData, NUM_SAMPLES);
  float trimmedHeight = calculateTrimmedMean(heightData, NUM_SAMPLES);
  float trimmedFlow = calculateTrimmedMean(flowData, NUM_SAMPLES);

  // Displaying results
  Serial.print("Curah Hujan: ");
  Serial.print(trimmedRain);
  Serial.print(" mm, ");
  Serial.print("Ketinggian Air: ");
  Serial.print(trimmedHeight);
  Serial.print(" cm, ");
  Serial.print("Debit Air: ");
  Serial.print(trimmedFlow);
  Serial.print(" L/min, ");
  Serial.print("Risiko Banjir: ");
  Serial.print(floodRisk);
  Serial.print(" - ");

  // Flood Risk Category based on fuzzy output
  if (floodRisk <= 25) {
    Serial.println("Rendah");
  } else if (floodRisk <= 70) {
    Serial.println("Sedang");
  } else {
    Serial.println("Tinggi");
  }

  // Flood Risk Detection and Warning
  if (floodRisk > 70) {
    digitalWrite(Led, HIGH);     // LED ON
    digitalWrite(Buzzer, HIGH);  // Buzzer ON
  } else {
    digitalWrite(Led, LOW);     // LED OFF
    digitalWrite(Buzzer, LOW);  // Buzzer OFF
  }

  delay(2000);  // Delay 2 seconds
}

float readUltrasonicSensor() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(1000);
  digitalWrite(TRIG_PIN, HIGH);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return (duration / 2.0) * (SOUND_VELOCITY / 10000.0);  // Distance in cm
}

void countFlow() {
  flowCount++;  // Count the pulse for water flow
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
