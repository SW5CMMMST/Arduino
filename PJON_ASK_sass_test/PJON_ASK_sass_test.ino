#define DEBUG
#include <SPI.h>
#include <EEPROM.h>
#include <Event.h>
#include <Timer.h>
#include <sass_ask.h>
#include <PJON_ASK.h>
#define ADDR EEPROM.read(0)
#define RX_PIN 11
#define TX_PIN 12

#ifdef DEBUG
char payloadString[64];
#endif

Timer t;
PJON_ASK network(RX_PIN, TX_PIN, ADDR);
payload_type out_payload;
payload_type in_payload;

static void receiver_function(uint8_t length, uint8_t *payload) {
  memcpy(&in_payload, payload, length);
  #ifdef DEBUG
  makePayloadString(in_payload, payloadString);
  Serial.print(payloadString);
  #endif
  network.send(BROADCAST, (char*)&in_payload, sizeof(payload_type));
}

void setup() {
  Serial.begin(9600);
  network.set_receiver(receiver_function);
}

void loop() {
  network.update();
  network.receive(1000);
}


