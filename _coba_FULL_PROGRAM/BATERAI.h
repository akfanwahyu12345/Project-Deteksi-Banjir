#define pinTegangan 35

float faktorTrim = 0.20; //5%-20%
int jumlahSampel = 50;
float maxV = 1260.00;  // maksimum tegangan baterai
float minV = 900.00;  // minimum tegangan baterai

bool notifBAT;

//bool bateraiAman();

void setupBaterai() {
  pinMode(pinTegangan, INPUT);
  //  bateraiAman();
}

int persenBAT() {
  int rawVoltage = analogRead(pinTegangan);

  //ngambil data sesuai jumlah sample
  float nilaiSensor[jumlahSampel];
  for (int i = 0; i < jumlahSampel; i++) {
    nilaiSensor[i] = rawVoltage;
  }

  //proses pengurutan (sorting)
  for (int i = 0; i < jumlahSampel - 1; i++) {
    for (int j = i + 1; j < jumlahSampel; j++) {
      if (nilaiSensor[i] > nilaiSensor[j]) {
        float temp = nilaiSensor[i];
        nilaiSensor[i] = nilaiSensor[j];
        nilaiSensor[j] = temp;
      }
    }
  }

  //rata-rata (trimmed mean)
  float sum = 0;
  for (int i = jumlahSampel * (faktorTrim / 2); i < jumlahSampel * (1 - (faktorTrim / 2)); i++) {
    sum += nilaiSensor[i];
  }

  int trimmedMean = sum / (jumlahSampel * (1 - faktorTrim));
  float regresiLinear = (0.0045 * trimmedMean) + 0.6842; //y = 0.0041x + 2.3775
  int persen = map(regresiLinear * 100, minV, maxV, 0, 100);

  return persen;
}
