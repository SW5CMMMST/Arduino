#include <VirtualWire.h>

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
  vw_set_tx_pin(transmit_pin);
  vw_setup(2000);       // Bits per sec
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
  switch(n) {
  }

  vw_send((uint8_t *)msg, 2);
  vw_wait_tx();

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
