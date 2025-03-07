#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2

void setup() {
  Serial.begin(115200);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {  // Frekuensi untuk wilayah Anda
    Serial.println("LoRa initialization failed!");
    while (true)
      ;  // Hentikan program jika inisialisasi gagal
  }

  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);  // Daya maksimum
  LoRa.setSpreadingFactor(12);                  // Spreading Factor 12
  LoRa.setSignalBandwidth(125E3);               // Bandwidth 125 kHz
  LoRa.setCodingRate4(8);                       // Coding Rate 4/8

  Serial.println("LoRa Transmitter Ready");
}

void loop() {
  Serial.println("Sending: aku sayang");
  LoRa.beginPacket();        // Memulai paket
  LoRa.print("aku sayang");  // Mengirimkan pesan
  LoRa.endPacket();          // Mengirimkan paket

  delay(1000);  // Delay 1 detik
}
