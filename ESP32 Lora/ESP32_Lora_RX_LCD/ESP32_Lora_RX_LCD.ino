#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <LoRa.h>
#define SS 5
#define RST 14
#define DIO0 2
#define ledpin 27
int Level = 0;
String received_message = "";  // Declare a string to store received messages

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 LORA");
  lcd.setCursor(0, 1);
  lcd.print("Communication");
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
  Serial.println("Lora RX Started");
  delay(1000);
  
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    received_message = "";
    while (LoRa.available()) {
      received_message += (char)LoRa.read();
      delay(5);
    }
    Level = received_message.toInt();
    Serial.println(Level);
    digitalWrite(ledpin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lora Received");
    lcd.setCursor(0, 1);
    lcd.print("Number: ");
    lcd.print(Level);
    lcd.print("   ");
    delay(300);
  } else
    digitalWrite(ledpin, LOW);
}
