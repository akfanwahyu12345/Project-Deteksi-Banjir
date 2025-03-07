#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2
#define ledpin 27

void setup() {
  Serial.begin(115200);
  pinMode(ledpin, OUTPUT);
  digitalWrite(ledpin, LOW);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {  // Adjust frequency for your region (e.g., 433E6 for Europe)
    Serial.println("LoRa Error");
    while (!LoRa.begin(433E6)) {
      digitalWrite(ledpin, HIGH);
      delay(300);
      digitalWrite(ledpin, LOW);
      delay(300);
    }
  }
  digitalWrite(ledpin, HIGH);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);  // Max power for PA_BOOST pin

  // Optional: Set Spreading Factor (SF), Bandwidth, and Coding Rate for maximum range
  LoRa.setSpreadingFactor(12);     // SF can be between 6-12
  LoRa.setSignalBandwidth(125E3);  // Bandwidth options: 7.8E3 to 500E3
  LoRa.setCodingRate4(8);          // Coding rate: 5-8, where 8 provides maximum error correction
  Serial.println("LoRa Tx Started");
}

void loop() {
  String message = "HELLO";  // Define the message to be sent

  Serial.print("Sending: ");
  Serial.println(message);

  LoRa.beginPacket();   // Start LoRa packet
  LoRa.print(message);  // Add the message to the packet
  LoRa.endPacket();     // Finish and send the packet

  delay(1000);  // Wait 1 second before sending the next message
}
