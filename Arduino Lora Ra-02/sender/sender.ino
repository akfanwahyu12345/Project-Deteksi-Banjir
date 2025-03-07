#include <SPI.h>
#include <LoRa.h>
#define SS 5
#define RST 14
#define DIO0 2
String InputString;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  LoRa.setPins(SS, RST, DIO0);
  Serial.println("LoRa Sender");

  if (!LoRa.begin(433E6)) { //use (915E6) for LoRa Ra-02 915 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  //LoRa.setSpreadingFactor(10);
  //LoRa.setSignalBandwidth(62.5E3);
  //LoRa.crc();

}

void loop() {
  while (Serial.available()) {
    InputString = Serial.readString();
    LoRa.beginPacket();
    LoRa.print(InputString);
    LoRa.endPacket();
    Serial.print("Lora Send : ");
    Serial.println(InputString);
  }
}
