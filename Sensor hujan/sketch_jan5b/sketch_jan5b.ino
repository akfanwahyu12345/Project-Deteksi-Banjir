// Sensor Curah Hujan
int Led = 13;
int OutputDO = 34;
int OutputAO = 33;
int Buzzer = 26;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(Led, OUTPUT);
  pinMode(OutputDO, INPUT);
  pinMode(OutputAO, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int rainValue = analogRead(OutputAO);
  // Serial.print("Curah Hujan: ");
  Serial.print(rainValue);
}
