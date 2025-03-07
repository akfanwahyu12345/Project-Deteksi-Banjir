//terima string metode serial event untuk arduino nano tidak berlaku untuk uno
#include <SPI.h>
#include <LoRa.h>
#define SS 5
#define RST 14
#define DIO0 2
int Led = 3;
String inputString = "";         // a String to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() {
  pinMode(Led, OUTPUT);
  Serial.begin(9600);
  while (!Serial);
  LoRa.setPins(SS, RST, DIO0);
  Serial.println("LoRa Receiver");

  if (!LoRa.begin(433E6)) { //use (915E6) for LoRa Ra-02 915 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  //LoRa.setSpreadingFactor(10);
  //LoRa.setSignalBandwidth(62.5E3);
  //LoRa.crc();
  inputString.reserve(200);
  digitalWrite(Led, HIGH);
  delay(100);
  digitalWrite(Led, LOW);
  delay(100);
  digitalWrite(Led, HIGH);
  delay(100);
  digitalWrite(Led, LOW);
  delay(100);
}

void loop() {
  serialEvent();
  if (stringComplete) {
    Serial.println(inputString);
    if (inputString=="Led On\n"){
      digitalWrite(Led, HIGH);
      //LoRa.beginPacket();
      //LoRa.print("Led On\n");
      //Serial.print("Led On");
      //LoRa.endPacket();
    }
    else if (inputString=="Led Blink\n"){
      digitalWrite(Led, HIGH);
      delay(500);
      digitalWrite(Led, LOW);
      delay(500);
      digitalWrite(Led, HIGH);
      delay(500);
      digitalWrite(Led, LOW);
      delay(500);
      digitalWrite(Led, HIGH);
      delay(500);
      digitalWrite(Led, LOW);
      delay(500);
      digitalWrite(Led, HIGH);
      delay(500);
      digitalWrite(Led, LOW);
      delay(500);
    }
    else if (inputString=="Led Off\n"){
      digitalWrite(Led, LOW);
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  } 
}

void serialEvent() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    //Serial.print("Received packet = ");

    // read packet
    while (LoRa.available()) {
      // get the new byte:
      char inChar = (char)LoRa.read();
      // add it to the inputString:
      inputString += inChar;
      // if the incoming character is a newline, set a flag so the main loop can
      // do something about it:
      if (inChar == '\n') {
        stringComplete = true;
      }
    }

    // print RSSI of packet
    //Serial.print("' with RSSI ");
    //Serial.println(LoRa.packetRssi());
  }
}
