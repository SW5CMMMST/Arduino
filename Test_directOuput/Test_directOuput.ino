void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(4, OUTPUT);
  pinMode(13, OUTPUT);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(4, HIGH);
  digitalWrite(13, HIGH);

  delay(300);
  digitalWrite(4, LOW);
  digitalWrite(13, LOW);
  delay(300);
}
