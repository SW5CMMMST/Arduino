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
int i = 0;

void loop()
{
    //int m
    char aaa[16];
    //sprintf(aaa, "%d", msg);
    itoa(i, aaa, 10);
    driver.send((uint8_t *)aaa, sizeof(aaa));
    driver.waitPacketSent();
    delay(200);
    Serial.println(i);
    i++;
}
