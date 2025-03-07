/*
 * Example on how to use the Wiegand reader library with interruptions.
 */

#include <Wiegand.h>
#include <Wire.h>

// These are the pins connected to the Wiegand D0 and D1 signals.
// Ensure your board supports external Interruptions on these pins
#define PIN_D0 2
#define PIN_D1 3

// The object that handles the wiegand protocol
Wiegand wiegand;

// Initialize Wiegand reader
void setup() {
  Serial.begin(9600);
  // wiegand.begin(Wiegand::LENGTH_ANY, true);
  wiegand.begin();



  //Install listeners and initialize Wiegand reader
  wiegand.onReceive(receivedData, "Card readed: ");
  wiegand.onReceiveError(receivedDataError, "Card read error: ");
  wiegand.onStateChange(stateChanged, "State changed: ");

  //initialize pins as INPUT and attaches interruptions
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_D0), pinStateChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_D1), pinStateChanged, CHANGE);

  //Sends the initial pin state to the Wiegand library
  pinStateChanged();
}

// Every few milliseconds, check for pending messages on the wiegand reFader
// This executes with interruptions disabled, since the Wiegand library is not thread-safe
void loop() {

  if (wiegand.available()) {
    Serial.print("Wiegand HEX = ");
    Serial.print(wiegand.getCode(), HEX);
    Serial.print(", DECIMAL = ");
    Serial.print(wiegand.getCode());
    Serial.print(", Type W");
    Serial.println(wiegand.getWiegandType());
  }
  // noInterrupts();
  // wiegand.flush();
  // interrupts();
  // wiegand.onReceive(receivedData, "Card readed: ");

  //Sleep a little -- this doesn't have to run very often.
  delay(1000);
}

// When any of the pins have changed, update the state of the wiegand library
void pinStateChanged() {
  wiegand.setPin0State(digitalRead(PIN_D0));
  wiegand.setPin1State(digitalRead(PIN_D1));
}

// Notifies when a reader has been connected or disconnected.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onStateChange()`
void stateChanged(bool plugged, const char* message) {
  Serial.print(message);
  Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

// Notifies when a card was read.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onReceive()`
void receivedData(uint8_t* data, uint8_t bits, const char* message) {
  Serial.print(message);
  Serial.print(bits);
  Serial.print("bits / ");
  //Print value in HEX
  uint8_t bytes = (bits + 7) / 8;
  for (int i = 0; i < bytes; i++) {
    Serial.print(data[i] >> 4, 16);
    Serial.print(data[i] & 0xF, 16);
  }
  Serial.println();
}

// Notifies when an invalid transmission is detected
void receivedDataError(Wiegand::DataError error, uint8_t* rawData, uint8_t rawBits, const char* message) {
  Serial.print(message);
  Serial.print(Wiegand::DataErrorStr(error));
  Serial.print(" - Raw data: ");
  Serial.print(rawBits);
  Serial.print("bits / ");

  //Print value in HEX
  uint8_t bytes = (rawBits + 7) / 8;
  for (int i = 0; i < bytes; i++) {
    Serial.print(rawData[i] >> 4, 16);
    Serial.print(rawData[i] & 0xF, 16);
  }
  Serial.println();
}