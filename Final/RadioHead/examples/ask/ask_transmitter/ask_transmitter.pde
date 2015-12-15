// ask_transmitter.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to transmit messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) transmitter with an TX-C1 module

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver;

void setup()
{
    Serial.begin(9600);	  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

uint8_t len = 1;

void loop()
{
    const char *msg = "hellohellohellohellohellohellohellohellohellohellohellohello";
    
    driver.send((uint8_t *)msg, len);
    driver.waitPacketSent();
    len = (len + 1) % 16;
    delay(200);
}
