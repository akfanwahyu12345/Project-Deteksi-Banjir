#define DESTINATION_ADDL 20
#include "Arduino.h"
#include "LoRa_E220.h"
#include <SoftwareSerial.h>;
SoftwareSerial mySerial(2, 3);
LoRa_E220 e220ttl(&mySerial);

void setup() {
  Serial.begin(9600);
  delay(500);
  e220ttl.begin();
}

void loop() {
  if (e220ttl.available() > 1) {
    ResponseContainer rc = e220ttl.receiveMessage();
    if (rc.status.code != 1) {
      Serial.println(rc.status.getResponseDescription());
    } else {
      Serial.println(rc.status.getResponseDescription());
      Serial.println(rc.data);
    }
  }

  if (Serial.available() > 1) {
    String input = Serial.readString();
    ResponseStatus rs = e220ttl.sendFixedMessage(0, DESTINATION_ADDL, 23, input);
    Serial.println(rs.getResponseDescription());
  }
}