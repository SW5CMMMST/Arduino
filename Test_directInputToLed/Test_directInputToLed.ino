void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(9, INPUT);
  pinMode(13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int d = digitalRead(9);
  digitalWrite(13, d);
  Serial.println(d);
  //delay(100);
}
