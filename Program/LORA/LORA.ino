#include <LoRa.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
 
#define ss 5
#define rst 14
#define dio0 2
 
int counter = 0;
 
// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     
 
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
 
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
 
void setup() 
{
  Serial.begin(115200); 
  sensors.begin();
  while (!Serial);
  Serial.println("LoRa Sender");
 
  LoRa.setPins(ss, rst, dio0);    //setup LoRa transceiver module
  
  while (!LoRa.begin(433E6))     //433E6 - Asia, 866E6 - Europe, 915E6 - North America
  {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xA5);
  Serial.println("LoRa Initializing OK!");
}
 
void loop() 
{
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print("Sending packet: ");
  Serial.println(counter);
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println("ยบC");
  Serial.println("");
 
  LoRa.beginPacket();   //Send LoRa packet to receiver
  LoRa.print("Pckt: ");
  LoRa.println(counter);
  LoRa.print("Temp: ");
  LoRa.print(temperatureC);
  LoRa.println(" C");
  LoRa.endPacket();
 
  counter++;
 
  delay(4000);
}