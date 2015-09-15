#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver;

// Setup for buttons
const int buttonPin[3] = {2,3,4};
int buttonState[3] = {0,0,0};
int lastState[3] = {0,0,0};

// Globals for transmitter
byte count = 1;
const int led_pin = 13;
const int transmit_pin = 12;

void setup() {
  // Button
  pinMode(buttonPin[0], INPUT);
  pinMode(buttonPin[1], INPUT);
  pinMode(buttonPin[2], INPUT);
  Serial.begin(9600);

  // Transmitter
  if (!driver.init())
         Serial.println("init failed");
  pinMode(led_pin, OUTPUT);
  Serial.println("Ready to send!");
  //StartSignal();
}

void loop(){
  UpdateButton(0);
  UpdateButton(1);
  UpdateButton(2); 
}

void StartSignal() {
  for(int i = 0; i < 30 ; i++)
  {
    int n = i % 4 == 3 ? 1 : i % 4;
    SendData(n, HIGH);
    delay(200);  
    SendData(n, LOW);
    delay(100);
  }
}

void SendData(int n, int status){
  char msg[2];


  switch(n) {
    case 0:
      msg[0] = 'r';
      msg[1] = status;
      break;
    case 1:
      msg[0] = 'y';
      msg[1] = status;
      break;
    case 2:
      msg[0] = 'g';
      msg[1] = status;
      break;
  }
  unsigned long t0 = millis();
  
  driver.send((uint8_t *)msg, strlen(msg));
  driver.waitPacketSent();

  unsigned long t1 = millis();

  unsigned long t_delta = t1 - t0;

  Serial.print(t_delta);
  
  Serial.print(n);
  Serial.print(": ");
  Serial.println(status ? "HIGH" : "LOW");
  delay(1);
}

void UpdateButton(int n) {
  // Read current button state
  buttonState[n] = digitalRead(buttonPin[n]);

  // Only write if state has changed
  if(lastState[n] != buttonState[n]) {
    SendData(n, buttonState[n]);
    // Set the last button state
    lastState[n] = buttonState[n];
  }
}
