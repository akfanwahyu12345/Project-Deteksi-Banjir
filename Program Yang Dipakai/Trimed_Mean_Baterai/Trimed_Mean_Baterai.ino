// Define analog input for battery voltage measurement
#define pinTegangan 27

// Resistor values in voltage divider
float R1 = 30000.0;
float R2 = 7500.0;
float in_voltage = 0;
float adc_voltage = 0;

// Reference Voltage
float ref_voltage = 5.0;

// Variables for battery capacity measurement
float faktorTrim = 0.20; // 5%-20%
int jumlahSampel = 50;
float maxV = 14.2;  // maksimum tegangan baterai
float minV = 3.7;  // minimum tegangan baterai

void setup() {
  Serial.begin(9600);
  pinMode(pinTegangan, INPUT);
}

float readBatteryVoltage() {
  int adc_value = analogRead(pinTegangan);
  adc_voltage = (adc_value * ref_voltage) / 1023.0; // Convert ADC value to voltage
  in_voltage = adc_voltage * ((R1 + R2) / R2); // Calculate input voltage based on voltage divider
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

void loop() {
  int batteryLevel = persenBAT();
  Serial.print("Battery Level: ");
  Serial.print(batteryLevel);
  Serial.println("%");
  Serial.print("Battery: ");
  Serial.print(adc_voltage);
  Serial.println("V");

  delay(1000); // Delay between readings
}
