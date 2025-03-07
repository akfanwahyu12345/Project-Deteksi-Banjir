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
  // Menyelaraskan pengaturan dengan transmitter
  LoRa.setSpreadingFactor(12);                  // Spreading Factor 12 (harus sama dengan transmitter)
  LoRa.setSignalBandwidth(125E3);               // Bandwidth 125 kHz (harus sama dengan transmitter)
  LoRa.setCodingRate4(8);   
  Serial.println("LoRa Receiver Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }
    Serial.print("Received: ");
    Serial.println(receivedMessage);  // Menampilkan pesan yang diterima
  } else {
    Serial.println("Waiting for packet...");
  }
}
