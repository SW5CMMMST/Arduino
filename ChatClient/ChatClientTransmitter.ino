#include <VirtualWire.h>

#define MAXLEN 32

// Globals for transmitter
byte count = 1;
const int led_pin = 13;
const int transmit_pin = 12;

void setup() {
  Serial.begin(9600);

  // Transmitter
  vw_set_tx_pin(transmit_pin);
  vw_setup(2000);       // Bits per sec
  pinMode(led_pin, OUTPUT);
  Serial.println("Ready to send!");
}

void loop(){
  char buffer[MAXLEN];
  int len = 0;

  Serial.print("> ")

  while((buffer[len] = Serial.Read()) != '\n' && len < MAXLEN) {
    len++;
  }

  SendText(buffer, len);
}

void SendText(char* buffer, int len) {
  vm_send((uint8_t *) msg, len);
  vm_wait_tx();
  Serial.print("Sent message:");
  Serial.println(buffer);
}