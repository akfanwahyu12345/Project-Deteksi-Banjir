#include "Arduino.h"
#include "LoRa_E220.h"
LoRa_E220 e220ttl(&Serial2);
void setup() {
  Serial.begin(9600);
  delay(500);
  e220ttl.begin();
  Serial.println("Start receiving!");
}

void loop() {
  if (e220ttl.available() > 1) {
    Serial.println("Message received!");
    ResponseContainer rc = e220ttl.receiveMessage();
    if (rc.status.code != 1) {
      Serial.println(rc.status.getResponseDescription());
    } else {
      Serial.println(rc.data);
      Serial.println(rc.status.getResponseDescription());
    }
  }
  delay(100);
}